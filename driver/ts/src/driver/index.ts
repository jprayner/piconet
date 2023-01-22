import { autoDetect } from '@serialport/bindings-cpp';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { Connection } from '../types/connection';

export const list = async () => {
  const Binding = autoDetect();
  const portInfos = await Binding.list();
  console.log(JSON.stringify(portInfos, null, 2));
};

const autoDetectDevice = async (): Promise<string> => {
  const Binding = autoDetect();
  const portInfos = await Binding.list();
  const port = portInfos.find((portInfo) => (portInfo.vendorId === '2e8a' && portInfo.productId === '000a'));

  if (!port) {
    throw new Error('Failed to find a PICO device');
  }

  return port.path;
};

export const connect = async (requestedDevice?: string): Promise<Connection> => {
  const deviceToUse = requestedDevice ?? (await autoDetectDevice());
  console.log(`Connecting to ${deviceToUse}...`);
  return new Promise((resolve, reject) => {
    const port = new SerialPort({
      path: deviceToUse,
      baudRate: 115200,
    });
    port.write('STATUS\r', (err) => {
      if (err) {
        reject(`Error on write: ${err.message}`);
        return;
      }
      console.log('message written');
    });
    port.on('error', (err) => {
      reject(`Error: ${err.message}`);
    });

    const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
    parser.on('data', (data) => {
      console.log(data);
      resolve({ device: requestedDevice, port });
    });
  });
};

export const close = async (connection: Connection): Promise<void> => {
  return new Promise((resolve, reject) => {
    connection.port.close((err) => {
      if (err) {
        reject(`Error on close: ${err.message}`);
        return;
      }
      console.log('Port closed');
      resolve();
    });
  });
};
