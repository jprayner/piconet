# nodejs/examples/monitor

This example shows how to implement a NodeJS-based equivalent of the Acorn NETMON utility.

## Hardware prerequisites

- Build/acquire hardware: https://github.com/jprayner/piconet/tree/main/board
- Install the latest firmware onto the board as desribed here: https://github.com/jprayner/piconet#getting-started

## Software prerequisites

- Install the latest version of NodeJS: https://docs.npmjs.com/downloading-and-installing-node-js-and-npm

## Install dependencies & run

Execute the following commands from this directory:

```
npm install
npm start
```

You should see output like the following:

```
Connecting to board...
Connected
Listening for traffic...
```

Try executing something like this from a beeb:

```
*I AM JPR93 MYPASS
```

You should see the following on your local machine:

```
MonitorEvent 0.127 --> 0.1
        00000000: 01 00 7f 00 88 00 00 db  00 00                   |.......Û..      |
MonitorEvent 0.1 --> 0.127
        00000000: 7f 00 01 00 05 00 25 04                          |......%.        |
MonitorEvent 0.127 --> 0.1
        00000000: 01 00 7f 00 80 99                                |......          |
MonitorEvent 0.1 --> 0.127
        00000000: 7f 00 01 00                                      |....            |
MonitorEvent 0.127 --> 0.1
        00000000: 01 00 7f 00 90 00 01 02  04 49 20 41 4d 20 4a 50 |.........I AM JP|
        00000010: 52 39 33 20 4d 59 50 41  53 53 0d                |R93 MYPASS.     |
MonitorEvent 0.1 --> 0.127
        00000000: 7f 00 01 00                                      |....            |
MonitorEvent 0.1 --> 0.127
        00000000: 7f 00 01 00 80 90                                |......          |
MonitorEvent 0.127 --> 0.1
        00000000: 01 00 7f 00                                      |....            |
MonitorEvent 0.1 --> 0.127
        00000000: 7f 00 01 00 05 00 01 02  04 00                   |..........      |
MonitorEvent 0.127 --> 0.1
        00000000: 01 00 7f 00                                      |....            |
```

Hit ctrl+c to exit gracefully.

If you have trouble during the board connection step, refer to the driver
[fault finding](https://github.com/jprayner/piconet/tree/main/driver/nodejs#fault-finding) section.
