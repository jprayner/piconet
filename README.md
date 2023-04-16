# Piconet

Piconet allows modern computers (MacOS, PC and Linux) to talk to Acorn Econet networks using a board which interfaces the ADF10 Econet module (BBC Master/Archimedes Econet board) to a Raspberry Pi Pico. The Pico can then be connected to your machine via USB.

Piconet is theoretically capable of supporting anything you can dream up which runs over Econet: file transfer utilities, file servers, D&D servers etc.

![system-view](https://user-images.githubusercontent.com/909745/225333540-71af28cc-e700-44a8-87bd-7ba4ffddd420.png)

This project provides the purple boxes in the above diagram: the hardware design of the Piconet board, the firmware which executes on the Pico, and the drivers which talk to it.

## Related projects

* [ecoclient](https://github.com/jprayner/ecoclient) — a command-line utility for performing Econet operations

## State of development

This project is still under development. Currently:

* The board currently only exists in prototype form. A circuit diagram is available [here](https://github.com/jprayner/piconet/tree/main/board). It is hoped that a prebuilt board will be available soon.
* The only driver available today is based on Node.js. Contributions of drivers based on other languages are welcome (Python, Golang etc) — please get in touch if interested! It is also possible to write apps which communicate with the board directly over a serial connection. See the protocol documentation below.
* Most development has been carried out on a Mac, although Windows and Linux have been tested too.
* Interoperability testing has been carried out with Acorn Level 3 (BBC Micro) and Level 4 (Archimedes) fileservers only.

It's still early days - expect weirdness!

## Getting started

1. Acquire or [build the Piconet board](https://github.com/jprayner/piconet/blob/main/board/README.md)
2. Download the `.uf2` image from the [releases page](https://github.com/jprayner/piconet/releases)
3. Connect the Pico to your machine with a USB cable whilst holding down the button on the Pico board
4. The Pico should show as a storage device: copy the `.uf2` image to it
5. The Pico should reboot automatically when the copy is complete
6. Connect an ADF10 Econet module to the board and hook it up to your Econet network
7. Install an app such as [ecoclient](https://github.com/jprayner/ecoclient) or try one of the code examples provided with the [driver](https://github.com/jprayner/piconet/tree/main/driver/nodejs)

## Protocol overview

This section describes the serial protocol between the board and driver.

Piconet protocol overview:

* Serial, text based protocol
  - human-readable
  - base64 encoding used for binary data (a decent trade-off between CPU efficiency (i.e. effort to encode/decode) and informational efficiency (i.e. size of data to exchange vs. that transferred over the wire)
* _Commands_ are sent by host machine such as:
  - status command
  - configuration commands
  - transmit command (e.g. `TX`)
* _Events_ are sent by board e.g.:
  - `TX_RESULT` in response to `TX` commands
  - ...or asynchronously e.g. `ERROR`, `RX_xxx`, `MONITOR`
* One command/event per line
  - aids recovery from reconnection (firmware keeps on running whilst apps start/stop/error)
* Utilises semantic versioning
  - determines compatibility between firmware and drivers/apps
  - allows apps/driver to signal to user that firmware or driver/app needs to upgrade before things get weird

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
| `TEST`                | Used to test hardware (with the device disconnected from the Econet, and generally the ADF10 Econet module too). See the [Hardware testing](https://github.com/jprayner/piconet/tree/main/board#hardware-testing) section of the documentation.|

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

## Firmware

### Functional overview

![functional-blocks](https://user-images.githubusercontent.com/909745/231214910-3fbc5c10-7e8f-45a0-8739-1eb8ad7ed1c4.png)

* [Core 0](https://github.com/jprayner/piconet/blob/main/board/src/piconet.c) handles serial I/O, leaving Core 1 free for more time-sensitive tasks. It does the following:
  - commands received from the host over the serial interface are put onto the command FIFO queue
  - events received from Core 1 on the event FIFO queue are marshalled and sent on to the host
* [Core 1](https://github.com/jprayner/piconet/blob/main/board/src/piconet.c) does the following:
  - receives commands from the command FIFO
  - handles the broadcast, transmit and receive Econet primitives and services ADLC interrupts in the [econet.c](https://github.com/jprayner/piconet/blob/main/board/src/econet.c) module
  - generates the appropriate signals to read and write from ADLC registers in the [adlc.c](https://github.com/jprayner/piconet/blob/main/board/src/adlc.c) module
  - generates events and places them on the event FIFO
* The FIFO queues are used to synchronise communication between the two cores and to queue (the sometimes bursty) events coming out of core 1
* Shared Memory is used by a [buffer pool](https://github.com/jprayner/piconet/blob/main/board/src/buffer_pool.c) to hold data frames in shared memory
* The [PIO state machine](https://github.com/jprayner/piconet/blob/main/board/src/pinctl.pio) handles the time-critical signals `!CS` (a.k.a. `!ADLC`), `R!W` and the data bus
  - some information on signal timing [may be found here](https://github.com/jprayner/piconet/tree/main/board#adlc-signals--timing)

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

Run the following from the `board` directory of this project:

```
export PICO_SDK_PATH=...  # set to directory created in previous step
cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=Debug -S. -B./build
cmake --build ./build --config Debug --target piconet -j 12 --
```

A `piconet.uf2` for flashing should appear under the `build` subdirectory.


## Credits

Thanks to the following projects:

* Base64 encode/decode code taken from: https://github.com/mbrt/libb64
* ADF-10 hardware interfacing and Econet protocol insights: https://github.com/cr12925/PiEconetBridge
* More Econet protocol insights: https://github.com/stardot/ArduinoFilestore
* General discussion on BBC Micro and Econent topics: https://stardot.org.uk
