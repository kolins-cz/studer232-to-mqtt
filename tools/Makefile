CC := gcc
CFLAGS := -g -I..
LIBS :=

# Source files
TOOL_SRC := studer_reset.c
SCOM_OBJS := ../scomlib_extra/scomlib_extra.o ../scomlib_extra/scomlib_extra_errors.o \
             ../scomlib/scom_data_link.o ../scomlib/scom_property.o ../src/serial.o

# Output binary
TARGET := studer_reset

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(TOOL_SRC) $(SCOM_OBJS)
	$(CC) $(CFLAGS) $(TOOL_SRC) $(SCOM_OBJS) $(LIBS) -o $(TARGET)
	@echo ""
	@echo "✓ studer_reset tool built successfully!"
	@echo ""
	@echo "Usage examples:"
	@echo "  ./studer_reset --system-reset              # Reset entire system"
	@echo "  ./studer_reset --xtender-all               # Reset all Xtenders"
	@echo "  ./studer_reset --xtender 101               # Reset Xtender at addr 101"
	@echo "  ./studer_reset /dev/ttyUSB0 --system-reset # Use different port"
	@echo ""

# Build dependencies if needed
$(SCOM_OBJS): %.o: %.c
	$(MAKE) -C .. $(subst ../,,$@)

clean:
	rm -f $(TARGET)

help:
	@echo "Studer Reset Tool - Build Instructions"
	@echo ""
	@echo "Targets:"
	@echo "  make          Build the reset tool"
	@echo "  make clean    Remove built binary"
	@echo "  make help     Show this help"
	@echo ""
	@echo "Note: This tool requires the parent project to be built first."
	@echo "      Run 'make' in the parent directory if you get linking errors."
