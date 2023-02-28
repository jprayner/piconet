import * as fs from 'fs';
import { driver, EconetEvent, RxDataEvent, RxTransmitEvent } from '@jprayner/piconet-nodejs';
import { hexdump } from '@gct256/hexdump';
import { Command, Option } from 'commander';

type txHeader = {
  replyPort: number,
  functionCode: number,
  handleUserRootDir: number,
  handleCurrentDir: number,
  handleLibDir: number,
};

type rxHeader = {
  commandCode: number,
  returnCode: number,
};

const responseMatcher = (sourceStation: number, sourceNetwork: number, controlByte: number, ports: number[]) => {
  return (event: EconetEvent) => {
    const result = event.type === 'RxTransmitEvent'
      && event.scoutFrame.length >= 6
      && event.scoutFrame[2] === sourceStation
      && event.scoutFrame[3] === sourceNetwork
      && event.scoutFrame[4] === controlByte
      && ports.find(p => p === event.scoutFrame[5]) !== undefined;
    return result;
  };
};

const standardTxMessage = (
    replyPort: number,
    functionCode: number,
    handleUserRootDir: number,
    handleCurrentDir: number,
    handleLibDir: number,
    data: Buffer) => {
  const header = Buffer.from([
    replyPort,
    functionCode,
    handleUserRootDir,
    handleCurrentDir,
    handleLibDir]);
  return Buffer.concat([header, data]);
};

const login = async (serverStation: number, username: string, password: string) => {
  const controlByte = 0x80;
  const port = 0x99;

  const replyPort = 0x90;
  const functionCode = 0x00;
  const handleUserRootDir = 0x01;
  const handleCurrentDir = 0x02;
  const handleLibDir = 0x04;

  const msg = standardTxMessage(
    replyPort,
    functionCode,
    handleUserRootDir,
    handleCurrentDir,
    handleLibDir,
    Buffer.from(`I AM ${username} ${password}\r`)
  );

  console.log(`sending msg: ${hexdump(msg)}`);

  const txResult = await driver.transmit(
    serverStation,
    0,
    controlByte,
    port,
    msg
  );

  if (txResult.result !== 'OK') {
    throw new Error(`Failed to send I AM command to station ${serverStation}`);
  }

  const serverReply = await waitForReceiveTxEvent(serverStation, controlByte, [replyPort]);

  if (serverReply.resultCode !== 0x00) {
    const message = stripCRs(serverReply.data.toString('ascii'));
    throw new Error(`Login failed: ${message}`);
  }

  if (serverReply.data.length < 4) {
    throw new Error(`Malformed response from station ${serverStation}: success but not enough data`);
  }

  return {
    handleCurrentDir: serverReply.data[0],
    handleUserRootDir: serverReply.data[1],
    handleLibDir: serverReply.data[2],
    bootOption: serverReply.data[3],
  };
};

const getFile = async (serverStation: number, filename: string) => {
  const loadTimeoutMs = 10000;
  const controlByte = 0x80;
  const port = 0x99;

  const replyPort = 0x90;
  const dataPort = 0x92; // different
  const functionCode = 0x02; // different
  const handleUserRootDir = 0x01;
  const handleCurrentDir = 0x02;
  const handleLibDir = 0x04;

  const msg = standardTxMessage(
    replyPort,
    functionCode,
    dataPort,
    handleCurrentDir,
    handleLibDir,
    Buffer.from(`${filename}\r`)
  );

  console.log(`sending msg: ${hexdump(msg)}`);

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

  console.log(`loadAddr: ${loadAddr}`
    + ` execAddr: ${execAddr}`
    + ` size: ${size}`
    + ` access: ${access}`
    + ` date: ${date}`
    + ` actualFilename: ${actualFilename}`);

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

const putFile = async (serverStation: number, filename: string) => {
  const loadTimeoutMs = 10000;
  const controlByte = 0x80;
  const port = 0x99;

  const replyPort = 0x90;
  const ackPort = 0x91; // different
  const functionCode = 0x01; // different
  const handleUserRootDir = 0x01;
  const handleCurrentDir = 0x02;
  const handleLibDir = 0x04;

  const loadAddr = 0xffff0e00;
  const execAddr = 0xffff2b80;
  const fileData = fs.readFileSync(filename);
  const fileTitle = `${filename}\r`;

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
  const bufferFileTitle = Buffer.from(fileTitle);

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
    handleCurrentDir,
    handleLibDir,
    requestData
  );

  console.log(`sending msg: ${hexdump(msg)}`);

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

  console.log(`dataPort: ${dataPort} blockSize: ${blockSize}`);

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
//    const message = stripCRs(serverReply.data.toString('ascii'));
    throw new Error(`Save failed`);
  }
  console.log('Save succeeded');
};
const stripCRs = (str: string) => str.replace(/\r/g, '');

