import { driver, ErrorEvent, RxDataEvent } from '@jprayner/piconet-nodejs';

async function main() {
  console.log('Connecting to board...');
  await driver.connect();

  driver.addListener(event => {
    if (event instanceof ErrorEvent) {
      console.error(`ERROR: ${event.description}`);
      return;
    } else if (event instanceof RxDataEvent) {
      console.log(`Received ${event.constructor.name}`);
      console.log(event.toString());
    }
  });

  await driver.setMode('MONITOR');

  console.log('Listening for traffic...');

  process.on('SIGINT', async () => {
    console.log('Disconnecting from board...');
    await driver.close();
    process.exit();
  });
}

main();
