#!/bin/bash

set -ex

sudo apt-get update
sudo apt-get install -y \
  libncurses5-dev \
  build-essential \
  libboost-date-time-dev \
  libboost-dev \
  libboost-program-options-dev \
  libboost-system-dev \
  libboost-test-dev \
  nlohmann-json3-dev \
  sudo \
  meson \
  cmake

