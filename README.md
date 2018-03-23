RaspberryPi Wall mounted display
================================
[![Build Status](https://travis-ci.org/torkildr/raspberry-display.svg?branch=master)](https://travis-ci.org/torkildr/raspberry-display)

## Overview
This project allows to control the Sure P4 32x8 displays. Though, it is written for this specific version, it should
pretty much work for all Holtek HT1632 based matrix displays.
 
The main goal is to give a generic, pluggable way to interface different kinds of applications with the display, without having to
bother with the driver stuff.

## Usage

The application is basically two parts.

`curses-client`, an interactive client, probably best used to test the hardware setup and display capabilites.

`fifo-client`, this client will listen for API commands on a fifo-queue. This queue is what you will interface with from
your own application.

Both of these applications also come in a `mock-` version. These will work without the physical hardware, and is perfect
for test setups or debugging the API usage.

## API

## Building
First install the system pre-requisites and the WiringPi library.

If you're on a debian system, like raspbian, this is simply
```bash
./install-prereqs-debian.sh
./install-wiringPi.sh
```

Once this is done, you should be able to build the project with
```bash
make
```

## Acknowledgements
This project is written by Torkild Retvedt, as a rewrite of a similar project for the [Arduino UNO](https://github.com/torkildr/display)

Much of the ht1632 driver is inspired by [this repo](https://github.com/DerBer/ht1632clib).

## License
This software is licensed under the MIT license, unless specified.

