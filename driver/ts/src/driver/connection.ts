import { autoDetect } from '@serialport/bindings-cpp';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { RxMode, StatusEvent } from '../types/statusEvent';
import { parseStatusEvent } from '../parser/status';
import { parseMonitorEvent } from '../parser/monitor';
import { MonitorEvent } from '../types/monitorEvent';
import { waitForEvent } from './waitEvent';

enum ConnectionState {
  Disconnected,
  Connecting,
  Connected,
  Error,
}

export type ConnectionEvent = StatusEvent | MonitorEvent;
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
      throw new Error(`Device ${this.device} already connected`);
    }

    console.log(`Connecting to ${this.device}...`);
    return new Promise((resolve, reject) => {
      this.port = new SerialPort({
        path: this.device,
        baudRate: 115200,
      });

      const parser = this.port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
      parser.on('data', (data) => this.handleData(data as string));

      this.readStatus()
        .then((status) => {
          this.state = ConnectionState.Connected;
          this.lastStatusEvent = status;
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
    switch (mode) {
      case RxMode.Stopped:
        return this.sendCommand('SET_MODE STOP');
        break;
      case RxMode.Monitoring:
        return this.sendCommand('SET_MODE MONITOR');
        break;
      case RxMode.Listening:
        return this.sendCommand('SET_MODE LISTEN');
        break;
      default:
        throw new Error('Invalid mode');
    }
  }

  public async close(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.port.drain((drainError) => {
        if (drainError) {
          reject(`Error on drain for close command: ${drainError.message}`);
          return;
        }
        this.state = ConnectionState.Connecting;

        resolve();
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
    console.log(`Received: ${data}`);
    try {
      console.log(`is it status event: ${data}`);
      const statusEvent = parseStatusEvent(data);
      if (statusEvent) {
        this.lastStatusEvent = statusEvent;
        console.log(`firing status event: ${data}`);
        this.fireListeners(statusEvent);
      }
      const monitorEvent = parseMonitorEvent(data);
      if (monitorEvent) {
        console.log(`firing monitor event: ${data}`);
        this.fireListeners(monitorEvent);
      }
    } catch (error) {
      // TODO do we need to disconnect here?
      console.error('Stopping connection due to protocol error', error);
      this.state = ConnectionState.Error;
    }
  }

  private async readStatus(): Promise<StatusEvent> {
    // if (this.state !== ConnectionState.Connected) {
    //   throw new Error('Not connected');
    // }

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
