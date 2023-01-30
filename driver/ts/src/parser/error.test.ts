import { parseErrorEvent } from './error';

describe('error message parser', () => {
  it('should parse valid ERROR event', () => {
    const eventStr = 'ERROR a bad thing happened';
    const result = parseErrorEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.description).toEqual('a bad thing happened');
  });
});
