import { SerialPort } from 'serialport';

export type Connection = {
  device: string;
  port: SerialPort;
};
