import { driver, EconetEvent, RxTransmitEvent } from '@jprayner/piconet-nodejs';
import { controlByte, executeCliCommand, port } from './common';

jest.mock('@jprayner/piconet-nodejs');
const driverMock = jest.mocked(driver);

interface RxTransmitProps {
  fsStation: number;
  fsNet: number;
  localStation: number;
  localNet: number;
  controlByte: number;
  replyPort: number;
  commandCode: number;
  resultCode: number;
  data: Buffer;
}

const dummyReplyRxTransmitEvent = (props: RxTransmitProps): RxTransmitEvent => {
  const header = Buffer.from([
    props.localStation,
    props.localNet,
    props.fsStation,
    props.fsNet,
    props.commandCode,
    props.resultCode,
  ]);
  const dataFrame = Buffer.concat([header, props.data]);
  return {
    type: 'RxTransmitEvent',
    scoutFrame: Buffer.from([
      props.localStation,
      props.localNet,
      props.fsStation,
      props.fsNet,
      props.controlByte,
      props.replyPort,
    ]),
    dataFrame,
    receiveId: 0,
  };
};

describe('common', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it('should execute a command successfully', async () => {
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
    driverMock.waitForEvent.mockImplementation(
      async (callback: (event: EconetEvent) => boolean, timeoutMs: number) => {
        return dummyReplyRxTransmitEvent({
          fsStation: 254,
          fsNet: 0,
          localStation: 1,
          localNet: 0,
          controlByte,
          replyPort: port,
          commandCode: 0,
          resultCode: 0,
          data: Buffer.from([]),
        });
      },
    );
    const resultPromise = executeCliCommand(254, 'BYE');
    const result = await resultPromise;

    expect(driverMock.transmit).toHaveBeenCalled();
    expect(driverMock.waitForEvent).toHaveBeenCalled();
    expect(result.resultCode).toBe(0);
  });

  it('should throw error when no response received from server', async () => {
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
          result: 'invalid station number',
        };
      },
    );
    await expect(executeCliCommand(254, 'BYE')).rejects.toThrowError(
      'Failed to send command to station 254: invalid station number',
    );
    expect(driverMock.transmit).toHaveBeenCalled();
    expect(driverMock.waitForEvent).not.toHaveBeenCalled();
  });

  it('should feed back error message when server rejects command', async () => {
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
    driverMock.waitForEvent.mockImplementation(
      async (callback: (event: EconetEvent) => boolean, timeoutMs: number) => {
        return dummyReplyRxTransmitEvent({
          fsStation: 254,
          fsNet: 0,
          localStation: 1,
          localNet: 0,
          controlByte,
          replyPort: port,
          commandCode: 0,
          resultCode: 1,
          data: Buffer.from('Bad things are occuring'),
        });
      },
    );
    await expect(executeCliCommand(254, 'BYE')).rejects.toThrowError(
      'Bad things are occuring',
    );
    expect(driverMock.transmit).toHaveBeenCalled();
    expect(driverMock.waitForEvent).toHaveBeenCalled();
  });
});
