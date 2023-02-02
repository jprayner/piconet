import { driver } from '@jprayner/piconet-ts';
import { hexdump } from '@gct256/hexdump';

async function main() {
  await driver.connect();
  await driver.setEconetStation(2);
  await driver.setMode('MONITOR');
  driver.addListener((event) => {
    if (event.type === 'error') {
      console.log('========================');
      console.log(`ERROR: ${event.description}`);
      console.log('========================\n');
    } else {
      logFrame(event);
    }
  });

  await sleep(30000);

  await driver.close();
}

const logFrame = (event) => {
  const hasScoutAndDataFrames = event.type === 'immediate' || event.type === 'transmit';
  const hasEconetFrame = event.type === 'monitor' || event.type === 'broadcast';
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
