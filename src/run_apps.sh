#!/bin/bash

set -e  # Exit on first failure

APPS=("CMD" "XCP_CMD" "XCP_DAQ")
BUILD_DIR="build"

for APP in "${APPS[@]}"; do
    BIN="$BUILD_DIR/$APP"
    if [[ -x "$BIN" ]]; then
        echo "=== Running $APP ==="
        ("$BIN")
        echo
    else
        echo "!! Skipping $APP: $BIN not found or not executable"
    fi
done