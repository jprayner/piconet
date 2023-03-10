import { hexdump } from '@gct256/hexdump';
import config from '../config';
import { StatusEvent } from '../types/statusEvent';
import { parseStatusEvent } from '../parser/statusParser';
import { parseMonitorEvent } from '../parser/monitorParser';
import { MonitorEvent } from '../types/monitorEvent';
import { TxResultEvent } from '../types/txResultEvent';
import { RxImmediateEvent } from '../types/rxImmediateEvent';
import { RxBroadcastEvent } from '../types/rxBroadcastEvent';
import { ErrorEvent } from '../types/errorEvent';
import { parseErrorEvent } from '../parser/errorParser';
import { parseRxImmediateEvent } from '../parser/rxImmediateParser';
import { parseTxResultEvent } from '../parser/txResultParser';
import { parseRxBroadcastEvent } from '../parser/rxBroadcastParser';
import { drainAndClose, openPort, writeToPort } from './serial';
import { areVersionsCompatible, parseSemver } from './semver';
import { RxTransmitEvent } from '../types/rxTransmitEvent';
import { parseRxTransmitEvent } from '../parser/rxTransmitParser';
import { ReplyResultEvent } from '../types/replyResultEvent';
import { parseReplyResultEvent } from '../parser/replyResultParser';

enum ConnectionState {
  Disconnected = 'Disconnected',
  Connecting = 'Connecting',
  Connected = 'Connected',
  Disconnecting = 'Disconnecting',
  Error = 'Error',
}

export type EconetEvent =
  | StatusEvent
  | ErrorEvent
  | MonitorEvent
  | RxTransmitEvent
  | RxImmediateEvent
  | RxBroadcastEvent
  | TxResultEvent
  | ReplyResultEvent;

export type RxDataEvent =
  | MonitorEvent
  | RxTransmitEvent
  | RxImmediateEvent
  | RxBroadcastEvent;

export type Listener = (event: EconetEvent) => void;
export type EventMatcher = (event: EconetEvent) => boolean;

const parsers = [
  parseStatusEvent,
  parseErrorEvent,
  parseMonitorEvent,
  parseRxTransmitEvent,
  parseRxImmediateEvent,
  parseRxBroadcastEvent,
  parseTxResultEvent,
  parseReplyResultEvent,
];
let listeners: Array<Listener> = [];
let state: ConnectionState = ConnectionState.Disconnected;

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
    const driverVersionStr = config.version;
    const driverVersion = parseSemver(config.version);
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
  // TODO: should we check that status has correct station number?
};

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

    console.log(
      `TX ${station} ${network} ${controlByte} ${port} ${data.toString(
        'base64',
      )} ${extraScoutData.toString('base64')}\r`,
    );
    await writeToPort(
      `TX ${station} ${network} ${controlByte} ${port} ${data.toString(
        'base64',
      )} ${extraScoutData.toString('base64')}\r`,
    );
  } else {
    console.log(
      `TX ${station} ${network} ${controlByte} ${port} ${data.toString(
        'base64',
      )}\r`,
    );
    await writeToPort(
      `TX ${station} ${network} ${controlByte} ${port} ${data.toString(
        'base64',
      )}\r`,
    );
  }

  const result = await waitForEvent(event => {
    return event.type === 'TxResultEvent';
  }, 2000);
  return result as TxResultEvent;
};

export const reply = async (
  receiveId: number,
  data: Buffer,
): Promise<ReplyResultEvent> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot transmit data whilst in ${state} state`);
  }

  console.log(`REPLY ${receiveId} ${data.toString('base64')}\r`);
  await writeToPort(`REPLY ${receiveId} ${data.toString('base64')}\r`);

  const result = await waitForEvent(event => {
    return event.type === 'ReplyResultEvent';
  }, 2000);
  return result as ReplyResultEvent;
};

export const close = async (): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot close device whilst in ${state} state`);
  }
  await drainAndClose();
  state = ConnectionState.Disconnected;
};

export const addListener = (listener: Listener) => {
  if (!listeners.find(l => l === listener)) {
    listeners.push(listener);
  }
};

export const removeListener = (listener: Listener) => {
  listeners = listeners.filter(l => l !== listener);
};

export const fireListeners = (event: EconetEvent) => {
  listeners.forEach(listener => listener(event));
};

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

export const rxDataEventToString = (event: RxDataEvent) => {
  const hasScoutAndDataFrames =
    event.type === 'RxImmediateEvent' || event.type === 'RxTransmitEvent';
  const hasEconetFrame =
    event.type === 'MonitorEvent' || event.type === 'RxBroadcastEvent';
  const frameForHeader = hasScoutAndDataFrames
    ? event.scoutFrame
    : event.econetFrame;
  const toStation = frameForHeader[0];
  const fromStation = frameForHeader[2];
  const title = `${event.type} ${fromStation} --> ${toStation}\n`;
  if (hasEconetFrame) {
    return title + '        ' + hexdump(event.econetFrame).join('\n        ');
  } else {
    return (
      title +
      '        ' +
      hexdump(event.scoutFrame).join('\n        ') +
      ' [SCOUT]\n' +
      hexdump(event.dataFrame).join('\n        ')
    );
  }
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

const readStatus = async (): Promise<StatusEvent> => {
  if (
    state !== ConnectionState.Connecting &&
    state !== ConnectionState.Connected
  ) {
    throw new Error(`Cannot read status from device whilst in ${state} state`);
  }

  await writeToPort('STATUS\r');
  const result = await waitForEvent(event => {
    return event.type === 'status';
  }, 2000);
  return result as StatusEvent;
};
