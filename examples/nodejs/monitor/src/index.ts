import { driver, EconetEvent, ErrorEvent } from '@jprayner/piconet-nodejs';

async function main() {
  console.log('Connecting to board...');
  await driver.connect();

  driver.addListener((event) => {
    if (event instanceof RxDataEvent) {
      console.log(`ERROR: ${event.description}`);
      return;
    } else {
      driver.rxDataEventToString(event);
    }
  });

  await driver.setMode('MONITOR');

  console.log('Listening for traffic...');

  await sleep(60000);
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
