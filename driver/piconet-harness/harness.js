import { driver } from '@jprayner/piconet-ts';
import { hexdump } from '@gct256/hexdump';

async function main() {
  await driver.connect();
  await driver.setEconetStation(2);

  // await driver.setMode('MONITOR');
  driver.addListener((event) => {
    if (event.type === 'ErrorEvent') {
      console.log('========================');
      console.log(`ERROR: ${event.description}`);
      console.log('========================\n');
    } else if (event.type === 'TxResultEvent') {
      console.log('========================');
      console.log(`TX RESULT: ${event.result}`);
      console.log('========================\n');
    } else {
      logFrame(event);
    }
  });

  await driver.setEconetStation(2);
  await driver.setMode('LISTEN');

  await sendNotify('A');
  await sleep(5);
  await sendNotify('B');
  await sleep(5);
  await sendNotify('C');
  
  //await driver.transmit(168, 0, 0x85, 0x00, Buffer.from([0x41]), Buffer.from([0x00, 0x00, 0x41, 0x00]));
  //while (true) {
  await sleep(1000);
  //}

  await driver.close();
}

const sendNotify = async (char) => {
  await driver.transmit(168, 0, 0x85, 0x00, Buffer.from(char), Buffer.from([0x00, 0x00, char.charCodeAt(0), 0x00]));
}

const logFrame = (event) => {
  const hasScoutAndDataFrames = event.type === 'RxImmediateEvent' || event.type === 'RxTransmitEvent';
  const hasEconetFrame = event.type === 'MonitorEvent' || event.type === 'RxBroadcastEvent';
  const hasAnyFrame = hasScoutAndDataFrames || hasEconetFrame;
  if (hasAnyFrame) {
    const frameForHeader = hasScoutAndDataFrames ? event.scoutFrame : event.econetFrame;
    const fromStation = frameForHeader[0];
    const toStation = frameForHeader[2];
    console.log(`${event.type.toUpperCase()} ${fromStation} --> ${toStation}`);
    if (hasEconetFrame) {
      console.log('        ' + hexdump(event.econetFrame).join('\n        '));
    } else {
      console.log('        ' + hexdump(event.scoutFrame).join('\n        ') + ' [SCOUT]');
      console.log('        ' + hexdump(event.dataFrame).join('\n        '));
    }
  }
};

async function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

main();
