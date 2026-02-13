/**
 * @file main.c
 * @brief Program to connect to Studer XCom-232, get data and push to MQTT
 *
 * This program connects to a Studer XCom-232 device, retrieves data, and publishes it to an MQTT broker.
 *
 * @license MIT License
 *
 * @author kolin
 *
 * @dependencies
 * This program uses the following library:
 * - [Studer Library](https://github.com/k3a/studer)
 */

#include "main.h"
#include "../scomlib_extra/scomlib_extra.h"
#include "serial.h"
#include <mosquitto.h>
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <termios.h> // for baud rate constant
#include <unistd.h>  // for usleep()
#include <time.h>     // for timestamps
#include <pthread.h>  // for mutex
#include <signal.h>   // for signal handling
#include <stdlib.h>   // for exit()

// Constants
#define SCOM_MAX_FRAME_SIZE 128
#define MAX_REQUEST_ATTEMPTS 3
#define MQTT_HEALTH_CHECK_INTERVAL 60  // seconds
#define DELAY_BETWEEN_PARAMS_US 10000  // 10ms in microseconds
#define DELAY_END_OF_CYCLE_US 100000   // 100ms in microseconds

// MQTT connection state tracking (protected by mutex)
static int mqtt_connected = 0;
static int comm_status_online = 0;  // Track if we're receiving valid serial data
static time_t last_mqtt_check = 0;
static pthread_mutex_t mqtt_mutex = PTHREAD_MUTEX_INITIALIZER;

// Global for cleanup on signal
static struct mosquitto *g_mqtt_client = NULL;
static volatile sig_atomic_t g_shutdown_requested = 0;

// Signal handler for graceful shutdown
void signal_handler(int signum)
{
    printf("\n[%ld] Received signal %d, initiating graceful shutdown...\n", time(NULL), signum);
    g_shutdown_requested = 1;
}

// Publish Home Assistant MQTT Discovery config for a single sensor
void publish_discovery_config(struct mosquitto *mosq, const parameter_t *param)
{
    char config_topic[256];
    char unique_id[128];
    char state_topic[256];
    
    // Create unique_id: xtender_<name>
    snprintf(unique_id, sizeof(unique_id), "xtender_%s", param->name);
    
    // State topic
    snprintf(state_topic, sizeof(state_topic), "%s/%s/%s", mqtt_topic, param->mqtt_prefix, param->name);
    
    // Discovery topic: homeassistant/sensor/<unique_id>/config
    snprintf(config_topic, sizeof(config_topic), "homeassistant/sensor/%s/config", unique_id);
    
    // Build JSON config
    struct json_object *config = json_object_new_object();
    json_object_object_add(config, "name", json_object_new_string(param->friendly_name));
    json_object_object_add(config, "unique_id", json_object_new_string(unique_id));
    json_object_object_add(config, "object_id", json_object_new_string(unique_id));
    json_object_object_add(config, "has_entity_name", json_object_new_boolean(false));
    json_object_object_add(config, "state_topic", json_object_new_string(state_topic));
    json_object_object_add(config, "availability_topic", json_object_new_string("studer/commstatus"));
    json_object_object_add(config, "payload_available", json_object_new_string("online"));
    json_object_object_add(config, "payload_not_available", json_object_new_string("offline"));
    json_object_object_add(config, "expire_after", json_object_new_int(20));
    
    // Add unit of measurement (convert kW/kVA to W/VA)
    if (strcmp(param->unit, "kW") == 0) {
        json_object_object_add(config, "unit_of_measurement", json_object_new_string("W"));
        json_object_object_add(config, "value_template", json_object_new_string("{{ value | float * 1000 }}"));
    } else if (strcmp(param->unit, "kVA") == 0) {
        json_object_object_add(config, "unit_of_measurement", json_object_new_string("VA"));
        json_object_object_add(config, "value_template", json_object_new_string("{{ value | float * 1000 }}"));
    } else {
        json_object_object_add(config, "unit_of_measurement", json_object_new_string(param->unit));
    }
    
    // Add device class and state class
    json_object_object_add(config, "device_class", json_object_new_string(param->device_class));
    json_object_object_add(config, "state_class", json_object_new_string("measurement"));
    
    // Add device with empty name - keeps sensors grouped but prevents name concatenation
    struct json_object *device = json_object_new_object();
    struct json_object *identifiers = json_object_new_array();
    json_object_array_add(identifiers, json_object_new_string("studer_xtender"));
    json_object_object_add(device, "identifiers", identifiers);
    json_object_object_add(device, "name", json_object_new_string(""));  // Empty name prevents concatenation
    json_object_object_add(device, "manufacturer", json_object_new_string("Studer Innotec"));
    json_object_object_add(device, "model", json_object_new_string("Xtender XTM4000-48"));
    json_object_object_add(config, "device", device);
    
    // Get JSON string and publish
    const char *json_str = json_object_to_json_string(config);
    mosquitto_publish(mosq, NULL, config_topic, (int)strlen(json_str), json_str, 0, true);
    
    json_object_put(config);  // Free JSON object
}

