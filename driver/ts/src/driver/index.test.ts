import { connect, close, setMode, addListener } from '.';
import { RxMode } from '../types/statusEvent';

jest.setTimeout(30000);

describe('driver', () => {
  // it('should list available ports', async () => {
  //   await list();
  // });

  it('should connect successfully', async () => {
    const connection = await connect();
    console.log(`Connected`);
    await setMode(RxMode.Listening);
    console.log('Set mode to monitoring');
    // await connection.setEconetStation(3);
    addListener((event) => {
      console.log(`Received event: ${JSON.stringify(event)}`);
    });

    await sleep(20000);
    await close();
  });
});

async function sleep(ms: number) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}
