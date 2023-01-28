import { RxMode, StatusEvent } from '../types/statusEvent';
import { parseStatusEvent } from '../parser/status';
import { parseMonitorEvent } from '../parser/monitor';
import { MonitorEvent } from '../types/monitorEvent';
import { TransmitEvent } from '../types/transmitEvent';
import { ImmediateEvent } from '../types/immediateEvent';
import { BroadcastEvent } from '../types/broadcastEvent';
import { ErrorEvent } from '../types/errorEvent';
import { parseErrorEvent } from '../parser/error';
import { parseImmediateEvent } from '../parser/immediate';
import { parseTransmitEvent } from '../parser/transmit';
import { parseBroadcastEvent } from '../parser/broadcast';
import { drainAndClose, openPort, writeToPort } from './serial';

export enum ConnectionState {
  Disconnected = 'Disconnected',
  Connecting = 'Connecting',
  Connected = 'Connected',
  Disconnecting = 'Disconnecting',
  Error = 'Error',
}

export type EconetEvent = StatusEvent | ErrorEvent | MonitorEvent | TransmitEvent | ImmediateEvent | BroadcastEvent;
export type Listener = (event: EconetEvent) => void;
type EventMatcher = (event: EconetEvent) => boolean;

const parsers = [ parseStatusEvent, parseErrorEvent, parseMonitorEvent, parseTransmitEvent, parseImmediateEvent, parseBroadcastEvent ];
let device: string;
let listeners: Array<Listener> = [ ];
let state: ConnectionState = ConnectionState.Disconnected;

export const connect = async (requestedDevice?: string): Promise<void> => {
  if (state !== ConnectionState.Disconnected && state !== ConnectionState.Error) {
    throw new Error(`Cannot connect whilst in ${state} state`);
  }

  state = ConnectionState.Connecting;
  try {
    await openPort(handleData, requestedDevice);
    const status = await readStatus();
    fireListeners(status);
    state = ConnectionState.Connected;
  } catch (e) {
    state = ConnectionState.Error;
    throw e;
  }
};

export const setMode = async (mode: RxMode): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot set mode on device '${device}' whilst in ${state} state`);
  }

  switch (mode) {
    case RxMode.Stopped:
      await writeToPort('SET_MODE STOP\r');
      break;
    case RxMode.Monitoring:
      await writeToPort('SET_MODE MONITOR\r');
      break;
    case RxMode.Listening:
      await writeToPort('SET_MODE LISTEN\r');
      break;
    default:
      throw new Error('Invalid mode');
  }

  await readStatus();
};

export const setEconetStation = async (station: number): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot set econet station number on device '${device}' whilst in ${state} state`);
  }

  if (station < 1 || station >= 255) {
    throw new Error('Invalid station number');
  }

  await writeToPort(`SET_STATION ${station}\r`);
  await readStatus();
};

export const close = async (): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot close device '${device}' whilst in ${state} state`);
  }
  await drainAndClose();
  state = ConnectionState.Disconnected;
};

export const addListener = (listener: Listener) => {
  if (!listeners.find((l) => l === listener)) {
    listeners.push(listener);
  }
};

export const removeListener = (listener: Listener) => {
  listeners = listeners.filter(l => l !== listener);
};

export const fireListeners = (event: EconetEvent) => {
  listeners.forEach((listener) => listener(event));
};

const handleData = (data: string) => {
  if (state !== ConnectionState.Connected && state !== ConnectionState.Connecting) {
    return;
  }

  try {
    parsers.forEach(parser => {
      const event = parser(data);
      if (event) {
        fireListeners(event);
      }
    });
  } catch (error) {
    console.error('Protocol error', error);
  }
};

const readStatus = async (): Promise<StatusEvent> => {
  if (state !== ConnectionState.Connecting && state !== ConnectionState.Connected) {
    throw new Error(`Cannot read status from device '${device}' whilst in ${state} state`);
  }

  await writeToPort('STATUS\r');
  const result = await waitForEvent((event => {
    return event.type === 'status';
  }), 2000);
  return result as StatusEvent;
};

const waitForEvent = async (matcher: EventMatcher, timeoutMs: number): Promise<EconetEvent> => {
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
