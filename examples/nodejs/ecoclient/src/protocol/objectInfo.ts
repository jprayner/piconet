import { driver } from '@jprayner/piconet-nodejs';
import {
  controlByte,
  directoryHandles,
  port,
  standardTxMessage,
  waitForReceiveTxEvent,
} from '../common';

export const readDirAccessObjectInfo = async (
  serverStation: number,
  dirPath: string,
) => {
  const replyPort = 0x90;
  const functionCode = 0x12;

  const objectInfoHeader = Buffer.from([
    // object info type a.k.a. ARG
    //    0x01 = read creation date
    //    0x02 = read load and execution addresses (8 bytes)
    //    0x03 = read size (3 bytes)
    //    0x04 = read type/access byte
    //    0x05 = read all file attributes
    //    0x06 = access/cycle/dir. name of given dir
    0x06,
  ]);
  const objectInfoTrailer = Buffer.from(`${dirPath}\r`);

  const msg = standardTxMessage(
    replyPort,
    functionCode,
    directoryHandles.userRoot,
    directoryHandles.current,
    directoryHandles.library,
    Buffer.concat([objectInfoHeader, objectInfoTrailer]),
  );

  const txResult = await driver.transmit(
    serverStation,
    0,
    controlByte,
    port,
    msg,
  );

  if (txResult.result !== 'OK') {
    throw new Error(
      `Failed to send object info command (0x12) to station ${serverStation}: ${txResult.result}`,
    );
  }

  const serverReply = await waitForReceiveTxEvent(serverStation, controlByte, [
    replyPort,
  ]);

  if (serverReply.data.length < 15) {
    throw new Error(
      `Malformed response from station ${serverStation}: success but not enough data`,
    );
  }

  const dirName = serverReply.data.slice(3, 13).toString('ascii').trim();
  const isOwner = serverReply.data[13] === 0;
  const cycleNum = serverReply.data[14];

  return {
    dirName,
    isOwner,
    cycleNum,
  };
};
