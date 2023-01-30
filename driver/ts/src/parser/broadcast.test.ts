import { parseBroadcastEvent } from './broadcast';

describe('broadcast message parser', () => {
  it('should parse valid BROADCAST event', () => {
    const eventStr = 'BROADCAST abcdef123=';
    const result = parseBroadcastEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.econetFrame).toEqual(Buffer.from('abcdef123', 'base64'));
  });

  it('should reject invalid BROADCAST event', () => {
    expect(() => parseBroadcastEvent('BROADCAST')).toThrow('Protocol error. Invalid BROADCAST event \'BROADCAST\' received.');
  });
});
