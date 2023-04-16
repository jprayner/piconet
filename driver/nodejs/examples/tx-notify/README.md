# nodejs/examples/tx-notify

This example shows how to send a text message using a `NOTIFY` immediate operation.

See also the [ecoclient](https://github.com/jprayner/ecoclient) project for more examples of implementing the Econet
fileserver protocol.

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
Sending message to station 127 from station 171...
Message sent, closing connection to board...
```

If you have trouble during the board connection step, refer to the driver
[fault finding](https://github.com/jprayner/piconet/tree/main/driver/nodejs#fault-finding) section.
