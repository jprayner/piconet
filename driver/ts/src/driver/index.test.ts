import { connect, close, list } from '.';
import { RxMode } from '../types/statusEvent';

jest.setTimeout(30000);

describe('driver', () => {
  // it('should list available ports', async () => {
  //   await list();
  // });

  it('should connect successfully', async () => {
    const connection = await connect();
    console.log(`Connected to ${connection.device}...`);
    await connection.setMode(RxMode.Listening);
    console.log('Set mode to monitoring');
    // await connection.setEconetStation(3);
    connection.addListener((event) => {
      console.log(`Received event: ${JSON.stringify(event)}`);
    });

    await sleep(20000);
    await close(connection);
  });
});

async function sleep(ms: number) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}
