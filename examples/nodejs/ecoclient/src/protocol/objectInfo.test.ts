import { driver, EconetEvent, RxTransmitEvent } from '@jprayner/piconet-nodejs';
import { dir } from 'console';
import { waitForReceiveTxEvent } from '../common';
import { readDirAccessObjectInfo } from './objectInfo';

jest.mock('@jprayner/piconet-nodejs');
jest.mock('../common');

const driverMock = jest.mocked(driver);
const waitForReceiveTxEventMock = jest.mocked(waitForReceiveTxEvent);

describe('objectInfo protocol handler', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it('should read directory object info successfully', async () => {
    driverMock.transmit.mockImplementation(
      async (
        station: number,
        network: number,
        controlByte: number,
        port: number,
        data: Buffer,
        extraScoutData?: Buffer,
      ) => {
        return {
          type: 'TxResultEvent',
          result: 'OK',
        };
      },
    );

    waitForReceiveTxEventMock.mockImplementation(
      async (station: number, controlByte: number, replyPorts: number[]) => {
        const header = Buffer.from([0, 0, 0]);
        const dirName10Chars = Buffer.from('SOMEDIR   ');
        const trailer = Buffer.from([
          0x00, // 0x00 = owner, anything else = public
          0x10, // cycle number
        ]);
        return {
          controlByte,
          port: replyPorts[0],
          commandCode: 0,
          resultCode: 0,
          data: Buffer.concat([header, dirName10Chars, trailer]),
        };
      },
    );

    const result = await readDirAccessObjectInfo(254, '$');
    expect(result).toEqual({
      dirName: 'SOMEDIR',
      isOwner: true,
      cycleNum: 0x10,
    });
  });

  it('should throw error if server fails to respond to read directory object info', async () => {
    driverMock.transmit.mockImplementation(
      async (
        station: number,
        network: number,
        controlByte: number,
        port: number,
        data: Buffer,
        extraScoutData?: Buffer,
      ) => {
        return {
          type: 'TxResultEvent',
          result: 'Something bad happened',
        };
      },
    );

    await expect(readDirAccessObjectInfo(254, '$')).rejects.toThrowError(
      'Failed to send object info command (0x12) to station 254: Something bad happened',
    );
  });

  it('should throw error if server returns malformed response from read directory object info', async () => {
    driverMock.transmit.mockImplementation(
      async (
        station: number,
        network: number,
        controlByte: number,
        port: number,
        data: Buffer,
        extraScoutData?: Buffer,
      ) => {
        return {
          type: 'TxResultEvent',
          result: 'OK',
        };
      },
    );

    waitForReceiveTxEventMock.mockImplementation(
      async (station: number, controlByte: number, replyPorts: number[]) => {
        return {
          controlByte,
          port: replyPorts[0],
          commandCode: 0,
          resultCode: 1,
          data: Buffer.from([0, 0, 0]),
        };
      },
    );

    await expect(readDirAccessObjectInfo(254, '$')).rejects.toThrowError(
      'Malformed response from station 254: success but not enough data',
    );
  });
});
