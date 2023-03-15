# Piconet board

This directory contains the firmware for Piconet which comprises ANSI C code for the Raspberry Pi Pico and accompanying PIO assembly language.

## Schematic

![Schematic_piconet_2023-02-15](https://user-images.githubusercontent.com/909745/219204768-f511ca68-83ad-4179-b935-38036b83f05f.png)

Currently this exists only as a big ole mess of wires on a breadboard. It is hoped that a prebuilt board will be available for purchase soon.

Before connecting the board to an Econet module, it is recommended that you validate connections carefully. A firmware image to help validate your build will be provided soon (using a logic analyser, oscilloscope or multimeter).

## Protocol overview

* Serial, text based protocol
  - human-readable
  - base64 encoding used for binary data (reasonably efficient in terms of CPU usage and data transferred)
* Commands are sent by host machine
  - status command
  - configuration commands
  - transmit command
* Events are sent by board
  - in response to commands e.g. `TX_RESULT`
  - ...or asynchronously e.g. `ERROR`, `RX_xxx`, `MONITOR`
* One command/event per line
  - aids recovery from reconnection
* Semantic versioning
  - determines compatibility between firmware, drivers and apps

### Operation modes

There are three modes of operation:

| Mode          | Description |
| ------------  | ----------- |
| `STOP`        | The board starts in this mode. No events are generated in response to network traffic, allowing the client to initialise board. |
| `LISTEN`      | The normal Econet station operating mode. The board generates events for broadcast frames or frames targeting the configured local Econet station number (see `SET_STATION` command). |
| `MONITORING`  | The board generates an event for every frame received, regardless of its source or destination (promiscuous mode). Useful for capturing traffic between other stations like the BBC `NETMON` utility. |


### Commands

| Command              | Description |
| -------              | --- |
| `STATUS`             | Requests status report from board. This causes a `STATUS` event to be generated in reply.|
| `RESTART`            | Reinitialises ADLC by forcing low `!RST` signal (not normally required). |
| `SET_MODE ${mode}`   | See _Operating modes_ section above. The `mode` parameter is a decimal integer where `0` == `STOP`, `1` == `LISTEN` and `2` == `MONITOR`.
| `SET_STATION ${num}` | Sets the Econet station number for the board so that `RX_xxx` events are fired in response to frames relevant to this station. `num` should be specified as a decimal integer in range 1-254 (254 is usually reserved for an Econet fileserver). |
| `TX ${station} ${network} ${controlByte} ${port} ${data}` | Sends an Econet packet (through the exchange of a sequence of frames between client and server which consitute the "four-way handshake": scout, scout ack, data, ack). All parameters are decimal integers except for `data` which is base64 encoded. `station` and `network` identify the destination station; `controlByte` and `port` help the recipient classify the incoming packet; `data` is the body of the message. A `TX_RESULT` event is generated in response to this command.

TODO: introduce a `BCAST` command for sending broadcast packets.

### Events

| Command                 | Description |
| -------                 | --- |
| `STATUS ${ver.major}.${ver.minor}.${ver.patch} ${station} ${sr1} ${mode}` | Status of board, reported in response to a `STATUS` command. Version parts are decimal and follow semantic versioning 2.0.0 guidelines (for determining driver compatibility). `station` is the configured local Econet station number (change this using the `SET_STATION` command). `sr1` gives the current value of the ADLC's status register 1 (useful for detecting Econet clock/connection status). `mode` reports the current operating mode (see above) `0` == `STOP`, `1` == `LISTEN` and `2` == `MONITOR`.
| `ERROR ${description}`  | May be fired at any time by the firmware to describe a problem. `description` is a human-readable string.
| `MONITOR ${frame}`      | Fired each time a frame is successfully captured whilst in the Monitor operating mode. `frame` is base64 encoded.
| `RX_BROADCAST ${frame}` | Fired when a broadcast frame is received whilst in the Listen operating mode. `frame` is base64 encoded.
| `RX_IMMEDIATE ${scout} ${data}` | Fired when an immediate operation is received whilst in the Listen operating mode. Both `scout` and `data` are base64 encoded.
| `RX_TRANSMIT ${replyId} ${scout} ${data}` | Fired when a transmit packet is received (i.e. a non-broadcast, non-immediate packet, utilising a four-way handshake) whilst in the Listen operating mode. `replyId` should be ignored right now. Both `scout` and `data` are base64 encoded.
| `TX_RESULT ${result}` | Indicates the result of a `TX` command. The value `OK` indicates a successful transmission. Any other value describes the reason for the failure. See below for possible values.

### TX_RESULT values

| Value | Description |
| ----  | ----|
| `OK` | Packet was successfully sent and acknowledged
| `UNINITIALISED` | Firmware issue — should never happen
| `OVERFLOW` | Indicates that size of message exceeds `TX_DATA_BUFFER_SZ`
| `UNDERRUN` | Indicates that firmware is failing to keep up with ADLC when sending data
| `LINE_JAMMED` | Suggests that a station is 'flag-filling' and preventing transmission
| `NO_SCOUT_ACK` | Remote station failed to acknowledge scout frame (disconnected or not listening on port?) — consider retrying
| `NO_DATA_ACK` | Remote station failed to acknowledge data frame — consider retrying
| `TIMEOUT` | Other timeout condition e.g. in communication with ADLC
| `MISC` | Logic error e.g. in protocol decode
| `UNEXPECTED` | Firmware issue — should never happen


## Building firmware from source

### Setting up Pico build tooling

This assumes you are building on Linux - see [Pi Pico C SDK documentation](https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf) for other platforms.

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
