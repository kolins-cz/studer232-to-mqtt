/**
 * @file studer_reset.c
 * @brief Simple tool to reset Studer devices to factory defaults
 * 
 * Usage:
 *   ./studer_reset [serial_port] [option]
 * 
 * Options:
 *   --system-reset       Reset all devices in the system (parameter 5121 at addr 501)
 *   --xtender-all        Reset all Xtenders to factory defaults (param 1287 at addr 100)
 *   --xtender <addr>     Reset specific Xtender (param 1287 at addr 101-109)
 *   --xcom               Reset Xcom-232i parameters (param 5121 at addr 501)
 * 
 * @license MIT License
 * @author kolin
 */

#include "../scomlib_extra/scomlib_extra.h"
#include "../src/serial.h"
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

// Parameter numbers for reset commands
#define PARAM_XTENDER_RESTORE_DEFAULT  1395  // Basic level: Restore default settings
#define PARAM_XTENDER_RESTORE_FACTORY  1287  // Installer level: Restore factory settings
#define PARAM_XCOM_RESET_ALL_DEVICES   5121  // Expert level: Reset all devices of the system
#define PARAM_XCOM_RESTORE_DEFAULTS    5044  // Installer level: Restore default access level

// Addresses
#define ADDR_XCOM232I           501
#define ADDR_ALL_XTENDERS       100
#define ADDR_XTENDER_START      101
#define ADDR_XTENDER_END        109

// Function to send reset command
int send_reset_command(int addr, int parameter) {
    scomx_enc_result_t encresult;
    scomx_header_dec_result_t dechdr;
    scomx_dec_result_t decres;
    size_t bytecounter;
    char readbuf[128];
    
    printf("\n==> Sending RESET command:\n");
    printf("    Address: %d\n", addr);
    printf("    Parameter: %d\n", parameter);
    
    // For signal parameters, we write a dummy value (1) to trigger the signal
    encresult = scomx_encode_write_parameter_value_u32(addr, parameter, 1);
    
    if (encresult.error != SCOM_ERROR_NO_ERROR) {
        printf("ERROR: Failed to encode command (error %d)\n", encresult.error);
        return -1;
    }
    
    printf("    Encoded %zu bytes: ", encresult.length);
    for (size_t i = 0; i < encresult.length; i++) {
        printf("%02X ", encresult.data[i]);
    }
    printf("\n");
    
    // Write the encoded command to the serial port
    bytecounter = serial_write(encresult.data, encresult.length);
    if (bytecounter != encresult.length) {
        printf("ERROR: Write failed (sent %zu of %zu bytes)\n", bytecounter, encresult.length);
        return -1;
    }
    printf("    Command sent successfully\n");
    
    // Read the frame header from the serial port
    bytecounter = serial_read(readbuf, SCOM_FRAME_HEADER_SIZE);
    if (bytecounter != SCOM_FRAME_HEADER_SIZE) {
        printf("ERROR: Failed to read response header (got %zu of %d bytes)\n", 
               bytecounter, SCOM_FRAME_HEADER_SIZE);
        return -1;
    }
    
    // Decode the frame header
    dechdr = scomx_decode_frame_header(readbuf, SCOM_FRAME_HEADER_SIZE);
    if (dechdr.error != SCOM_ERROR_NO_ERROR) {
        printf("ERROR: Failed to decode response header (error %d)\n", dechdr.error);
        return -1;
    }
    
    // Read the frame data from the serial port
    bytecounter = serial_read(readbuf, dechdr.length_to_read);
    if (bytecounter != dechdr.length_to_read) {
        printf("ERROR: Failed to read response data (got %zu of %d bytes)\n", 
               bytecounter, dechdr.length_to_read);
        return -1;
    }
    
    // Decode the frame data
    decres = scomx_decode_frame(readbuf, dechdr.length_to_read);
    if (decres.error != SCOM_ERROR_NO_ERROR) {
        printf("ERROR: Device returned error code %d\n", decres.error);
        return -1;
    }
    
    printf("    Response received: SUCCESS\n");
    return 0;
}

