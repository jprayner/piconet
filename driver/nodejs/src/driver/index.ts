import config from '../config';
import { PKG_VERSION } from './version';
import { StatusEvent } from '../types/statusEvent';
import { EconetEvent } from '../types/econetEvent';
import { TxResultEvent } from '../types/txResultEvent';
import { parseStatusEvent } from '../parser/statusParser';
import { parseMonitorEvent } from '../parser/monitorParser';
import { parseErrorEvent } from '../parser/errorParser';
import { parseRxImmediateEvent } from '../parser/rxImmediateParser';
import { parseTxResultEvent } from '../parser/txResultParser';
import { parseRxBroadcastEvent } from '../parser/rxBroadcastParser';
import { drainAndClose, openPort, setDebug, writeToPort } from './serial';
import { areVersionsCompatible, parseSemver } from './semver';
import { parseRxTransmitEvent } from '../parser/rxTransmitParser';

enum ConnectionState {
  Disconnected = 'Disconnected',
  Connecting = 'Connecting',
  Connected = 'Connected',
  Disconnecting = 'Disconnecting',
  Error = 'Error',
}

/**
 * Used to register a listener for events from the Econet driver.
 */
export type Listener = (event: EconetEvent) => void;

/**
 * Use to filter events from the Econet driver.
 */
export type EventMatcher = (event: EconetEvent) => boolean;

const parsers = [
  parseStatusEvent,
  parseErrorEvent,
  parseMonitorEvent,
  parseRxTransmitEvent,
  parseRxImmediateEvent,
  parseRxBroadcastEvent,
  parseTxResultEvent,
];
let listeners: Array<Listener> = [];
let state: ConnectionState = ConnectionState.Disconnected;

/**
 * Connect the driver to the Piconet board.
 *
 * This function must be successfully called before interacting with the Econet driver. The following steps
 * are carried out:
 *
 * 1. Open a serial connection to the board. If `requestedDevice` is not specified then an attempt will be
 *    made to autodetect it. This step may fail if the board is not connected or another application is using
 *    it.
 * 2. The driver will then send a `STATUS` command to the board. This step may fail if the Raspberry Pi Pico
 *    has not been flashed with {@link https://github.com/jprayner/piconet/releases|the correct firmware}
 *    (.uf2 image).
 * 3. The driver will then compare the firmware version returned by the board against that of the driver
 *    using semantic versioning. If the versions are not compatible then an error will be thrown.
 *
 * After connecting, the board will be in the `STOP` operating mode. Remember to call {@link setMode} to
 * start it doing useful work.
 *
 * @param requestedDevice Optionally specifies the serial device for the Raspberry Pi Pico on the
 *                        Piconet board. If left undefined then an attempt will be made to automatically
 *                        detect it.
 */
export const connect = async (requestedDevice?: string): Promise<void> => {
  if (
    state !== ConnectionState.Disconnected &&
    state !== ConnectionState.Error
  ) {
    throw new Error(`Cannot connect whilst in ${state} state`);
  }

  state = ConnectionState.Connecting;
  try {
    await openPort(handleData, requestedDevice);
    const status = await readStatus();

    const firmwareVersionStr = status.firmwareVersion;
    const firmwareVersion = parseSemver(firmwareVersionStr);
    const driverVersionStr = PKG_VERSION;
    const driverVersion = parseSemver(PKG_VERSION);
    if (!areVersionsCompatible(firmwareVersion, driverVersion)) {
      throw new Error(
        `Driver version ${driverVersionStr} is not compatible with board firmware version ${firmwareVersionStr}.`,
      );
    }

    state = ConnectionState.Connected;
  } catch (e) {
    state = ConnectionState.Error;
    throw e;
  }
};

