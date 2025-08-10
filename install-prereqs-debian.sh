#!/bin/bash

set -ex

sudo apt-get update
sudo apt-get install -y \
  libncurses5-dev \
  build-essential \
  nlohmann-json3-dev \
  sudo \
  libmosquitto-dev \
  mosquitto \
  mosquitto-clients
