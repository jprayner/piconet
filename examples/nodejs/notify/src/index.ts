import { driver } from '@jprayner/piconet-ts';

const localEconetStation = 2;
const destEconetStation = 127;

async function main() {
  console.log('Connecting to board...');
  await driver.connect();

  console.log(`Sending message to station ${destEconetStation} from station ${localEconetStation}...`);
  await driver.setEconetStation(localEconetStation);
  await sendNotify(destEconetStation, 'Mary had a little lamb');

  console.log('Message sent, closing connection to board...');
  await driver.close();
}

const sendNotify = async (station: number, str: string) => {
  for (const char of str) {
    await driver.transmit(station, 0, 0x85, 0x00, Buffer.from(char), Buffer.from([0x00, 0x00, char.charCodeAt(0), 0x00]));
  }
}

main();
