# Piconet board

This directory contains the firmware for Piconet which comprises ANSI C code for the Raspberry Pi Pico and accompanying PIO assembly language.

## Hardware schematic

The Piconet board is simply a couple of SN74LVC245 buffers doing conversion between the 3.3V CMOS levels of the Pico and the 5V TTL levels of ADF10 module. SN74HCT245 buffers work fine too. The ADF10 is powered from the 5V of the USB connection, no need for a separate power supply.

![Schematic_piconet_2023-02-15](https://user-images.githubusercontent.com/909745/219204768-f511ca68-83ad-4179-b935-38036b83f05f.png)

## Prototype

It is hoped that a prebuilt board will be available for purchase soon but currently this exists only as a big ole mess of wires on a breadboard:

![breadboard](https://user-images.githubusercontent.com/909745/229339862-0e919559-c6a4-44d5-bc3d-d3d99635fa6b.jpg)

I knocked up a separate board for the DIN connector:

![DIN5-conn-front](https://user-images.githubusercontent.com/909745/229340063-d8e43cc3-caf1-4c82-9b0a-10b5a3b97085.jpg)

![DIN5-conn-back](https://user-images.githubusercontent.com/909745/229340078-ed838d5a-56e3-4ab2-a40a-79b63b046099.jpg)

### Hardware testing

Before connecting the board to the USB of your computer, use a multimeter to make sure that you don't have a short between VCC and ground.

Before connecting the board to an Econet module, it is recommended that you validate connections carefully to avoid frustration and potential damage to your Econet module! 

Flash the Pico with latest firmware as described in the [Getting started](https://github.com/jprayner/piconet#getting-started) guide.

Connect a logic analyser to the 5V side of the buffers instead of the ADF10. Open a serial connection to the Pico and execute the `TEST` command. You should see the message:

```
TESTING BOARD (REBOOT TO STOP)
```

The board will cycle through the following:

1. For each of the register addresses a0:a1 (i.e. 4 values), perform 8 writes with a single data bit d0:d7 high at a time
2. Assert !RST
3. Wait approx. 100ms
4. Release !RST
5. Wait approx. 100ms

Here is a logic analyser trace with steps [1] and [2] visible:

![newboard2](https://user-images.githubusercontent.com/909745/229341287-9c1a31fa-7497-4a26-b3a0-b025fde5e477.png)

Once you've satisfied yourself that each of the signals is doing what it should, check you have 5V across pins 16 and 17 of the header, disconnect the logic analyser and plug in your ADF10 module.

## ADLC signals & timing

The ADLC is clocked at 2 MHz clock from GPIO 21. The time-sensitive business of reading/writing data from the bus, and of generating the `!CS`, `R!W` signals, is handled using the Pico's — amazing! — PIO (Programmable Input/Output) state machine feature. This is defined in [pinctl.pio](https://github.com/jprayner/piconet/blob/main/board/src/pinctl.pio). Initialisation of the PIO and communication with it is handled by the [adlc.c](https://github.com/jprayner/piconet/blob/main/board/src/adlc.c) module.

Here's an example of a write of 0x01 followed by a write of 0x02 to control register 1 (a0 and a1 both zero):

![newboard1](https://user-images.githubusercontent.com/909745/229342641-037f1345-7197-4bba-b61d-28f8250de281.png)

Note that `d0:d7`, `a0:a1` and `R!W` are set-up — and `!ADLC` asserted to perform an operation — in good time before the clock's rising edge. Hold times are observed before the signals are released after the clock falls.

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
| `BCAST ${data}`       | The single `data` parameter is base64 encoded. This shall be sent with destination station/network octets both set to `0xff` and the configured econet station number as the source address. A `TX_RESULT` event is generated in response to this command. |
| `TEST`                | Used to test hardware **with ADF10 Econet module disconnected**. See "Prototype" section above. |

### Events

| Event                 | Description |
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
cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -S. -B./build
cmake --build ./build --config Debug --target piconet -j 12 --
```

A `piconet.uf2` for flashing should appear under the `build` subdirectory.
