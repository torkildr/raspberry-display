#!/bin/bash

set -xe

#git clone git://git.drogon.net/wiringPi

# drogon.net seems out of commission, use unofficial mirror
git clone https://github.com/torkildr/wiringPi

cd wiringPi
./build

