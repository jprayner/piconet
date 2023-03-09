import { driver } from "@jprayner/piconet-nodejs";
import { bufferToHexDump, controlByte, directoryHandles, port, standardTxMessage, stripCRs, waitForReceiveTxEvent } from "../common";

export const examineDir = async (serverStation: number, dirPath: string) => {
  const replyPort = 0x90;
  const functionCode = 0x03;

  let results: string[] = [];
  let startIndex = 0;
  while (true) {
    const examineHeader = Buffer.from([
      // return type a.k.a. ARG
      //    0x00 = all info, machine readable
      //    0x01 = all info, human readable
      //    0x02 = file title only
      //    0x03 = access + file title, character string
      0x01,
      // index of start of directory listing
      startIndex,
      // number of entries to return
      0x0b
    ]);
    const examinePathTrailer = Buffer.from(`${dirPath}\r`);
  
    const msg = standardTxMessage(
      replyPort,
      functionCode,
      directoryHandles.userRoot,
      directoryHandles.current,
      directoryHandles.library,
      Buffer.concat([examineHeader, examinePathTrailer])
    );
  
    const txResult = await driver.transmit(
      serverStation,
      0,
      controlByte,
      port,
      msg
    );
  
    if (txResult.result !== 'OK') {
      throw new Error(`Failed to send examine command to station ${serverStation}`);
    }
  
    const serverReply = await waitForReceiveTxEvent(serverStation, controlByte, [replyPort]);
  
    if (serverReply.resultCode !== 0x00) {
      const message = stripCRs(serverReply.data.toString('ascii'));
      throw new Error(`Login failed: ${message}`);
    }
    
    if (serverReply.data.length < 2) {
      throw new Error(`Malformed response from station ${serverStation}: success but not enough data`);
    }

    const numEntriesReturned = serverReply.data[0];
    if (numEntriesReturned === 0) {
      break;
    }

    const fileData = serverReply.data.slice(2).toString('ascii');
    const filesWithAccess = fileData
      .split('\0')
      .filter(
        f => f.length != 0
        && !(f.length === 1 && f.charCodeAt(0) === 0x80)
      );
    results = results.concat(filesWithAccess);
    startIndex += numEntriesReturned;

    // TODO timeout
  }

  return results;
};