// MQTT callbacks
void on_connect(struct mosquitto *mosq, void *obj __attribute__((unused)), int rc)
{
    pthread_mutex_lock(&mqtt_mutex);
    mqtt_connected = (rc == 0) ? 1 : 0;
    pthread_mutex_unlock(&mqtt_mutex);
    
    printf("[%ld] MQTT connect callback: rc=%d (%s)\n", time(NULL), rc, 
           rc == 0 ? "success" : "failed");
    
    if (rc == 0) {
        // Reset status flag so we republish online after reconnection
        comm_status_online = false;
        
        // Publish Home Assistant discovery configs for all sensors
        printf("[%ld] Publishing MQTT Discovery configs...\n", time(NULL));
        for (size_t i = 0; i < NUM_PARAMETERS; i++) {
            publish_discovery_config(mosq, &requested_parameters[i]);
        }
        printf("[%ld] Discovery configs published (%zu sensors)\n", time(NULL), NUM_PARAMETERS);
    }
}

void on_disconnect(struct mosquitto *mosq __attribute__((unused)), 
                   void *obj __attribute__((unused)), int rc)
{
    pthread_mutex_lock(&mqtt_mutex);
    mqtt_connected = 0;
    pthread_mutex_unlock(&mqtt_mutex);
    
    printf("[%ld] MQTT disconnected: rc=%d (%s)\n", time(NULL), rc,
           rc == 0 ? "clean disconnect" : "unexpected disconnect");
}

