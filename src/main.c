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
#include <stdio.h>
#include <string.h>
#include <termios.h> // for baud rate constant
#include <unistd.h>  // Include the header file for usleep

// Function to read a parameter from a device at a specific address
read_param_result_t read_param(int addr, int parameter)
{
    read_param_result_t result;
    scomx_enc_result_t encresult;
    scomx_header_dec_result_t dechdr;
    scomx_dec_result_t decres;
    size_t bytecounter;
    char readbuf[128];
    float outval = 0.0;

    // Encode the read user info value command
    encresult = scomx_encode_read_user_info_value(addr, parameter);

#ifdef SERIAL_DEBUG
    printf("[SCOM DEBUG] Reading param %d from addr %d\n", parameter, addr);
    printf("[SCOM DEBUG] Encoded command (%zu bytes): ", encresult.length);
    for (size_t i = 0; i < encresult.length; i++) {
        printf("%02X ", encresult.data[i]);
    }
    printf("\n");
#endif

    // Write the encoded command to the serial port
    bytecounter = serial_write(encresult.data, encresult.length);
    if (bytecounter != encresult.length) {
#ifdef SERIAL_DEBUG
        printf("[SCOM DEBUG] Write failed: sent %zu of %zu bytes\n", bytecounter, encresult.length);
#endif
        result.error = -1;
        return result;
    }

    // Read the frame header from the serial port
    bytecounter = serial_read(readbuf, SCOM_FRAME_HEADER_SIZE);
    if (bytecounter != SCOM_FRAME_HEADER_SIZE) {
#ifdef SERIAL_DEBUG
        printf("[SCOM DEBUG] Header read failed: got %zu of %d bytes\n", bytecounter, SCOM_FRAME_HEADER_SIZE);
#endif
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
    printf("[SCOM DEBUG] Header decoded, need to read %d more bytes\n", dechdr.length_to_read);
#endif

    // Sanity check on length to prevent hang
    if (dechdr.length_to_read > sizeof(readbuf) || dechdr.length_to_read == 0) {
#ifdef SERIAL_DEBUG
        printf("[SCOM DEBUG] Invalid length_to_read: %d (buffer size: %zu)\n", 
               dechdr.length_to_read, sizeof(readbuf));
#endif
        serial_flush(); // Clear buffer on error
        result.error = -1;
        return result;
    }

    // Read the frame data from the serial port
    bytecounter = serial_read(readbuf, dechdr.length_to_read);
    if (bytecounter != dechdr.length_to_read) {
#ifdef SERIAL_DEBUG
        printf("[SCOM DEBUG] Data read failed: got %zu of %d bytes\n", bytecounter, dechdr.length_to_read);
#endif
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

    // Extract the float value from the decoded frame
    result.value = scomx_result_float(decres);
    result.error = 0; // no error

#ifdef SERIAL_DEBUG
    printf("[SCOM DEBUG] Successfully decoded value: %.3f\n", result.value);
#endif

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

    mosquitto_lib_init();
    struct mosquitto *mqtt_client = mosquitto_new(NULL, true, NULL);

    // Set up the last will before connecting
    int rc = mosquitto_will_set(mqtt_client, "studer/commstatus", strlen(lwt_message), lwt_message, 0, true);
    if (rc != MOSQ_ERR_SUCCESS) {
        printf("Setting up Last Will and Testament failed, return code %d\n", rc);
        return rc;
    }

    // Enable automatic reconnection
    mosquitto_reconnect_delay_set(mqtt_client, 1, 30, true);

    rc = mosquitto_connect(mqtt_client, mqtt_server, mqtt_port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        printf("Connect failed, return code %d - continuing anyway (will retry)\n", rc);
    } else {
        printf("Connected to MQTT broker\n");
        
        // Publish "online" status after connecting
        char *online_status = "online";
        rc = mosquitto_publish(mqtt_client, NULL, "studer/commstatus", strlen(online_status), online_status, 0, true);
        if (rc != MOSQ_ERR_SUCCESS) {
            printf("Publish online status failed, return code %d\n", rc);
        }
    }
    
    // Start the network loop in background thread
    rc = mosquitto_loop_start(mqtt_client);
    if (rc != MOSQ_ERR_SUCCESS) {
        printf("Failed to start mosquitto loop, return code %d\n", rc);
        return rc;
    }

    while (1) {
        // Iterate over the requested_parameters array
        for (int i = 0; i < sizeof(requested_parameters) / sizeof(parameter_t); i++) {
            // Get the current parameter
            parameter_t current_param = requested_parameters[i];

            // Read the parameter
            read_param_result_t result = read_param(current_param.address, current_param.parameter);

            // Current topic
            char topic[256];
            snprintf(topic, sizeof(topic), "%s/%s/%s", mqtt_topic, current_param.mqtt_prefix, current_param.name);

            // Check if the read was successful
            if (result.error == 0) {
                // Print the parameter name and value
                printf("%s = %.3f %s\n", current_param.name, result.value * current_param.sign, current_param.unit);

                // Convert the float value to a string
                char value_str[32];
                snprintf(value_str, sizeof(value_str), "%.3f", result.value * current_param.sign);

                // Publish the value to MQTT
                rc = mosquitto_publish(mqtt_client, NULL, topic, strlen(value_str), value_str, 0, false);
                if (rc != MOSQ_ERR_SUCCESS) {
                    printf("Publish failed, return code %d (continuing)\n", rc);
                    // Don't try to reconnect manually - loop_start handles it automatically
                }
            } else {
                // Print an error message
                printf("%s = read failed\n", current_param.name);
                if (mqtt_client != NULL && topic != NULL) {
                    mosquitto_publish(mqtt_client, NULL, topic, 3, "nAn", 0, false);
                }
            }
        }

        printf("---------------------------------------------------------\n");

        usleep(500000); // Sleep for 500 milliseconds
    }

    // Cleanup for Mosquitto (never reached in this case, but good practice)
    mosquitto_loop_stop(mqtt_client, false);
    mosquitto_disconnect(mqtt_client);
    mosquitto_destroy(mqtt_client);
    mosquitto_lib_cleanup();
    return 0;
}
