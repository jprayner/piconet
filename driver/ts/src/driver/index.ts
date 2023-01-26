import { autoDetect } from '@serialport/bindings-cpp';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';
import { Connection } from './connection';

const connections = new Map<string, Connection>();

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
