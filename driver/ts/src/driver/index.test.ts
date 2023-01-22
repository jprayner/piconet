import { connect, close, list } from '.';

describe('driver', () => {
  it('should list available ports', async () => {
    await list();
  });

  it('should connect successfully', async () => {
    const connection = await connect();
    await close(connection);
  });
});
