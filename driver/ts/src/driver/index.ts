import { autoDetect } from '@serialport/bindings-cpp';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
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

export enum ConnectionState {
  Disconnected = 'Disconnected',
  Connected = 'Connected',
  Disconnecting = 'Disconnecting',  
  Error = 'Error',
}

export type EconetEvent = StatusEvent | ErrorEvent | MonitorEvent | TransmitEvent | ImmediateEvent | BroadcastEvent;
export type Listener = (event: EconetEvent) => void;
type EventMatcher = (event: EconetEvent) => boolean;

let device: string;
let listeners: Array<Listener> = [ ];
let port: SerialPort;
let state: ConnectionState = ConnectionState.Disconnected;
let lastStatusEvent: StatusEvent | undefined;

export const connect = async (requestedDevice?: string): Promise<void> => {
  if (state !== ConnectionState.Disconnected && state !== ConnectionState.Error) {
    throw new Error(`Cannot connect whilst in ${state} state`);
  }

  device = requestedDevice ?? (await autoDetectDevice());

  return new Promise((resolve, reject) => {
    port = new SerialPort({
      path: device,
      baudRate: 115200,
    });

    const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
    parser.on('data', (data) => handleData(data as string));

    state = ConnectionState.Connected;

    readStatus()
      .then((status) => {
        fireListeners(status);
        resolve();
      })
      .catch((err) => {
        state = ConnectionState.Error;
        reject(err);
      });
  });
};

export const setMode = async(mode: RxMode): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot set mode on device '${device}' whilst in ${state} state`);
  }

  switch (mode) {
    case RxMode.Stopped:
      await sendCommand('SET_MODE STOP');
      break;
    case RxMode.Monitoring:
      await sendCommand('SET_MODE MONITOR');
      break;
    case RxMode.Listening:
      await sendCommand('SET_MODE LISTEN');
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

  await sendCommand(`SET_STATION ${station}`);
  await readStatus();
};

export const close = async (): Promise<void> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot close device '${device}' whilst in ${state} state`);
  }

  return new Promise((resolve, reject) => {
    port.drain((drainError) => {
      if (drainError) {
        reject(`Error on drain for close command: ${drainError.message}`);
        return;
      }
      port.close((closeError) => {
        if (closeError) {
          reject(`Error closing port: ${closeError.message}`);
          return;
        }
        state = ConnectionState.Disconnected;
        resolve();
      });
    });
  });
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
  if (state !== ConnectionState.Connected) {
    return;
  }

  try {
    const statusEvent = parseStatusEvent(data);
    if (statusEvent) {
      lastStatusEvent = statusEvent;
      fireListeners(statusEvent);
    }
    const monitorEvent = parseMonitorEvent(data);
    if (monitorEvent) {
      fireListeners(monitorEvent);
    }
    const immediateEvent = parseImmediateEvent(data);
    if (immediateEvent) {
      fireListeners(immediateEvent);
    }
    const transmitEvent = parseTransmitEvent(data);
    if (transmitEvent) {
      fireListeners(transmitEvent);
    }
    const broadcastEvent = parseBroadcastEvent(data);
    if (broadcastEvent) {
      fireListeners(broadcastEvent);
    }
    const errorEvent = parseErrorEvent(data);
    if (errorEvent) {
      fireListeners(errorEvent);
    }
  } catch (error) {
    console.error('Protocol error', error);
  }
};

const readStatus = async (): Promise<StatusEvent> => {
  if (state !== ConnectionState.Connected) {
    throw new Error(`Cannot read status from device '${device}' whilst in ${state} state`);
  }

  await sendCommand('STATUS');
  const result = await waitForEvent((event => {
    return event.type === 'status';
  }), 2000);
  return result as StatusEvent;
};

const sendCommand = async (command: string): Promise<void> => {
  return new Promise((resolve, reject) => {
    port.write(`${command}\r`, (err) => {
      if (err) {
        reject(`Error on write for command '${command}: ${err.message}`);
        return;
      }

      port.drain((drainError) => {
        if (drainError) {
          reject(`Error on drain for command '${command}: ${drainError.message}`);
          return;
        }
        resolve();
      });  
    });
  });
};

const autoDetectDevice = async (): Promise<string> => {
  const Binding = autoDetect();
  const portInfos = await Binding.list();
  const picoPort = portInfos.find((portInfo) => (portInfo.vendorId === '2e8a' && portInfo.productId === '000a'));

  if (!picoPort) {
    throw new Error('Failed to find a PICO device');
  }

  return picoPort.path;
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
