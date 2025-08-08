#!/bin/bash
# Build on a more powerful Pi (Pi 3/4) then copy to Pi Zero
# This avoids resource issues while maintaining binary compatibility

set -e

BUILDER_PI="${1:-}"
BUILD_TYPE="${2:-release}"

if [ -z "$BUILDER_PI" ]; then
    echo "Usage: $0 <builder-pi-ip> [build-type]"
    echo "Example: $0 192.168.1.200 release"
    echo ""
    echo "Builder Pi should be a Pi 3/4 with more resources"
    echo "but same ARM architecture for compatibility"
    exit 1
fi

echo "Building on more powerful Pi: $BUILDER_PI"

# Copy source code to builder Pi
echo "Copying source code to builder Pi..."
rsync -avz --exclude=build* --exclude=debug* . pi@$BUILDER_PI:~/raspberry-display-build/

# Build on the more powerful Pi
echo "Building on $BUILDER_PI..."
ssh pi@$BUILDER_PI "
    cd ~/raspberry-display-build
    
    # Install dependencies if needed
    sudo apt update
    sudo apt install -y libncurses5-dev libboost-system-dev wiringpi-dev build-essential meson ninja-build
    
    # Generate font
    make font-generate
    
    # Build with meson (Pi 3/4 can handle this easily)
    meson setup build-remote -Dbuildtype=$BUILD_TYPE --prefix=/usr
    meson compile -C build-remote
    
    echo 'Build complete on builder Pi'
    ls -la build-remote/
"

# Copy binaries back to Pi Zero
echo "Copying binaries back to Pi Zero..."
mkdir -p binaries-from-builder
rsync -avz pi@$BUILDER_PI:~/raspberry-display-build/build-remote/ binaries-from-builder/

echo "Binaries copied to binaries-from-builder/"
echo "To install on Pi Zero:"
echo "  sudo cp binaries-from-builder/raspberry-display-mqtt /usr/bin/"
echo "  sudo systemctl daemon-reload"
