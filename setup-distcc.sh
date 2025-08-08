#!/bin/bash
# Set up distributed compilation using distcc
# This offloads compilation to a more powerful machine

set -e

HELPER_HOST="${1:-}"

if [ -z "$HELPER_HOST" ]; then
    echo "Usage: $0 <helper-host-ip>"
    echo "Example: $0 192.168.1.100"
    echo ""
    echo "The helper host should have distcc installed and configured"
    echo "to accept connections from this Pi Zero."
    exit 1
fi

echo "Setting up distcc for distributed compilation..."

# Install distcc on Pi Zero
sudo apt update
sudo apt install -y distcc

# Configure distcc to use helper host
echo "Configuring distcc to use helper host: $HELPER_HOST"
export DISTCC_HOSTS="$HELPER_HOST"
echo "export DISTCC_HOSTS=\"$HELPER_HOST\"" >> ~/.bashrc

# Set up compiler wrappers
sudo mkdir -p /usr/lib/distcc
sudo ln -sf /usr/bin/distcc /usr/lib/distcc/gcc
sudo ln -sf /usr/bin/distcc /usr/lib/distcc/g++

# Add distcc to PATH
export PATH="/usr/lib/distcc:$PATH"
echo "export PATH=\"/usr/lib/distcc:\$PATH\"" >> ~/.bashrc

echo "Distcc setup complete!"
echo ""
echo "On your helper host ($HELPER_HOST), run:"
echo "  sudo apt install distcc"
echo "  sudo systemctl enable distccd"
echo "  sudo systemctl start distccd"
echo ""
echo "Then build with:"
echo "  make -j2 CC=distcc CXX=distcc"
