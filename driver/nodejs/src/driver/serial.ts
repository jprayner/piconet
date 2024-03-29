import { autoDetect } from '@serialport/bindings-cpp';
import { SerialPort } from 'serialport';
import { ReadlineParser } from '@serialport/parser-readline';

export type DataListener = (data: string) => void;
let port: SerialPort;
let debug: boolean;

export const openPort = async (
  listener: DataListener,
  requestedDevice?: string,
): Promise<void> => {
  const device = requestedDevice ?? (await autoDetectDevice());

  return new Promise((resolve, reject) => {
    port = new SerialPort({
      path: device,
      baudRate: 115200,
      autoOpen: false,
    });

    port.open(openError => {
      if (openError) {
        reject(`[open] Failed to open: ${openError.message}`);
        return;
      }

      const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));
      parser.on('data', data => {
        if (debug) {
          console.debug(data);
        }
        listener(data as string);
      });

      resolve();
    });
  });
};

export const drainAndClose = async (): Promise<void> => {
  return new Promise((resolve, reject) => {
    port.drain(drainError => {
      if (drainError) {
        reject(`[drainAndClose] Failed to drain: ${drainError.message}`);
        return;
      }
      port.close(closeError => {
        if (closeError) {
          reject(`[drainAndClose] Failed to close: ${closeError.message}`);
          return;
        }
        resolve();
      });
    });
  });
};

export const writeToPort = async (data: string): Promise<void> => {
  return new Promise((resolve, reject) => {
    if (debug) {
      console.debug(data);
    }

    port.write(`${data}\r`, err => {
      if (err) {
        reject(`[writeToPort] Error writing '${data}: ${err.message}`);
        return;
      }

      port.drain(drainError => {
        if (drainError) {
          reject(
            `[writeToPort] Error on drain writing '${data}: ${drainError.message}`,
          );
          return;
        }
        resolve();
      });
    });
  });
};

export const setDebug = (value: boolean): void => {
  debug = value;
};

const autoDetectDevice = async (): Promise<string> => {
  const Binding = autoDetect();
  const portInfos = await Binding.list();
  const picoPort = portInfos.find(
    portInfo =>
      portInfo.vendorId &&
      portInfo.vendorId.toLowerCase() === '2e8a' &&
      portInfo.productId &&
      portInfo.productId.toLowerCase() === '000a',
  );

  if (!picoPort) {
    throw new Error('Failed to find a Pico device');
  }

  return picoPort.path;
};