// Function to read a parameter from a device at a specific address
read_param_result_t read_param(int addr, int parameter)
{
    read_param_result_t result;
    scomx_enc_result_t encresult;
    scomx_header_dec_result_t dechdr;
    scomx_dec_result_t decres;
    size_t bytecounter;
    char readbuf[SCOM_MAX_FRAME_SIZE];

    // Initialize result to prevent undefined behavior
    result.value = 0.0f;
    result.error = -1;

    for (int request_attempt = 0; request_attempt < MAX_REQUEST_ATTEMPTS; request_attempt++) {
        // Encode the read user info value command
        encresult = scomx_encode_read_user_info_value(addr, parameter);

#ifdef SERIAL_DEBUG
        if (request_attempt > 0) {
            printf("[SCOM DEBUG] Retry %d: Reading param %d from addr %d\n", 
                   request_attempt, parameter, addr);
        } else {
            printf("[SCOM DEBUG] Reading param %d from addr %d\n", parameter, addr);
        }
        printf("[SCOM DEBUG] Encoded command (%zu bytes): ", encresult.length);
        for (size_t i = 0; i < encresult.length; i++) {
            printf("%02X ", encresult.data[i]);
        }
        printf("\n");
#endif

        // Write the encoded command to the serial port
        bytecounter = serial_write(encresult.data, encresult.length);
        if (bytecounter != encresult.length) {
            printf("Serial write failed: sent %zu of %zu bytes\n", bytecounter, encresult.length);
            serial_flush();  // Clear buffer on write failure
            continue;  // Retry the request
        }
        // Read the frame header from the serial port
        bytecounter = serial_read(readbuf, SCOM_FRAME_HEADER_SIZE);
        if (bytecounter != SCOM_FRAME_HEADER_SIZE) {
            if (bytecounter == 0) {
                printf("Serial timeout: no header received (inverter disconnected?)\n");
            } else {
                printf("Serial header read failed: got %zu of %d bytes\n", bytecounter, SCOM_FRAME_HEADER_SIZE);
            }
            serial_flush(); // Clear buffer on error
            result.error = -1;
            return result;
        }

        // Decode the frame header
        dechdr = scomx_decode_frame_header(readbuf, SCOM_FRAME_HEADER_SIZE);
        if (dechdr.error != SCOM_ERROR_NO_ERROR) {
#ifdef SERIAL_DEBUG
            printf("[SCOM DEBUG] Header decode failed: error %d\n", dechdr.error);
#endif
            serial_flush(); // Clear buffer on error
            result.error = -1;
            return result;
        }
#ifdef SERIAL_DEBUG
        printf("[SCOM DEBUG] Header decoded, need to read %zu more bytes\n", dechdr.length_to_read);
#endif

        // Sanity check on length to prevent hang
        if (dechdr.length_to_read > sizeof(readbuf) || dechdr.length_to_read == 0) {
#ifdef SERIAL_DEBUG
            printf("[SCOM DEBUG] Invalid length_to_read: %zu (buffer size: %zu)\n",
                   dechdr.length_to_read, sizeof(readbuf));
#endif
            serial_flush(); // Clear buffer on error
            result.error = -1;
            return result;
        }

        // Read the frame data from the serial port
        bytecounter = serial_read(readbuf, dechdr.length_to_read);
        if (bytecounter != dechdr.length_to_read) {
            if (bytecounter == 0) {
                printf("Serial timeout: no data received\n");
            } else {
                printf("Serial data read failed: got %zu of %zu bytes\n", bytecounter, dechdr.length_to_read);
            }
            serial_flush(); // Clear buffer on error
            result.error = -1;
            return result;
        }

        // Decode the frame data
        decres = scomx_decode_frame(readbuf, dechdr.length_to_read);
        if (decres.error != SCOM_ERROR_NO_ERROR) {
#ifdef SERIAL_DEBUG
            printf("[SCOM DEBUG] Frame decode failed: error %d\n", decres.error);
#endif
            serial_flush(); // Clear buffer on error
            result.error = -1;
            return result;
        }

        // Validate response matches the requested parameter
        if (decres.service_id != SCOM_READ_PROPERTY_SERVICE ||
            decres.object_type != SCOM_USER_INFO_OBJECT_TYPE ||
            decres.property_id != SCOMX_PROP_USER_INFO_VALUE ||
            (int)decres.object_id != parameter ||
            (int)decres.src_addr != addr) {
            printf("Response mismatch (attempt %d/%d): expected param=%d addr=%d, got obj_id=%u addr=%u\n",
                   request_attempt + 1, MAX_REQUEST_ATTEMPTS,
                   parameter, addr, decres.object_id, decres.src_addr);
            serial_flush();  // Flush buffer before retry
            continue;  // Retry the entire request (outer loop)
        }

        // Extract the float value from the decoded frame
        result.value = scomx_result_float(decres);
        result.error = 0; // no error

#ifdef SERIAL_DEBUG
        printf("[SCOM DEBUG] Successfully decoded value: %.3f\n", result.value);
#endif

        return result;
    }

    // All retry attempts failed
    return result;
}

