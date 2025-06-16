# List of app directories
APPS := CMD XCP_CMD XCP_DAQ

# Output directory
BUILD_DIR := build

# Compiler and flags
CC := gcc
CFLAGS := -Wall -O2 -Icommon -g

# Find all .c files in common directory
COMMON_SRCS := $(shell find common -name '*.c')

# Default target
all: $(patsubst %, $(BUILD_DIR)/%, $(APPS))

# Per-app rules
$(BUILD_DIR)/%: 
	@mkdir -p $(BUILD_DIR)
	SRCS="$$(find $* -name '*.c')"; \
	$(CC) $(CFLAGS) -I$* $$SRCS $(COMMON_SRCS) -o $@ -DAPP_NAME=\""$*"\" 

# Add explicit dependencies for each app
$(foreach app,$(APPS),$(eval $(BUILD_DIR)/$(app): $(shell find $(app) -name '*.c') $(COMMON_SRCS)))

# Clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
