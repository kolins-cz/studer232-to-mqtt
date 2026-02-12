//
//  UNIX Serial Port Interface
//
//  Created by K3A (www.k3a.me)
//  Released under MIT
//

#include "serial.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#define error_message(fmt, ...) fprintf(stderr, "[SERIAL ERROR] " fmt, ##__VA_ARGS__)

int serial_fd = 0;

#ifdef SERIAL_DEBUG
// Convert termios speed constant to actual baud rate for debug output
static int speed_to_baud(int speed) {
    switch(speed) {
        case B0: return 0;
        case B50: return 50;
        case B75: return 75;
        case B110: return 110;
        case B134: return 134;
        case B150: return 150;
        case B200: return 200;
        case B300: return 300;
        case B600: return 600;
        case B1200: return 1200;
        case B1800: return 1800;
        case B2400: return 2400;
        case B4800: return 4800;
        case B9600: return 9600;
        case B19200: return 19200;
        case B38400: return 38400;
        case B57600: return 57600;
        case B115200: return 115200;
        case B230400: return 230400;
        default: return -1;
    }
}
#endif

static int set_interface_attribs(int fd, int speed, serial_parity_t parity, int stop_bits)
{
    struct termios tio;

#ifdef SERIAL_DEBUG
    int baud = speed_to_baud(speed);
    SERIAL_DEBUG_PRINT("Setting interface attributes: %d baud, parity=%d, stop_bits=%d\n", 
                       baud, parity, stop_bits);
#endif

    bzero(&tio, sizeof(tio)); // clear struct for new port settings

    tio.c_cflag = speed | CS8 /* 8 data bits */ | CLOCAL /* Ignore modem control lines */ | CREAD /* Enable receiver */;

    if (stop_bits == 2) {
        tio.c_cflag |= CSTOPB;
    }

    if (parity > 0) {
        tio.c_cflag |= PARENB;
        if (parity & PARITY_ODD) {
            tio.c_cflag |= PARODD;
        }
    }

    // keep input, output and line flags empty
    tio.c_iflag = INPCK; // enable parity check
    tio.c_oflag = 0;
    tio.c_lflag = 0; // will be noncanonical mode (ICANON not set)

    // set blocking with timeout
    // VMIN = 0 and VTIME > 0: read() returns immediately when data arrives,
    // or after VTIME deciseconds if no data received.
    // NOTE: Timer resets on each received byte, so total frame time can exceed VTIME.
    // The serial_read() function loops to collect full frames.
    tio.c_cc[VMIN] = 0;   // minimum number of characters for noncanonical read
    tio.c_cc[VTIME] = 20; // 2 second timeout per read() call (in deciseconds)

    if (tcsetattr(fd, TCSANOW, &tio) != 0) {
        error_message("tcsetattr error %d: %s\n", errno, strerror(errno));
        return -1;
    }

    SERIAL_DEBUG_PRINT("Interface attributes set successfully\n");
    return 0;
}

int serial_init(const char *port_path, int speed, serial_parity_t parity, int stop_bits)
{
    int ret;

    SERIAL_DEBUG_PRINT("Initializing serial port: %s\n", port_path);

    serial_fd = open(port_path, O_RDWR | O_NOCTTY);
    if (serial_fd < 0) {
        error_message("error %d opening %s: %s\n", errno, port_path, strerror(errno));
        return serial_fd;
    }

    SERIAL_DEBUG_PRINT("Serial port opened successfully, fd=%d\n", serial_fd);

    // set speed and parity
    if ((ret = set_interface_attribs(serial_fd, speed, parity, stop_bits)) < 0) {
        return ret;
    }

    SERIAL_DEBUG_PRINT("Serial port initialized successfully\n");
    return 0;
}

// write to serial port size bytes from ptr
int serial_write(const void *ptr, unsigned size) {
    SERIAL_DEBUG_PRINT("Writing %u bytes to serial port\n", size);
    SERIAL_DEBUG_HEX("TX", ptr, size);
    
    int bytes_written = write(serial_fd, ptr, size);
    
    if (bytes_written < 0) {
        error_message("Write error %d: %s\n", errno, strerror(errno));
    } else if ((unsigned int)bytes_written != size) {
        SERIAL_DEBUG_PRINT("Warning: Only wrote %d of %u bytes\n", bytes_written, size);
    } else {
        SERIAL_DEBUG_PRINT("Successfully wrote %d bytes\n", bytes_written);
    }
    
    return bytes_written;
}

// read size bytes from serial into ptr buffer
int serial_read(void *ptr, unsigned size)
{
    unsigned char *buf = (unsigned char *)ptr;
    unsigned bts_read = 0;

    SERIAL_DEBUG_PRINT("Reading %u bytes from serial port\n", size);

    while (bts_read < size) {
        int ret = read(serial_fd, buf + bts_read, size - bts_read);

        if (ret < 0) {
            error_message("Read error %d: %s\n", errno, strerror(errno));
            return ret;
        } else if (ret == 0) {
            // timeout
            SERIAL_DEBUG_PRINT("Read timeout after %u bytes (expected %u)\n", bts_read, size);
            if (bts_read > 0) {
                SERIAL_DEBUG_HEX("RX (partial)", ptr, bts_read);
            }
            return bts_read;
        }

        SERIAL_DEBUG_PRINT("Read %d bytes (total: %u/%u)\n", ret, bts_read + ret, size);
        bts_read += ret;
    }

    SERIAL_DEBUG_PRINT("Successfully read %u bytes\n", bts_read);
    SERIAL_DEBUG_HEX("RX", ptr, bts_read);

    return bts_read;
}

// flush/clear serial input buffer
void serial_flush(void) {
    SERIAL_DEBUG_PRINT("Flushing serial input buffer\n");
    tcflush(serial_fd, TCIFLUSH);
}