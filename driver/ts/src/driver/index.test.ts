import { connect, close } from '.';

describe('driver', () => {
  it('should connect successfully', async () => {
    const connection = await connect('/dev/cu.usbmodem1101');
    await close(connection);
  });
});
