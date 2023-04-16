import { driver } from '@jprayner/piconet-nodejs';

const localEconetStation = 171; // make sure this is unused on your network
const destEconetStation = 168; // set this to the destination station number

async function main() {
  console.log('Connecting to board...');
  await driver.connect();

  console.log(
    `Sending message to station ${destEconetStation} from station ${localEconetStation}...`,
  );
  await driver.setEconetStation(localEconetStation);
  await sendNotify(destEconetStation, 'Mary had a little lamb');

  console.log('Message sent, closing connection to board...');
  await driver.close();
}

const sendNotify = async (station: number, str: string) => {
  for (const char of str) {
    const scoutExtraData = Buffer.from([0x00, 0x00, char.charCodeAt(0), 0x00]);
    await driver.transmit(
      station, // destination station number
      0, // destination network number
      0x85, // control byte for NOTIFY
      0x00, // port for immediate operation
      Buffer.from(char), // contents of data frame
      scoutExtraData, // NOTIFY has unusual extra data in scout
    );
  }
};

main();
