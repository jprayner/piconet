import { driver } from '@jprayner/piconet-nodejs';
import { hexdump } from '@gct256/hexdump';
import { EconetEvent } from '@jprayner/piconet-nodejs/dist/types/driver';
import { RxTransmitEvent } from '@jprayner/piconet-nodejs/dist/types/types/rxTransmitEvent';

async function main() {
  console.log('Connecting to board...');
  await driver.connect();

  driver.addListener((event) => {
    console.log(event);
    if (event.type === 'ErrorEvent') {
      console.log('========================');
      console.log(`ERROR: ${event.description}`);
      console.log('========================\n');
    }
    if (event.type === 'RxTransmitEvent') {
      logFrame(event);
      handleReceive(event);
    }
  });

  await driver.setEconetStation(2);
  await driver.setMode('LISTEN');

  console.log('Listening for traffic...');

  await sleep(10000);
  // process.on('SIGINT', async () => {
  console.log('Disconnecting from board...');
  await driver.close();
  process.exit();
  // });  
}

const sleep = async (ms: number) => {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

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

const handleReceive = async (event: RxTransmitEvent) => {
  const scout = parseScoutFrame(event.scoutFrame);
  const data = parseData(event.dataFrame).contents;

  if (scout.controlByte !== 0x80) {
    console.log('Ignoring request with unexpected control byte');
    return;
  }
  if (scout.port !== 0x99) {
    console.log('Ignoring request on unexpected port');
    return;
  }

  if (data.length < 5) {
    console.log('OSCLI data frame is too short');
    return;
  }

  const replyPort = data[0];
  const command = data.subarray(5);
  console.log(`Received replyport=${replyPort}, command="${command.toString('ascii')}"`);
  //await driver.reply(event.receiveId, Buffer.from([]));

  await driver.transmit(scout.fromStation, scout.fromNetwork, 0x80, replyPort, Buffer.from([0x05, 0x00, 0x01, 0x02, 0x04, 0x00]));
  //driver.reply(event.receiveId, )
  //event.receiveId
};

const logFrame = (event: EconetEvent) => {
  const hasScoutAndDataFrames = event.type === 'RxImmediateEvent' || event.type === 'RxTransmitEvent';
  const hasEconetFrame = event.type === 'MonitorEvent' || event.type === 'RxBroadcastEvent';
  const hasAnyFrame = hasScoutAndDataFrames || hasEconetFrame;
  if (hasAnyFrame) {
    const frameForHeader = hasScoutAndDataFrames ? event.scoutFrame : event.econetFrame;
    const toStation = frameForHeader[0];
    const fromStation = frameForHeader[2];
    console.log(`${event.type.toUpperCase()} ${fromStation} --> ${toStation}`);
    if (hasEconetFrame) {
      console.log('        ' + hexdump(event.econetFrame).join('\n        '));
    } else {
      console.log('        ' + hexdump(event.scoutFrame).join('\n        ') + ' [SCOUT]');
      console.log('        ' + hexdump(event.dataFrame).join('\n        '));
    }
  }
};

main();
