# nodejs/examples/login-server

This example shows how to implement the beginnings of an Econet fileserver by handling a login request (`*I AM` command).

## Hardware prerequisites

* Build/aquire hardware: https://github.com/jprayner/piconet/tree/main/board
* Install the latest firmware onto the board as desribed here: https://github.com/jprayner/piconet#getting-started

## Software prerequisites

* Install the latest version of NodeJS: https://docs.npmjs.com/downloading-and-installing-node-js-and-npm

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
Received OSCLI command="I am JPR93 MYPASS"
Successfully sent reply
```

Hit ctrl+c to exit gracefully.