void print_usage(const char *prog_name) {
    printf("Studer Device Reset Tool\n\n");
    printf("Usage: %s [serial_port] [option]\n\n", prog_name);
    printf("Options:\n");
    printf("  --system-reset       Reset ALL devices in the system (RECOMMENDED)\n");
    printf("                       Uses parameter 5121 at Xcom-232i address 501\n\n");
    printf("  --xtender-all        Reset ALL Xtenders to factory defaults\n");
    printf("                       Uses multicast address 100\n\n");
    printf("  --xtender <addr>     Reset specific Xtender at address 101-109\n");
    printf("                       Example: --xtender 101\n\n");
    printf("  --xcom-defaults      Restore default access levels on Xcom-232i\n");
    printf("                       Uses parameter 5044 at address 501\n\n");
    printf("Default serial port: /dev/serial/by-path/platform-xhci-hcd.1.auto-usb-0:1.1.1:1.0-port0\n");
    printf("Default baud rate: 115200, even parity, 1 stop bit\n\n");
    printf("WARNING: These operations will reset devices to factory defaults!\n");
    printf("         All custom settings will be lost.\n");
}

int main(int argc, char *argv[]) {
    const char *port = "/dev/serial/by-path/platform-xhci-hcd.1.auto-usb-0:1.1.1:1.0-port0";
    int result = 0;
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Check if first argument is a serial port path
    int arg_offset = 1;
    if (argc > 2 && argv[1][0] == '/') {
        port = argv[1];
        arg_offset = 2;
    }
    
    if (argc <= arg_offset) {
        print_usage(argv[0]);
        return 1;
    }
    
    printf("Studer Device Reset Tool\n");
    printf("========================\n");
    printf("Serial port: %s\n", port);
    
    // Initialize the serial port
    if (serial_init(port, B115200, PARITY_EVEN, 1) != 0) {
        printf("ERROR: Failed to initialize serial port\n");
        return 1;
    }
    printf("Serial port initialized successfully\n");
    
    const char *option = argv[arg_offset];
    
    if (strcmp(option, "--system-reset") == 0) {
        printf("\n*** SYSTEM RESET - RESETTING ALL DEVICES ***\n");
        printf("This will reset ALL devices in your Studer system!\n");
        printf("Press Enter to continue or Ctrl+C to cancel...");
        getchar();
        
        result = send_reset_command(ADDR_XCOM232I, PARAM_XCOM_RESET_ALL_DEVICES);
        
    } else if (strcmp(option, "--xtender-all") == 0) {
        printf("\n*** XTENDER RESET - RESETTING ALL XTENDERS ***\n");
        printf("This will reset all Xtender inverters to factory defaults!\n");
        printf("Press Enter to continue or Ctrl+C to cancel...");
        getchar();
        
        result = send_reset_command(ADDR_ALL_XTENDERS, PARAM_XTENDER_RESTORE_FACTORY);
        
    } else if (strcmp(option, "--xtender") == 0) {
        if (argc <= arg_offset + 1) {
            printf("ERROR: --xtender requires an address (101-109)\n");
            return 1;
        }
        
        int addr = atoi(argv[arg_offset + 1]);
        if (addr < ADDR_XTENDER_START || addr > ADDR_XTENDER_END) {
            printf("ERROR: Invalid Xtender address %d (must be 101-109)\n", addr);
            return 1;
        }
        
        printf("\n*** XTENDER RESET - RESETTING XTENDER AT ADDRESS %d ***\n", addr);
        printf("This will reset the Xtender to factory defaults!\n");
        printf("Press Enter to continue or Ctrl+C to cancel...");
        getchar();
        
        result = send_reset_command(addr, PARAM_XTENDER_RESTORE_FACTORY);
        
    } else if (strcmp(option, "--xcom-defaults") == 0) {
        printf("\n*** XCOM DEFAULTS - RESTORING DEFAULT ACCESS LEVELS ***\n");
        printf("This will restore default access levels on Xcom-232i!\n");
        printf("Press Enter to continue or Ctrl+C to cancel...");
        getchar();
        
        result = send_reset_command(ADDR_XCOM232I, PARAM_XCOM_RESTORE_DEFAULTS);
        
    } else {
        printf("ERROR: Unknown option '%s'\n\n", option);
        print_usage(argv[0]);
        return 1;
    }
    
    if (result == 0) {
        printf("\n✓ Reset command completed successfully!\n");
        printf("  The device(s) should now restart with factory defaults.\n");
        printf("  Wait a few seconds for the system to reinitialize.\n");
    } else {
        printf("\n✗ Reset command FAILED!\n");
        printf("  Check the serial connection and try again.\n");
    }
    
    return result;
}
