CC := gcc
CFLAGS_COMMON := -Wall -Wextra -pthread
CFLAGS_NORMAL := $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG := $(CFLAGS_COMMON) -g -DSERIAL_DEBUG
LIBS := -lmosquitto -lpthread -ljson-c

# Source files - libraries and main sources
LIB_SOURCES := scomlib_extra/scomlib_extra.c scomlib_extra/scomlib_extra_errors.c \
               scomlib/scom_data_link.c scomlib/scom_property.c \
               src/serial.c

MAIN_SOURCE := src/main.c

# Header dependencies
HEADERS := src/main.h src/serial.h \
           scomlib_extra/scomlib_extra.h \
           scomlib/scom_data_link.h scomlib/scom_property.h scomlib/scom_port_c99.h

# Object files - shared libs + per-build main
LIB_OBJECTS := $(LIB_SOURCES:%.c=build/lib/%.o)
MAIN_NORMAL := build/normal/src/main.o
MAIN_DEBUG := build/debug/src/main.o

.PHONY: all clean debug install

all: bin/studer232-to-mqtt bin/studer232-to-mqtt-debug

debug: bin/studer232-to-mqtt-debug

clean:
	rm -rf build bin/studer232-to-mqtt bin/studer232-to-mqtt-debug

# Normal release binary
bin/studer232-to-mqtt: $(LIB_OBJECTS) $(MAIN_NORMAL)
	@mkdir -p bin
	$(CC) $(CFLAGS_NORMAL) $(LIB_OBJECTS) $(MAIN_NORMAL) $(LIBS) -o $@

# Debug binary
bin/studer232-to-mqtt-debug: $(LIB_OBJECTS) $(MAIN_DEBUG)
	@mkdir -p bin
	$(CC) $(CFLAGS_DEBUG) $(LIB_OBJECTS) $(MAIN_DEBUG) $(LIBS) -o $@

# Shared library object files (optimized)
build/lib/%.o: %.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_NORMAL) -c -o $@ $<

# Normal main.o
build/normal/src/main.o: $(MAIN_SOURCE) $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_NORMAL) -c -o $@ $<

# Debug main.o (with SERIAL_DEBUG and debug symbols)
build/debug/src/main.o: $(MAIN_SOURCE) $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_DEBUG) -c -o $@ $<