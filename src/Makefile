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

# Build rule for each app
$(BUILD_DIR)/%: %/main.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -DAPP_NAME=\""$*"\" $< -o $@

# Clean target
clean:
	rm -rf $(BUILD_DIR)