/**
 * Puts the board into a new operating mode.
 *
 * The board can be in one of three operating modes:
 *
 * * `STOP` - The board starts in this mode. No events are generated in response to network traffic,
 *        allowing the client to initialise configuration before proceeding.
 *
 * * `LISTEN` - The normal Econet station operating mode. The board generates events for broadcast
 *        frames or frames targeting the configured local Econet station number. You should normally
 *        set the station number before entering this mode: see {@link setEconetStation}.
 *
 * * `MONITOR` - The board generates an event for every frame received, regardless of its source or
 *        destination (promiscuous mode). Useful for capturing traffic between other stations like
 *        the BBC NETMON utility. A code example is provided for how to build such a utility.
 *
 * @param mode The new operating mode.
 */
export const setMode = async (
  mode: 'STOP' | 'MONITOR' | 'LISTEN',
): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot set mode on device whilst in ${state} state`);
  }

  switch (mode) {
    case 'STOP':
      await writeToPort('SET_MODE STOP\r');
      break;
    case 'MONITOR':
      await writeToPort('SET_MODE MONITOR\r');
      break;
    case 'LISTEN':
      await writeToPort('SET_MODE LISTEN\r');
      break;
    default:
      throw new Error('Invalid mode');
  }

  await readStatus();
};

/**
 * Sets the Econet station number for the board so that it knows which received frames to generate
 * events for and how to populate the "from address" of outbound frames.
 *
 * The station number should be unique on the Econet network.
 *
 * @param station The new Econet station number (an integer in range 1-254, inclusive).
 */
export const setEconetStation = async (station: number): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(
      `Cannot set econet station number on device whilst in ${state} state`,
    );
  }

  if (station < 1 || station >= 255) {
    throw new Error('Invalid station number');
  }

  await writeToPort(`SET_STATION ${station}\r`);
  await readStatus();
};

/**
 * Enables/disables driver debug logging. When enabled, shows the raw data passing between the driver
 * and the board.
 */
export const setDebugEnabled = (enabled: boolean) => {
  setDebug(enabled);
};

/**
 * Implements an Econet `TRANSMIT` operation.
 *
 * This consists of a "four-way handshake":
 *
 * 1. A "scout" frame sent from the board to the destination station. This includes a `controlByte`
 *    and `port` which are used by the receiver to identify the type of data being sent.
 * 2. Assuming the receiver is (a) connected to the network and listening for traffic and (b)
 *    is configured to handle the `controlByte`/`port`, it responds with a "scout ack" frame
 *    instructing the caller to proceed.
 * 3. The board sends a frame containing the payload `data` to the destination station.
 * 4. The receiver responds with a "data ack" frame confirming receipt of the data.
 *
 * @param station         Destination Econet station number (integer in range 1-254, inclusive).
 * @param network         Destination Econet network number (0 for local network; only use other
 *                        values if you have an appropriately configured Econet bridge).
 * @param controlByte     Econet control byte (integer in range 0-255, inclusive).
 * @param port            Econet port number (integer in range 0-255, inclusive).
 * @param data            Buffer containing binary payload data to send.
 * @param extraScoutData  Optional extra data to include in the scout frame. This is useful for
 *                        a small number of special operations such as NOTIFY.
 *
 * @returns Describes the result of the operation. If the operation was successful then the
 *          `success` flag is set to `true`; otherwise the `description` field describes the
 *          error.
 */
export const transmit = async (
  station: number,
  network: number,
  controlByte: number,
  port: number,
  data: Buffer,
  extraScoutData?: Buffer,
): Promise<TxResultEvent> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot transmit data on device whilst in ${state} state`);
  }

  if (station < 1 || station >= 255) {
    throw new Error('Invalid station number');
  }

  if (network < 0 || network > 255) {
    throw new Error('Invalid network number');
  }

  if (controlByte < 0 || controlByte >= 255) {
    throw new Error('Invalid control byte');
  }

  if (port < 0 || port > 255) {
    throw new Error('Invalid port number');
  }

  if (data.length > config.maxTxDataLength) {
    throw new Error('Data too long');
  }

  if (typeof extraScoutData !== 'undefined') {
    if (extraScoutData.length > config.maxScoutExtraDataLength) {
      throw new Error('Extra scout data too long');
    }

    await writeToPort(
      `TX ${station} ${network} ${controlByte} ${port} ${data.toString(
        'base64',
      )} ${extraScoutData.toString('base64')}\r`,
    );
  } else {
    await writeToPort(
      `TX ${station} ${network} ${controlByte} ${port} ${data.toString(
        'base64',
      )}\r`,
    );
  }

  const result = await waitForEvent(event => {
    return event instanceof TxResultEvent;
  }, 20000);
  return result as TxResultEvent;
};

