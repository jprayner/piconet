import { controlByte, directoryHandles, port, responseMatcher, standardTxMessage, stripCRs, waitForReceiveTxEvent } from "../common";
import { driver } from '@jprayner/piconet-nodejs';

export const load = async (serverStation: number, filename: string) => {
  const loadTimeoutMs = 10000;
  const replyPort = 0x90;
  const dataPort = 0x92;
  const functionCode = 0x02;

  const msg = standardTxMessage(
    replyPort,
    functionCode,
    dataPort,
    directoryHandles.current,
    directoryHandles.library,
    Buffer.from(`${filename}\r`)
  );

  const txResult = await driver.transmit(
    serverStation,
    0,
    controlByte,
    port,
    msg
  );

  if (txResult.result !== 'OK') {
    throw new Error(`Failed to send LOAD command to station ${serverStation}`);
  }

  const serverReply = await waitForReceiveTxEvent(serverStation, controlByte, [replyPort]);

  if (serverReply.resultCode !== 0x00) {
    const message = stripCRs(serverReply.data.toString('ascii'));
    throw new Error(`Load failed: ${message}`);
  }

  if (serverReply.data.length < 20) {
    throw new Error(`Malformed response in LOAD from station ${serverStation}: success but not enough data`);
  }

  const loadAddr = serverReply.data.readUInt32LE(0);
  const execAddr = serverReply.data.readUInt32LE(4);
  const size = serverReply.data[8]
    + (serverReply.data[9] << 8)
    + (serverReply.data[10] << 16);
  const access = serverReply.data[11];
  const date = serverReply.data.readUint16LE(12);
  const actualFilename = serverReply.data.subarray(14, 26).toString('ascii').trim();

  const startTime = Date.now();
  let data = Buffer.from('');
  while (Date.now() - startTime < loadTimeoutMs) {
    const dataOrEndEvent = await waitForDataOrStatus(serverStation, controlByte, dataPort, replyPort);
    if (dataOrEndEvent.port === replyPort) {
      if (serverReply.resultCode !== 0x00) {
        const message = stripCRs(serverReply.data.toString('ascii'));
        throw new Error(`Load failed during delivery: ${message}`);
      }
      break;
    }

    data = Buffer.concat([data, dataOrEndEvent.data]);
  };

  // TODO: handle timeout

  return {
    loadAddr,
    execAddr,
    size,
    access,
    date,
    actualFilename,
    actualSize: data.length,
    data,
  };
};

const waitForDataOrStatus = async (serverStation: number, controlByte: number, dataPort: number, statusPort: number) => {
  const rxTransmitEvent = await driver.waitForEvent(
    responseMatcher(serverStation, 0, controlByte, [dataPort, statusPort]),
    2000);
  if (rxTransmitEvent.type !== 'RxTransmitEvent') {
    throw new Error(`Unexpected response from station ${serverStation}`);
  }
  if (rxTransmitEvent.scoutFrame[5] === statusPort) {
    if (rxTransmitEvent.dataFrame.length < 6) {
      throw new Error(`Malformed response from station ${serverStation}`);
    }

    return {
      type: 'status',
      controlByte: rxTransmitEvent.scoutFrame[4],
      port: rxTransmitEvent.scoutFrame[5],
      commandCode: rxTransmitEvent.dataFrame[4],
      resultCode: rxTransmitEvent.dataFrame[5],
      data: rxTransmitEvent.dataFrame.slice(6),
    }
  } else {
    if (rxTransmitEvent.dataFrame.length < 2) {
      throw new Error(`Malformed response from station ${serverStation}`);
    }
  
    return {
      type: 'data',
      data: rxTransmitEvent.dataFrame.slice(4),
    }
  }
}
