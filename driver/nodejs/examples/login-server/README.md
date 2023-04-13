# nodejs/examples/login-server

This example shows how to implement the beginnings of an Econet fileserver by handling a login request (`*I AM` command).

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
Set local station number to 171
Listening for traffic...
```

Try executing something like this from a beeb:

```
*I AM 171 JPR93 MYPASS
```

The command should complete successfully and you will see the following on your local machine:

```
RxTransmitEvent 0.127 --> 0.171
[SCOUT] 00000000: ab 00 7f 00 80 99                                |«.....          |
[DATA]  00000000: ab 00 7f 00 90 00 01 02  04 49 20 61 6d 20 4a 50 |«........I am JP|
        00000010: 52 39 33 20 41 42 43 0d                          |R93 ABC.        |
Received OSCLI command="I am JPR93 ABC"
Successfully sent reply
```

Hit ctrl+c to exit gracefully.
