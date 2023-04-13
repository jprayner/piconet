# NodeJS driver for Piconet

Piconet allows modern computers (MacOS, PC and Linux) to talk to Acorn Econet networks using a board which interfaces the ADF10 Econet module (BBC Master/Archimedes Econet board) to a Raspberry Pi Pico. The Pico can then be connected to your machine via USB.

This library simplifies the use of Piconet for NodeJS apps written in JavaScript or TypeScript. It handles serial communication with the board and provides facilities to make life easier when dealing with asynchronous events. That said, it is actually a really thin layer over the [board's own protocol](https://github.com/jprayner/piconet#protocol-overview).

## Pre-requisites

Piconet works with JavaScript or TypeScript NodeJS apps with:

- Node version 14 and above
- NPM version 6 and above

## Installing

Add the piconet driver to your project like this:

```
npm i --save @jprayner/piconet-nodejs
```

## Initialising the board

The board boots into the `STOP` state and won't generate any events in response to network traffic until it is put into either the `LISTEN` or `MONITOR` mode.

### Listen mode

This is the normal Econet station operating mode where events are generated in response to broadcast frames or other frames targetting the local machine.

To configure the board for `LISTEN` mode:

```
import { driver } from '@jprayner/piconet-nodejs';

...

// establish serial comms with the board
await driver.connect();

// configure local Econet station number (1-254)
await driver.setEconetStation(33);

// tell board to start listening for Econet frames
await driver.setMode('LISTEN');

... do stuff ...

// close the serial connection gracefully
await driver.close();
```

### Monitor mode

In this mode, the board generates an event for every frame received, regardless of its source or destination (promiscuous mode). This is useful for capturing traffic between other stations like the BBC Micro's NETMON utility.

To configure the board for `MONITOR` mode:

```
import { driver } from '@jprayner/piconet-nodejs';

...

// establish serial comms with the board
await driver.connect();

// note: not necessary to set Econet station here

// tell board to start listening for any Econet frames
await driver.setMode('MONITOR');

... do stuff ...

// close the serial connection gracefully
await driver.close();
```

### Connection fault-finding

By default, the `connect` function will attempt to autodetect the correct serial device for the Pi Pico using the USB vendor ID and product ID. If this doesn't work — for example, you have multiple Picos attached to your machine — then you can explicitly pass it a device string such as `/dev/ttyACM0` for a Linux serial device or `COM4` for a Windows serial port.

The `connect` function will throw an error if it fails to communicate with the board. Some common reasons for this:

- Piconet board is not connected
- Pico device has not been flashed with the Piconet firmware — see [Getting started](https://github.com/jprayner/piconet#getting-started)
- The firmware version loaded into the Pico is not compatible with device driver version (i.e. major/minor version parts don't match) - update the firmware and/or driver version to make them match

The error description should provide additional clues.

## Handling events

Events are generated in response to network traffic or as a result of certain calls made to the driver.

There are three ways to receive events in your application: `waitForEvent`, `EventQueue` or a simple event listener. The best option will depend on what you're trying to achieve but `waitForEvent` is the simplest to use and is usually adequate.

The first two methods require you to supply an [EventMatcher](https://github.com/jprayner/piconet/blob/main/driver/nodejs/docs/modules/driver.md#eventmatcher) to specify what type of events you are interested in.

To create a matcher which matches all events:

```
// JavaScript
const matcher = (event) => true;

// TypeScript
const matcher = (event: EconetEvent) => true;
```

Matchers can be as complex as you like. Perhaps you are only interested in `TRANSMIT` operations received from a particular station with a particular control byte and port:

```
  const matcher = (event : EconetEvent) => { // leave off ": EconetEvent" if you're using JavaScript rather than TypeScript
    const result =
      event instanceof RxTransmitEvent &&
      event.scoutFrame.length >= 6 &&
      event.scoutFrame[2] === sourceStation &&
      event.scoutFrame[3] === sourceNetwork &&
      event.scoutFrame[4] === controlByte &&
      event.scoutFrame[5] === port
    return result;
  };
```

Note that, because events are defined as classes, you can use the `instanceof` operator to differentiate them at runtime.

**Tip:** received data events (subclasses of [RxDataEvent](https://github.com/jprayner/piconet/blob/main/driver/nodejs/docs/classes/index.RxDataEvent.md)) provide hex dumps of scout/data frames, as appropriate, in their `toString()` implementations. So doing `console.log(event.toString());` can be a useful debugging tool!

### waitForEvent

[waitForEvent](https://github.com/jprayner/piconet/blob/main/driver/nodejs/docs/modules/driver.md#waitforevent) is an async function (returns a promise of an `EconetEvent`) which waits for a certain period of time for a matching event to occur. It is simple to use and perfect for simple request/response type operations.

```
import { driver } from '@jprayner/piconet-nodejs';

... initialise board etc. ...

// wait for upto 1s for a matching event
const event = await waitForEvent(matcher, 1000);
```

### EventQueue

`EventQueue`s store matching events in order. An advantage over `waitForEvent` is that you won't miss events that occur whilst your app is busy doing other things. This is useful for situations where frames arrive quickly such as Econet fileserver `LOAD` operations.

You can wait for events to appear on a queue using [eventQueueWait](https://github.com/jprayner/piconet/blob/main/driver/nodejs/docs/modules/driver.md#eventqueuewait). You can also do a quick synchronous check for new events using [eventQueueShift](https://github.com/jprayner/piconet/blob/main/driver/nodejs/docs/modules/driver.md#eventqueueshift).

Here's an example:

```
import { driver, RxTransmitEvent } from '@jprayner/piconet-nodejs';

... initialise board etc. ...

const queue = driver.eventQueueCreate(
  (event) => event instanceof RxTransmitEvent
);

while (!done) {
  const rxTransmitEvent = await driver.eventQueueWait(queue, 1000);

  // do some processing
}

driver.eventQueueDestroy(queue);
```

Note that if you have multiple queues which match a particular event, that event will be appended to each queue.

### Simple listener

In most situations the previous two methods are the most convenient way to receive events. However, if you have special requirements (or want to avoid Promises and async/await) then the option of using a simple listener callback is available to you too.

```
import { driver, EconetEvent, ErrorEvent, RxDataEvent } from '@jprayner/piconet-nodejs';

... initialise board etc. ...

const listener = (event: EconetEvent) => { // leave off ": EconetEvent" if you're using JS
  if (event instanceof ErrorEvent) {
    console.error(`ERROR: ${event.description}`);
  } else if (event instanceof RxDataEvent) {
    console.log(`Received ${event.constructor.name}`);
    console.log(event.toString());
  }
};

driver.addListener(listener);

... do stuff ...

driver.removeListener(listener);

... close driver etc. ...
```

## Econet essentials

Econet network interfaces (like the ADF10 card) are based upon the MC68B54 Advanced Data Link Controller (ADLC). This handles the business of serialising data onto the wire, performing CRC checks etc. Layered upon this is the Econet protocol which defines the format of discrete messages sent over the network (known as "frames") via the ADLC.

Rather than IP addresses, Econet uses two 8-bit numbers to uniquely identify a device on the network: the station and network numbers. The station number is in the range 1-254. Station 254 is normally reserved for the default fileserver and station 255 is used to indicate a broadcast frame, rather than a actual machine. A network number of zero indicates the local network, with other values used to address machines on other, "bridged" networks.

### The four-way handshake

The Econet `TRANSMIT` operation is a key building block for a wide variety of Econet communications. During a `TRANSMIT` operation, four frames are exchanged in a sequence known as "the four-way handshake":

1. the transmitting station sends a "scout" frame to the recipient, including a control byte (to indicate the type of traffic) and a port number
2. the recipient sends an acknowledgement frame back to the transmitting station (assuming it is listening and knows how to handle the control byte/port combination)
3. the transmitting station sends a "data" frame containing the body of the message
4. the recipient sends another acknowledgement frame

This provides a reasonably robust mechanism for sending data and is handled by the driver's [transmit](https://github.com/jprayner/piconet/blob/main/driver/nodejs/docs/modules/driver.md#transmit) function. A failure of the caller to receive an "ACK" in steps [2] or [3] will often result in a limited number of back-offs and retries, although such mechanisms are application-dependant and out-of-scope here.

### Scout frame format

A scout frame consists of the following octets (bytes):

1. Destination station number
2. Destination network number
3. Source station number
4. Source network number
5. Control byte
6. Port number

Again, the values of [5] and [6] depend on the type of traffic. In some exceptional cases (such as the `NOTIFY` operation), there may be a small amount of extra data following the port number.

### Acknowledgement frame format

An acknowledgement frame is dead simple (note that destination in this case is the sender of the scout frame, because the frame is being sent in the opposite direction; in other words, source and destination are swapped):

1. Destination station number
2. Destination network number
3. Source station number
4. Source network number

The same format is used for acknowledging both scout and data frames.

### Data frame format

The first four octets of the data frame are:

1. Destination station number
2. Destination network number
3. Source station number
4. Source network number

The remainder of the frame is dependent on the operation being performed. Given the memory limitations of devices at the time, data frames are typically a couple of kilobytes or less in length. Some operations such as `PEEK` or `POKE` can generate much larger data frames (e.g. 20kB for a screen grab, depending on video mode).

### Broadcast frames

TODO

### Further reading

The following are recommended reading to learn more about Econet programming and file server protocols:

- Acorn Econet Advanced User Guide
- The Econet System User Guide
- Source code for the [ecoclient](https://github.com/jprayner/ecoclient) project

## Sending data

The [transmit](https://github.com/jprayner/piconet/blob/main/driver/nodejs/docs/modules/driver.md#transmit) function implements the Econet `TRANSMIT` operation by carrying out the sender's role in the four-way handshake.
