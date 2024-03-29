import {
  connect,
  close,
  setMode,
  addListener,
  setEconetStation,
  removeListener,
  waitForEvent,
  eventQueueCreate,
  eventQueueWait,
  eventQueueDestroy,
} from '.';
import { EconetEvent } from '../types/econetEvent';
import { StatusEvent } from '../types/statusEvent';
import { openPort, writeToPort } from './serial';
import { PKG_VERSION } from './version';

jest.mock('./serial');

const openPortMock = jest.mocked(openPort, true);
const writeToPortMock = jest.mocked(writeToPort, true);

describe('driver', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it('should connect successfully', async () => {
    setTimeout(() => {
      const dataHandlerFunc = openPortMock.mock.calls[0][0];
      dataHandlerFunc(`STATUS ${PKG_VERSION} 2 00 0\r`);
    }, 100);

    await expect(connect()).resolves.toBeUndefined();
    await expect(close()).resolves.toBeUndefined();
  });

  it('should fire events to handler registered with addListener', async () => {
    let event;
    const eventHandler = (e: EconetEvent) => {
      event = e;
    };
    addListener(eventHandler);

    setTimeout(() => {
      const dataHandlerFunc = openPortMock.mock.calls[0][0];
      dataHandlerFunc(`STATUS ${PKG_VERSION} 2 00 0\r`);
    }, 100);

    await connect();

    expect(event).toBeDefined();
    await close();
    removeListener(eventHandler);
  });

  it('should not fire events to handler removed with removeListener', async () => {
    let event;
    const eventHandler = (e: EconetEvent) => {
      event = e;
    };
    addListener(eventHandler);
    removeListener(eventHandler);

    setTimeout(() => {
      const dataHandlerFunc = openPortMock.mock.calls[0][0];
      dataHandlerFunc(`STATUS ${PKG_VERSION} 2 00 0\r`);
    }, 100);

    await connect();

    expect(event).toBeUndefined();
    await close();
  });

  it('should return matching event from waitForEvent', async () => {
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    const matcher = (e: EconetEvent) => true;

    mockStatusEventFromBoard(0);
    await connect();

    mockStatusEventFromBoard(1);
    const event = await waitForEvent(matcher, 1000);

    expect(event).toBeDefined();
    expect(event instanceof StatusEvent && event.rxMode === 1).toBeTruthy();

    await close();
  });

  it('should throw exception from waitForEvent if no matching event', async () => {
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    const matcher = (e: EconetEvent) => false;

    mockStatusEventFromBoard(0);
    await connect();

    mockStatusEventFromBoard(1);
    await expect(waitForEvent(matcher, 1000)).rejects.toThrowError();

    await close();
  });

  it('should correctly queue and return events in order', async () => {
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    const matcher = (e: EconetEvent) => true;

    mockStatusEventFromBoard(0);
    await connect();
    const dataHandlerFunc = openPortMock.mock.calls[0][0];

    const queue = eventQueueCreate(matcher);

    dataHandlerFunc(`STATUS ${PKG_VERSION} 2 00 1\r`);
    dataHandlerFunc(`STATUS ${PKG_VERSION} 2 00 2\r`);

    const event1 = await eventQueueWait(queue, 1000);
    const event2 = await eventQueueWait(queue, 1000);

    expect(event1 instanceof StatusEvent && event1.rxMode === 1).toBeTruthy();
    expect(event2 instanceof StatusEvent && event2.rxMode === 2).toBeTruthy();

    eventQueueDestroy(queue);
    await close();
  });

  it('should throw exception from eventQueueWait if no matching event', async () => {
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    const matcher = (e: EconetEvent) => false;

    mockStatusEventFromBoard(0);
    await connect();
    const dataHandlerFunc = openPortMock.mock.calls[0][0];

    const queue = eventQueueCreate(matcher);

    dataHandlerFunc(`STATUS ${PKG_VERSION} 2 00 1\r`);

    await expect(eventQueueWait(queue, 1000)).rejects.toThrowError();

    eventQueueDestroy(queue);
    await close();
  });

  it('should return a connection failure if versions do not match', async () => {
    setTimeout(() => {
      const dataHandlerFunc = openPortMock.mock.calls[0][0];
      dataHandlerFunc('STATUS 99.99.99 2 00 0\r');
    }, 100);

    await expect(connect()).rejects.toThrow(
      `Driver version ${PKG_VERSION} is not compatible with board firmware version 99.99.99.`,
    );
  });

  it('should return a connection failure if the board is already connected', async () => {
    mockStatusEventFromBoard(0);
    await connect();
    await expect(connect()).rejects.toThrow(
      'Cannot connect whilst in Connected state',
    );
    await close();
  });

  it('should send setMode correctly for RxMode.Stopped', async () => {
    mockStatusEventFromBoard(0);
    await connect();

    mockStatusEventFromBoard(0);
    await setMode('STOP');
    expect(writeToPortMock).toHaveBeenCalledWith('SET_MODE STOP\r');
    await close();
  });

  it('should send setMode correctly for RxMode.Listening', async () => {
    mockStatusEventFromBoard(0);
    await connect();

    mockStatusEventFromBoard(1);
    await setMode('LISTEN');
    expect(writeToPortMock).toHaveBeenCalledWith('SET_MODE LISTEN\r');
    await close();
  });

  it('should send setMode correctly for RxMode.Monitoring', async () => {
    mockStatusEventFromBoard(0);
    await connect();

    mockStatusEventFromBoard(2);
    await setMode('MONITOR');
    expect(writeToPortMock).toHaveBeenCalledWith('SET_MODE MONITOR\r');
    await close();
  });

  it('should throw error if setMode called when driver in wrong state', async () => {
    mockStatusEventFromBoard(0);
    await connect();
    await close();
    expect(writeToPortMock).toHaveBeenCalledTimes(1);

    await expect(setMode('MONITOR')).rejects.toThrow(
      'Cannot set mode on device whilst in Disconnected state',
    );
    expect(writeToPortMock).toHaveBeenCalledTimes(1);
  });

  it('should send SET_STATION correctly on call to setEconetStation', async () => {
    mockStatusEventFromBoard(0);
    await connect();

    mockStatusEventFromBoard(1);
    await setEconetStation(123);
    expect(writeToPortMock).toHaveBeenCalledWith('SET_STATION 123\r');
    await close();
  });

  it('should throw error if setEconetStation passed station number >=255', async () => {
    mockStatusEventFromBoard(0);
    await connect();

    await expect(setEconetStation(255)).rejects.toThrow(
      'Invalid station number',
    );
    await close();
  });

  it('should throw error if setEconetStation passed station number < 1', async () => {
    mockStatusEventFromBoard(0);
    await connect();

    await expect(setEconetStation(0)).rejects.toThrow('Invalid station number');
    await close();
  });

  // TODO: test transmit
});

const mockStatusEventFromBoard = (rxMode: number) => {
  setTimeout(() => {
    const dataHandlerFunc = openPortMock.mock.calls[0][0];
    dataHandlerFunc(`STATUS ${PKG_VERSION} 2 00 ${rxMode}\r`);
  }, 100);
};
