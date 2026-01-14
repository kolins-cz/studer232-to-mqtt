#include <stdbool.h>

// Debug macros for serial communication
#ifdef SERIAL_DEBUG
    #define SERIAL_DEBUG_PRINT(fmt, ...) fprintf(stderr, "[SERIAL DEBUG] " fmt, ##__VA_ARGS__)
    #define SERIAL_DEBUG_HEX(prefix, data, len) do { \
        fprintf(stderr, "[SERIAL DEBUG] %s: ", prefix); \
        for (size_t i = 0; i < (len); i++) { \
            fprintf(stderr, "%02X ", ((unsigned char*)(data))[i]); \
        } \
        fprintf(stderr, "\n"); \
    } while(0)
#else
    #define SERIAL_DEBUG_PRINT(fmt, ...)
    #define SERIAL_DEBUG_HEX(prefix, data, len)
#endif

typedef enum {
    PARITY_NONE = 0,
    PARITY_EVEN = 1,
    PARITY_ODD = (1 << 1),
} serial_parity_t;

// initialize the serial port
int serial_init(const char *port_path, int speed, serial_parity_t parity, int stop_bits);

// write to serial port size bytes from ptr
int serial_write(const void *ptr, unsigned size);

// read size bytes from serial into ptr buffer
int serial_read(void *ptr, unsigned size);