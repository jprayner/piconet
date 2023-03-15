# Piconet board

This directory contains the firmware for Piconet which comprises ANSI C code for the Raspberry Pi Pico and accompanying PIO assembly language.

## Schematic

![Schematic_piconet_2023-02-15](https://user-images.githubusercontent.com/909745/219204768-f511ca68-83ad-4179-b935-38036b83f05f.png)

Currently this exists only as a big ole mess of wires on a breadboard. It is hoped that a prebuilt board will be available for purchase soon.

Before connecting the board to an Econet module, it is recommended that you validate connections carefully. A firmware image to help validate your build will be provided soon (using a logic analyser, oscilloscope or multimeter).

## Protocol overview

* Serial, text based protocol
  - human-readable
  - base64 encoding used for binary data (reasonably efficient in terms of CPU and size)
* Commands are sent by host machine
  - status command
  - configuration commands
  - transmit command
* Events are sent by board
  - in response to commands e.g. `TX_RESULT`
  - ...or asynchronously e.g. `ERROR`, `RX_xxx`, `MONITOR`
* One command/event per line
  - aids recovery from reconnection

### Operation modes

There are three modes of operation:

* `STOPPED` — no events are generated in response to network traffic, allowing the client to initialise board. The board starts in this mode.
* `MONITORING` — the board generates an event for every frame received, regardless of its source or destination (promiscuous mode).
* `LISTENING` — the board generates events for broadcast frames or frames targetting the configured local Econet station number

### Commands

STATUS
RESTART
SET_MODE
SET_STATION
TX

### Events

STATUS
TX_RESULT
REPLY_RESULT
ERROR
MONITOR
RX_BROADCAST
RX_IMMEDIATE
RX_TRANSMIT


## Building firmware from source

### Setting up Pico build tooling

This assumes you are building on Linux - see ![Pi Pico C SDK documentation](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf) for other platforms.

Install dependencies (Ubuntu/Debian derivatives)

```
apt install -y git build-essential cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib python3
git clone https://github.com/raspberrypi/pico-sdk.git --branch master
cd pico-sdk
git submodule update --init
```

### Building

Run the following from this directory (`board`):

```
export PICO_SDK_PATH=...  # set to directory created in previous step
cmake .
cmake --build ./
```

A `piconet.uf2` for flashing should appear under the `build` subdirectory.