/**
 * Disconnects from the board and closes the serial port.
 */
export const close = async (): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot close device whilst in ${state} state`);
  }
  await drainAndClose();
  state = ConnectionState.Disconnected;
};

/**
 * Adds a new listener for events generated by the board.
 *
 * @param listener The listener to add.
 */
export const addListener = (listener: Listener) => {
  if (!listeners.find(l => l === listener)) {
    listeners.push(listener);
  }
};

/**
 * Removes a listener previously registered with {@link addListener}.
 *
 * @param listener The listener to remove.
 */
export const removeListener = (listener: Listener) => {
  listeners = listeners.filter(l => l !== listener);
};

const fireListeners = (event: EconetEvent) => {
  listeners.forEach(listener => listener(event));
};

/**
 * Waits for a matching event with a timeout.
 *
 * @param matcher   A function that returns `true` if the passed event matches the required
 *                  criteria.
 * @param timeoutMs Maximum time to wait for a matching event in milliseconds. If no matching
 *                  event is found within this period then the promise is rejected.
 * @returns         The matching event.
 */
export const waitForEvent = async (
  matcher: EventMatcher,
  timeoutMs: number,
): Promise<EconetEvent> => {
  return new Promise((resolve, reject) => {
    // eslint-disable-next-line prefer-const
    let listener: Listener;

    const timer = setTimeout(() => {
      if (listener) {
        removeListener(listener);
      }
      reject(new Error(`No matching event found within ${timeoutMs}ms`));
    }, timeoutMs);

    listener = (event: EconetEvent) => {
      if (!matcher(event)) {
        return;
      }

      removeListener(listener);
      clearTimeout(timer);
      resolve(event);
    };
    addListener(listener);
  });
};

export type EventQueue = {
  events: Array<EconetEvent>;
  listener: Listener;
};

export const eventQueueCreate = (
  matcher: EventMatcher,
): EventQueue => {
  const events = new Array<EconetEvent>();
  const listener = (event: EconetEvent) => {
    if (!matcher(event)) {
      return;
    }
    events.push(event);
  };
  addListener(listener);

  return {
    events,
    listener,
  };
};

export const eventQueueDestroy = (
  queue: EventQueue,
) => {
  removeListener(queue.listener);
};

const sleepMs = (ms: number) => new Promise(resolve => setTimeout(resolve, ms));

export const eventQueueWait = async (queue: EventQueue, timeoutMs: number): Promise<EconetEvent> => {
  const startTime = Date.now();
  while (Date.now() - startTime < timeoutMs) {
    const event = queue.events.shift();
    if (event) {
      return event;
    }
    await sleepMs(10);
  }

  throw new Error(`No matching event found within ${timeoutMs}ms`);
};


/**
 * Queries the current status of the board.
 *
 * @returns The current status of the board.
 */
export const readStatus = async (): Promise<StatusEvent> => {
  if (
    state !== ConnectionState.Connecting &&
    state !== ConnectionState.Connected
  ) {
    throw new Error(`Cannot read status from device whilst in ${state} state`);
  }

  await writeToPort('STATUS\r');
  const result = await waitForEvent(
    event => event instanceof StatusEvent,
    2000,
  );
  return result as StatusEvent;
};

const handleData = (data: string) => {
  if (
    state !== ConnectionState.Connected &&
    state !== ConnectionState.Connecting
  ) {
    return;
  }

  parsers.forEach(parser => {
    const event = parser(data);
    if (event) {
      fireListeners(event);
    }
  });
};
