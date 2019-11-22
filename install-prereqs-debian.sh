#!/bin/bash

set -ex

sudo apt-get update
sudo apt-get install -y \
  libncurses5-dev \
  build-essential \
  libboost-dev \
  libboost-system-dev \
  sudo

