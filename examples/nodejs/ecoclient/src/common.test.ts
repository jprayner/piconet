import { driver } from '@jprayner/piconet-nodejs';
import { controlByte, executeCliCommand, port } from './common';

//const driverMock = jest.mocked(driver, true);
//jest.mock('@jprayner/piconet-nodejs');


const driverMock = jest.mocked(driver, true);

describe('common', () => {
  /*
  it('should execute a command', async () => {
    jest.mocked(driver.transmit).mockImplementation(async (station: number, network: number, controlByte: number, port: number, data: Buffer, extraScoutData?: Buffer) => {
      return {
        type: 'TxResultEvent',
        result: 'OK',
      };
    });
    jest.mocked(driver.waitForEvent).mockImplementation(async (callback: (event: EconetEvent) => boolean, timeoutMs: number) => {
      return {
        type: 'RxTransmitEvent',
        scoutFrame: Buffer.from([0x01, 0x00, 0xfe, 0x00, controlByte, port]),
        dataFrame: Buffer.from([0xbe, 0xef]),
        receiveId: 0,
      };
    });
    const resultPromise = executeCliCommand(254, 'BYE');
    //expect(waitForEventMock.mock.calls).toHaveLength(1);
    //expect(waitForEventMock.mock.calls[0]).toHaveLength(1);
    //const callback = waitForEventMock.mock.calls[0][0];
    //callback();
    const result = await resultPromise;
  });
  */
});
