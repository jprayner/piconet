import { driver, ErrorEvent } from '@jprayner/piconet-nodejs';
import { RxTransmitEvent } from '@jprayner/piconet-nodejs';

const localStationNum = 171;  // Make sure this is unique on your Econet network!
const controlByte = 0x80;     // Identifies Econet file server packets
const port = 0x99;            // Econet file server listen port

const stripCRs = (str: string) => str.replace(/\r/g, '');

const main = async () => {
  console.log('Connecting to board...');
  await driver.connect();

  driver.addListener((event) => {
    if (event instanceof ErrorEvent) {
      console.log(`ERROR: ${event.description}`);
    } else if (event instanceof RxTransmitEvent) {
      handleReceive(event);
    }
  });

  await driver.setEconetStation(localStationNum);
  console.log(`Set local station number to ${localStationNum}`);

  await driver.setMode('LISTEN');
  console.log('Listening for traffic...');

  process.on('SIGINT', async () => {
    console.log('Disconnecting from board...');
    await driver.close();
    process.exit();
  });
}

const handleReceive = async (event: RxTransmitEvent) => {
  const scout = parseScoutFrame(event.scoutFrame);
  const data = parseData(event.dataFrame).contents;

  if (scout.controlByte !== controlByte) {
    console.log('Ignoring request with unexpected control byte');
    return;
  }
  if (scout.port !== port) {
    console.log('Ignoring request on unexpected port');
    return;
  }

  if (data.length < 5) {
    console.log('OSCLI data frame is too short');
    return;
  }

  const replyPort = data[0];
  const command = data.subarray(5);
  console.log(`Received OSCLI command="${stripCRs(command.toString('ascii'))}"`);

  // issue a dummy successful reply
  const txResult = await driver.transmit(
    scout.fromStation,
    scout.fromNetwork,
    controlByte,
    replyPort,
    Buffer.from([
      0x05, // indicates a successful login
      0x00, // return code of zero indicates success
      0x01, // user root dir handle
      0x02, // currently selected dir handle
      0x04, // library dir handle
      0x00, // boot option (0 = none)
    ]));
  
  if (txResult.success) {
    console.log('Successfully sent reply');
  } else {
    console.log(`Failed to send reply: ${txResult.description}`);
  }
};

const parseScoutFrame = (scoutFrame: Buffer) => {
  if (scoutFrame.length < 6) {
    throw new Error('Scout frame is too short');
  }

  return {
    toStation: scoutFrame[0],
    toNetwork: scoutFrame[1],
    fromStation: scoutFrame[2],
    fromNetwork: scoutFrame[3],
    controlByte: scoutFrame[4],
    port: scoutFrame[5],
  }
};

const parseData = (dataFrame: Buffer) => {
  if (dataFrame.length < 4) {
    throw new Error('Data frame is too short');
  }

  return {
    toStation: dataFrame[0],
    toNetwork: dataFrame[1],
    fromStation: dataFrame[2],
    fromNetwork: dataFrame[3],
    contents: dataFrame.subarray(4),
  };
};

main();
