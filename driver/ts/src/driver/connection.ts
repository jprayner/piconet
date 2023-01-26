import { autoDetect } from '@serialport/bindings-cpp';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { BoardState } from '../types/statusEvent';
import { parseEvent } from '../parser/status';

enum ConnectionState {
  Disconnected,
  Connecting,
  Connected,
  Error,
}

export default class Connection {
  device: string;

  port: SerialPort;
  
  state: ConnectionState;

  lastBoardState: BoardState | undefined;

  constructor(device: string) {
    this.device = device;
    this.state = ConnectionState.Disconnected;
  }

  async connect(): Promise<void> {
    if (this.state !== ConnectionState.Disconnected) {
      throw new Error('Already connected');
    }

    console.log(`Connecting to ${this.device}...`);
    return new Promise((resolve, reject) => {
      const port = new SerialPort({
        path: this.device,
        baudRate: 115200,
      });

      port.write('STATUS\r', (err) => {
        if (err) {
          reject(`Error on write: ${err.message}`);
          return;
        }
      });

      // TODO: Add timeout
      const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
      parser.on('data', (data: string) => {
        console.log(data);

        try {
          this.lastBoardState = parseEvent(data);
          if (this.lastBoardState) {
            // got valid status response
            resolve();
          }
        } catch (e) {
          reject(e);
        }
      });
    });  
  }
}
