CC := gcc
CFLAGS := -g
LIBS := -lmosquitto

# Enable serial debug output by setting DEBUG=1 on command line: make DEBUG=1
ifdef DEBUG
    CFLAGS += -DSERIAL_DEBUG
endif

OBJECTS := scomlib_extra/scomlib_extra.o scomlib_extra/scomlib_extra_errors.o scomlib/scom_data_link.o scomlib/scom_property.o src/serial.o src/main.o

.PHONY: all clean debug

all: bin/studer232-to-mqtt

debug:
	@echo "Building with serial debug output enabled..."
	$(MAKE) DEBUG=1 all

clean:
	rm -f $(OBJECTS) bin/studer232-to-mqtt

bin/studer232-to-mqtt: $(OBJECTS)
	mkdir -p ../bin
	$(CC) $(OBJECTS) $(LIBS) -o bin/studer232-to-mqtt

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<