const waitForAckEvent = async (serverStation: number, port: number) => {
  return await driver.waitForEvent(
    (event: EconetEvent) => {
      const result = event.type === 'RxTransmitEvent'
        && event.scoutFrame.length >= 6
        && event.scoutFrame[2] === serverStation
        && event.scoutFrame[3] === 0
        && event.scoutFrame[6] === port;
      return result;
    },
    2000);
}

const waitForReceiveTxEvent = async (serverStation: number, controlByte: number, ports: number[]) => {
  const rxTransmitEvent = await driver.waitForEvent(
    responseMatcher(serverStation, 0, controlByte, ports),
    2000);
  if (rxTransmitEvent.type !== 'RxTransmitEvent') {
    throw new Error(`Unexpected response from station ${serverStation}`);
  }
  if (rxTransmitEvent.dataFrame.length < 6) {
    throw new Error(`Malformed response from station ${serverStation}`);
  }
  return {
    controlByte: rxTransmitEvent.scoutFrame[4],
    port: rxTransmitEvent.scoutFrame[5],
    commandCode: rxTransmitEvent.dataFrame[4],
    resultCode: rxTransmitEvent.dataFrame[5],
    data: rxTransmitEvent.dataFrame.slice(6),
  }
}

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

const initConnection = async (options: any) => {
  console.log('Connecting to board...');
  await driver.connect(options.device);

  driver.addListener((event) => {
    console.log(event);
    if (event.type === 'ErrorEvent') {
      console.log('========================');
      console.log(`ERROR: ${event.description}`);
      console.log('========================\n');
    }
  });

  await driver.setEconetStation(options.station);
  await driver.setMode('LISTEN');
}

const commandLogin = async (username: string, password: string, options: any) => {
  const serverStation = parseInt(options.fileserver);

  await initConnection(options);
  const result = await login(serverStation, username, password);

  console.log(`tx result: ${JSON.stringify(result, null, 4)}`);
  
  console.log('Disconnecting from board...');
  await driver.close();
}

const commandGet = async (filename: string, options: any) => {
  const serverStation = parseInt(options.fileserver);

  await initConnection(options);
  const result = await getFile(serverStation, filename);
  console.log(`result: ${JSON.stringify(result, null, 4)}`);
  
  console.log(hexdump(result.data).join('\n'));
  
  fs.writeFileSync(result.actualFilename, result.data);

  console.log('Disconnecting from board...');
  await driver.close();
}

const commandPut = async (filename: string, options: any) => {
  const serverStation = parseInt(options.fileserver);

  await initConnection(options);
  await putFile(serverStation, filename);

  console.log('Disconnecting from board...');
  await driver.close();
}

const sleep = async (ms: number) => {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

const main = async () => {
  const program = new Command();

  program
    .name('ecoclient')
    .description('Econet fileserver client')
    .version('0.0.1');

    program.command('login')
      .description('login to fileserver like a "*I AM" command')
      .argument('<username>', 'username')
      .argument('[password]', 'password')
      .addOption(new Option('-dev, --device <string>', 'specify PICO serial device'))
      .addOption(new Option('-s, --station <number>', 'specify local econet station number')
        .env('ECONET_STATION')
      )
      .addOption(new Option('-fs, --fileserver <number>', 'specify fileserver station number')
        .default(254)
        .env('ECONET_FS_STATION')
      )
      .action(commandLogin);

    program.command('get')
      .description('get file from fileserver using "LOAD" command')
      .argument('<filename>', 'filename')
      .addOption(new Option('-dev, --device <string>', 'specify PICO serial device'))
      .addOption(new Option('-s, --station <number>', 'specify local econet station number')
        .env('ECONET_STATION')
      )
      .addOption(new Option('-fs, --fileserver <number>', 'specify fileserver station number')
        .default(254)
        .env('ECONET_FS_STATION')
      )
      .action(commandGet);

    program.command('put')
      .description('get file from fileserver using "SAVE" command')
      .argument('<filename>', 'filename')
      .addOption(new Option('-dev, --device <string>', 'specify PICO serial device'))
      .addOption(new Option('-s, --station <number>', 'specify local econet station number')
        .env('ECONET_STATION')
      )
      .addOption(new Option('-fs, --fileserver <number>', 'specify fileserver station number')
        .default(254)
        .env('ECONET_FS_STATION')
      )
      .action(commandPut);

  await program.parseAsync(process.argv);
}

main();