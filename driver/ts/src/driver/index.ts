import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { Connection } from '../types/connection';

export const connect = async (device: string): Promise<Connection> => {
  console.log(`Connecting to ${device}...`);
  return new Promise((resolve, reject) => {
    const port = new SerialPort({
      path: device,
      baudRate: 115200,
    });
    port.write('STATUS\r', (err) => {
      if (err) {
        reject(`Error on write: ${err.message}`);
      }
      console.log('message written');
    });
    port.on('error', (err) => {
      reject(`Error: ${err.message}`);
    });

    const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
    parser.on('data', (data) => {
      console.log(data);
      resolve({ device, port });
    });
  });
};

export const close = async (connection: Connection): Promise<void> => {
  return new Promise((resolve, reject) => {
    connection.port.close((err) => {
      if (err) {
        reject(`Error on close: ${err.message}`);
      }
      console.log('Port closed');
      resolve();
    });
  });
};
