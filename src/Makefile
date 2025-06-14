# List of app directories
APPS := CMD XCP_CMD XCP_DAQ

# Output directory
BUILD_DIR := build

# Compiler and flags
CC := gcc
CFLAGS := -Wall -O2

# Define output binaries
BINARIES := $(patsubst %, $(BUILD_DIR)/%, $(APPS))

# Default target: build all
all: $(BINARIES)

# Pattern rule to build each app
$(BUILD_DIR)/%: 
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$* $$(find $* -name '*.c') -o $@ -DAPP_NAME=\""$*"\" 

# Clean target
clean:
	rm -rf $(BUILD_DIR)