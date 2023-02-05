import { parseRxBroadcastEvent as parseRxBroadcastEvent } from './rxBroadcastParser';

describe('rx broadcast message parser', () => {
  it('should parse valid RX_BROADCAST event', () => {
    const eventStr = 'RX_BROADCAST abcdef123=';
    const result = parseRxBroadcastEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.econetFrame).toEqual(Buffer.from('abcdef123', 'base64'));
  });

  it('should reject invalid RX_BROADCAST event', () => {
    expect(() => parseRxBroadcastEvent('RX_BROADCAST')).toThrow('Protocol error. Invalid RX_BROADCAST event \'RX_BROADCAST\' received.');
  });
});
