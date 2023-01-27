import { autoDetect } from '@serialport/bindings-cpp';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { RxMode, StatusEvent } from '../types/statusEvent';
import { parseStatusEvent } from '../parser/status';
import { parseMonitorEvent } from '../parser/monitor';
import { MonitorEvent } from '../types/monitorEvent';
import { waitForEvent } from './waitEvent';
import { TransmitEvent } from '../types/transmitEvent';
import { ImmediateEvent } from '../types/immediateEvent';
import { BroadcastEvent } from '../types/broadcastEvent';
import { ErrorEvent } from '../types/errorEvent';
import { parseErrorEvent } from '../parser/error';
import { parseImmediateEvent } from '../parser/immediate';
import { parseTransmitEvent } from '../parser/transmit';
import { parseBroadcastEvent } from '../parser/broadcast';

enum ConnectionState {
  Disconnected = 'Disconnected',
  Connected = 'Connected',
  Disconnecting = 'Disconnecting',  
  Error = 'Error',
}

export type ConnectionEvent = StatusEvent | ErrorEvent | MonitorEvent | TransmitEvent | ImmediateEvent | BroadcastEvent;
export type Listener = (event: ConnectionEvent) => void;

export class Connection {
  public device: string;

  private listeners: Array<Listener> = [ ];

  private port: SerialPort;
  
  private state: ConnectionState;

  private lastStatusEvent: StatusEvent | undefined;

  constructor(device: string) {
    this.device = device;
    this.state = ConnectionState.Disconnected;
  }

  public async connect(): Promise<void> {
    if (this.state !== ConnectionState.Disconnected) {
      throw new Error(`Cannot connect device '${this.device}' whilst in ${this.state} state`);
    }

    return new Promise((resolve, reject) => {
      this.port = new SerialPort({
        path: this.device,
        baudRate: 115200,
      });

      const parser = this.port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
      parser.on('data', (data) => this.handleData(data as string));

      this.state = ConnectionState.Connected;

      this.readStatus()
        .then((status) => {
          this.fireListeners(status);
          resolve();
        })
        .catch((err) => {
          this.state = ConnectionState.Error;
          reject(err);
        });
    });
  }

  public async setMode(mode: RxMode): Promise<void> {
    if (this.state !== ConnectionState.Connected) {
      throw new Error(`Cannot set mode on device '${this.device}' whilst in ${this.state} state`);
    }

    switch (mode) {
      case RxMode.Stopped:
        await this.sendCommand('SET_MODE STOP');
        break;
      case RxMode.Monitoring:
        await this.sendCommand('SET_MODE MONITOR');
        break;
      case RxMode.Listening:
        await this.sendCommand('SET_MODE LISTEN');
        break;
      default:
        throw new Error('Invalid mode');
    }

    await this.readStatus();
  }

  public async setEconetStation(station: number): Promise<void> {
    if (this.state !== ConnectionState.Connected) {
      throw new Error(`Cannot set econet station number on device '${this.device}' whilst in ${this.state} state`);
    }

    if (station < 1 || station >= 255) {
      throw new Error('Invalid station number');
    }

    await this.sendCommand(`SET_STATION ${station}`);
    await this.readStatus();
  }

  public async close(): Promise<void> {
    if (this.state !== ConnectionState.Connected) {
      throw new Error(`Cannot close device '${this.device}' whilst in ${this.state} state`);
    }

    return new Promise((resolve, reject) => {
      this.port.drain((drainError) => {
        if (drainError) {
          reject(`Error on drain for close command: ${drainError.message}`);
          return;
        }
        this.port.close((closeError) => {
          if (closeError) {
            reject(`Error closing port: ${closeError.message}`);
            return;
          }
          this.state = ConnectionState.Disconnected;
          resolve();
        });
      });
    });
  }

  public addListener = (listener: Listener) => {
    if (!this.listeners.find((l) => l === listener)) {
      this.listeners.push(listener);
    }
	};

	public removeListener = (listener: Listener) => {
    this.listeners = this.listeners.filter(l => l !== listener);
	};

	private fireListeners = (event: ConnectionEvent): void => {
    this.listeners.forEach((listener) => listener(event));
	};

  private handleData(data: string) {
    if (this.state !== ConnectionState.Connected) {
      return;
    }

    try {
      const statusEvent = parseStatusEvent(data);
      if (statusEvent) {
        this.lastStatusEvent = statusEvent;
        this.fireListeners(statusEvent);
      }
      const monitorEvent = parseMonitorEvent(data);
      if (monitorEvent) {
        this.fireListeners(monitorEvent);
      }
      const immediateEvent = parseImmediateEvent(data);
      if (immediateEvent) {
        this.fireListeners(immediateEvent);
      }
      const transmitEvent = parseTransmitEvent(data);
      if (transmitEvent) {
        this.fireListeners(transmitEvent);
      }
      const broadcastEvent = parseBroadcastEvent(data);
      if (broadcastEvent) {
        this.fireListeners(broadcastEvent);
      }
      const errorEvent = parseErrorEvent(data);
      if (errorEvent) {
        this.fireListeners(errorEvent);
      }
    } catch (error) {
      console.error('Protocol error', error);
    }
  }

  private async readStatus(): Promise<StatusEvent> {
    if (this.state !== ConnectionState.Connected) {
      throw new Error(`Cannot read status from device '${this.device}' whilst in ${this.state} state`);
    }

    await this.sendCommand('STATUS');

    const result = await waitForEvent(this, (event => {
      return event.type === 'status';
    }), 2000);
    return result as StatusEvent;
  }

  private async sendCommand(command: string): Promise<void> {
    return new Promise((resolve, reject) => {
      this.port.write(`${command}\r`, (err) => {
        if (err) {
          reject(`Error on write for command '${command}: ${err.message}`);
          return;
        }

        this.port.drain((drainError) => {
          if (drainError) {
            reject(`Error on drain for command '${command}: ${drainError.message}`);
            return;
          }
          resolve();
        });  
      });
    });
  }
}