int main(int argc, const char *argv[])
{
    // Default serial port if no argument provided
    const char *port = "/dev/serial/by-path/platform-xhci-hcd.1.auto-usb-0:1.1.1:1.0-port0";

    // Check if a port is provided as a command line argument
    if (argc > 1) {
        port = argv[1];
    }

    printf("Studer serial comm test on port %s\n", port);

    // Initialize the serial port
    if (serial_init(port, B115200, PARITY_EVEN, 1) != 0) {
        return 1;
    }
    printf("Serial connection established\n");

    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // systemctl stop

    mosquitto_lib_init();
    struct mosquitto *mqtt_client = mosquitto_new(NULL, true, NULL);
    if (mqtt_client == NULL) {
        printf("Failed to create mosquitto client instance\n");
        mosquitto_lib_cleanup();
        return 1;
    }
    g_mqtt_client = mqtt_client;  // Store for signal handler

    // Set up MQTT callbacks
    mosquitto_connect_callback_set(mqtt_client, on_connect);
    mosquitto_disconnect_callback_set(mqtt_client, on_disconnect);

    // Set up the last will before connecting
    int rc = mosquitto_will_set(mqtt_client, "studer/commstatus", strlen(lwt_message), lwt_message, 0, true);
    if (rc != MOSQ_ERR_SUCCESS) {
        printf("Setting up Last Will and Testament failed, return code %d\n", rc);
        return rc;
    }

    // Enable automatic reconnection
    mosquitto_reconnect_delay_set(mqtt_client, 1, 30, true);

    printf("[%ld] Connecting to MQTT broker %s:%d\n", time(NULL), mqtt_server, mqtt_port);
    rc = mosquitto_connect(mqtt_client, mqtt_server, mqtt_port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        printf("Connect failed, return code %d - continuing anyway (will retry)\n", rc);
        mqtt_connected = 0;
    } else {
        printf("Initial MQTT connect() succeeded\n");
        mqtt_connected = 1;
    }
    
    // Start the network loop in background thread
    rc = mosquitto_loop_start(mqtt_client);
    if (rc != MOSQ_ERR_SUCCESS) {
        printf("Failed to start mosquitto loop, return code %d\n", rc);
        return rc;
    }

    // Give the connection a moment to establish
    sleep(1);

    while (!g_shutdown_requested) {
        // Check MQTT connection status every 60 seconds
        time_t now = time(NULL);
        if (now - last_mqtt_check >= MQTT_HEALTH_CHECK_INTERVAL) {
            last_mqtt_check = now;
            pthread_mutex_lock(&mqtt_mutex);
            int connected = mqtt_connected;
            pthread_mutex_unlock(&mqtt_mutex);
            
            if (!connected) {
                printf("[%ld] MQTT disconnected, attempting manual reconnect...\n", now);
                rc = mosquitto_reconnect(mqtt_client);
                printf("[%ld] Reconnect result: %d\n", now, rc);
            } else {
#ifdef SERIAL_DEBUG
                printf("[%ld] MQTT status: connected\n", now);
#endif
            }
        }

        // Iterate over the requested_parameters array
        for (size_t i = 0; i < NUM_PARAMETERS; i++) {
            // Get the current parameter
            parameter_t current_param = requested_parameters[i];

            // Read the parameter
            read_param_result_t result = read_param(current_param.address, current_param.parameter);

            // Current topic
            char topic[256];
            snprintf(topic, sizeof(topic), "%s/%s/%s", mqtt_topic, current_param.mqtt_prefix, current_param.name);

            // Check if the read was successful
            if (result.error == 0) {
                // First successful read - publish online status if not already done
                pthread_mutex_lock(&mqtt_mutex);
                if (!comm_status_online && mqtt_connected) {
                    mosquitto_publish(mqtt_client, NULL, "studer/commstatus", 6, "online", 0, true);
                    printf("[%ld] Serial communication established - status set to online\n", time(NULL));
                    comm_status_online = 1;
                }
                pthread_mutex_unlock(&mqtt_mutex);
                
#ifdef SERIAL_DEBUG
                // Print the parameter name and value
                printf("%s = %.3f %s\n", current_param.name, result.value * current_param.sign, current_param.unit);
#endif

                // Convert the float value to a string
                char value_str[32];
                snprintf(value_str, sizeof(value_str), "%.3f", result.value * current_param.sign);

                // Publish the value to MQTT
                rc = mosquitto_publish(mqtt_client, NULL, topic, (int)strlen(value_str), value_str, 0, false);
                if (rc != MOSQ_ERR_SUCCESS) {
                    printf("Publish failed, return code %d (continuing)\n", rc);
                    // Don't try to reconnect manually - loop_start handles it automatically
                }
            } else {
                // Serial read failed - set status to offline
                pthread_mutex_lock(&mqtt_mutex);
                if (comm_status_online) {
                    mosquitto_publish(mqtt_client, NULL, "studer/commstatus", 7, "offline", 0, true);
                    printf("[%ld] Serial communication lost - status set to offline\n", time(NULL));
                    comm_status_online = 0;
                }
                pthread_mutex_unlock(&mqtt_mutex);
                
                // Print an error message
                printf("%s = read failed\n", current_param.name);
                mosquitto_publish(mqtt_client, NULL, topic, 3, "nAn", 0, false);
            }
            
            // Small delay between parameters to avoid overwhelming inverter
            usleep(DELAY_BETWEEN_PARAMS_US);
        }

#ifdef SERIAL_DEBUG
        printf("---------------------------------------------------------\n");
#endif

        usleep(DELAY_END_OF_CYCLE_US);
    }

    // Cleanup
    printf("[%ld] Shutting down gracefully...\n", time(NULL));
    
    // Force stop the loop immediately (don't wait for thread)
    mosquitto_loop_stop(mqtt_client, true);
    
    // Disconnect forcefully
    mosquitto_disconnect(mqtt_client);
    
    // Try to send offline status (best effort, may not work after disconnect)
    // mosquitto_publish(mqtt_client, NULL, "studer/commstatus", 7, "offline", 0, true);
    
    mosquitto_destroy(mqtt_client);
    mosquitto_lib_cleanup();
    
    printf("[%ld] Shutdown complete.\n", time(NULL));
    return 0;
}
