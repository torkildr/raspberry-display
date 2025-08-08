#!/bin/bash
# Ultra-minimal resource build for Pi Zero
# Optimizes every aspect to avoid overwhelming the device

set -e

BUILD_TYPE="${1:-release}"

echo "=== Pi Zero Minimal Resource Build ==="
echo "CPU: Single-core ARM1176JZF-S @ 1GHz"
echo "RAM: 512MB"
echo "Strategy: Minimize memory usage and compilation load"
echo

# Check available memory
echo "Current memory status:"
free -h

# Stop unnecessary services during build
echo "Stopping non-essential services to free memory..."
sudo systemctl stop bluetooth 2>/dev/null || true
sudo systemctl stop cups 2>/dev/null || true
sudo systemctl stop avahi-daemon 2>/dev/null || true

# Set up minimal swap if needed
if ! swapon --show | grep -q swap; then
    echo "Setting up minimal swap file..."
    sudo fallocate -l 512M /tmp/build-swap
    sudo chmod 600 /tmp/build-swap
    sudo mkswap /tmp/build-swap
    sudo swapon /tmp/build-swap
    CLEANUP_SWAP=1
fi

# Use ccache to speed up rebuilds
if ! command -v ccache >/dev/null 2>&1; then
    echo "Installing ccache for faster rebuilds..."
    sudo apt install -y ccache
fi

export CC="ccache gcc"
export CXX="ccache g++"

# Generate font first (lightweight)
echo "Generating font header..."
make font-generate

# Build using the lightweight Makefile approach
echo "Building with minimal resource Makefile..."
make -f Makefile.pi-zero build-sequential

echo "Build complete!"

# Cleanup
if [ "$CLEANUP_SWAP" = "1" ]; then
    echo "Cleaning up temporary swap..."
    sudo swapoff /tmp/build-swap
    sudo rm -f /tmp/build-swap
fi

echo "Restarting services..."
sudo systemctl start bluetooth 2>/dev/null || true
sudo systemctl start cups 2>/dev/null || true
sudo systemctl start avahi-daemon 2>/dev/null || true

echo "=== Build Summary ==="
echo "Built with minimal resource Makefile (non-MQTT clients):"
ls -la curses-client mock-curses-client 2>/dev/null || echo "Check for build errors above"
echo ""
echo "Note: For MQTT clients (raspberry-display-mqtt), use:"
echo "  ./build-pi-zero-efficient.sh  # Uses meson with Pi Zero optimizations"
echo ""
echo "Memory usage during build:"
free -h
