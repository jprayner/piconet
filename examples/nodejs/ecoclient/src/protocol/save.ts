import { controlByte, directoryHandles, port, responseMatcher, standardTxMessage, stripCRs, waitForAckEvent, waitForReceiveTxEvent } from "../common";
import { driver } from '@jprayner/piconet-nodejs';

export const save = async (serverStation: number, fileData: Buffer, remoteFilename: string, loadAddr: number, execAddr: number) => {
  const loadTimeoutMs = 10000;
  const replyPort = 0x90;
  const ackPort = 0x91;
  const functionCode = 0x01;

  const bufferLoadAddr = Buffer.from([
    loadAddr & 0xff,
    (loadAddr >> 8) & 0xff,
    (loadAddr >> 16) & 0xff,
    (loadAddr >> 24) & 0xff,
  ]);
  const bufferExecAddr = Buffer.from([
    loadAddr & 0xff,
    (loadAddr >> 8) & 0xff,
    (loadAddr >> 16) & 0xff,
    (loadAddr >> 24) & 0xff,
  ]);
  const bufferFileSize = Buffer.from([
    fileData.length & 0xff,
    (fileData.length >> 8) & 0xff,
    (fileData.length >> 16) & 0xff
  ]);
  const bufferFileTitle = Buffer.from(remoteFilename);

  const requestData = Buffer.concat([
    bufferLoadAddr,
    bufferExecAddr,
    bufferFileSize,
    bufferFileTitle,
  ]);

  const msg = standardTxMessage(
    replyPort,
    functionCode,
    ackPort,
    directoryHandles.current,
    directoryHandles.library,
    requestData
  );

  const txResult = await driver.transmit(
    serverStation,
    0,
    controlByte,
    port,
    msg
  );

  if (txResult.result !== 'OK') {
    throw new Error(`Failed to send SAVE command to station ${serverStation}`);
  }

  const serverReply = await waitForReceiveTxEvent(serverStation, controlByte, [replyPort]);

  if (serverReply.resultCode !== 0x00) {
    const message = stripCRs(serverReply.data.toString('ascii'));
    throw new Error(`Save failed: ${message}`);
  }

  if (serverReply.data.length < 3) {
    throw new Error(`Malformed response in SAVE from station ${serverStation}: success but not enough data`);
  }

  const dataPort = serverReply.data[0];
  const blockSize = serverReply.data.readUInt16LE(1);

  const startTime = Date.now();
  let dataLeftToSend = Buffer.from(fileData);
  while (dataLeftToSend.length > 0 && (Date.now() - startTime < loadTimeoutMs)) {
    const dataToSend = dataLeftToSend.slice(0, blockSize);
    dataLeftToSend = dataLeftToSend.slice(blockSize);

    const dataTxResult = await driver.transmit(
      serverStation,
      0,
      controlByte,
      dataPort,
      dataToSend
    );

    if (dataTxResult.result !== 'OK') {
      throw new Error(`Failed to send SAVE data to station ${serverStation}`);
    }

    if (dataLeftToSend.length > 0) {
      waitForAckEvent(serverStation, ackPort);
    }
  }

  const finalReply = await waitForSaveStatus(serverStation, controlByte, replyPort);

  if (finalReply.resultCode !== 0x00) {
    throw new Error(`Save failed`);
  }
};


const waitForSaveStatus = async (serverStation: number, controlByte: number, statusPort: number) => {
  const rxTransmitEvent = await driver.waitForEvent(
    responseMatcher(serverStation, 0, controlByte, [statusPort]),
    2000);
  if (rxTransmitEvent.type !== 'RxTransmitEvent') {
    throw new Error(`Unexpected response from station ${serverStation}`);
  }
  if (rxTransmitEvent.dataFrame.length < 9) {
    throw new Error(`Malformed response from station ${serverStation}`);
  }

  return {
    commandCode: rxTransmitEvent.dataFrame[4],
    resultCode: rxTransmitEvent.dataFrame[5],
    accessByte: rxTransmitEvent.dataFrame[6],
    date: rxTransmitEvent.dataFrame.readUint16LE(7),
  }
}
