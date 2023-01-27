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
  console.log(`Connecting to ${deviceToUse}...`);
  if (connections.has(deviceToUse)) {
    throw new Error(`Device ${deviceToUse} already connected`);
  }
  const connection = new Connection(deviceToUse);
  await connection.connect();
  connections.set(deviceToUse, connection);
  return connection;
};

export const close = async (connection: Connection): Promise<void> => {
  await connection.close();
  connections.delete(connection.device);
};
