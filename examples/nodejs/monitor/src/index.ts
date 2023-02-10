import { driver } from '@jprayner/piconet-ts';
import { hexdump } from '@gct256/hexdump';
import { EconetEvent } from '@jprayner/piconet-ts/dist/types/driver';

async function main() {
  console.log('Connecting to board...');
  await driver.connect();

  driver.addListener((event) => {
    console.log(event);
    if (event.type === 'ErrorEvent') {
      console.log('========================');
      console.log(`ERROR: ${event.description}`);
      console.log('========================\n');
    } else {
      logFrame(event);
    }
  });

  await driver.setMode('MONITOR');

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

const logFrame = (event: EconetEvent) => {
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

main();