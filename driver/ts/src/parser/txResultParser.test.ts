import { parseTxResultEvent } from './txResultParser';

describe('tx result message parser', () => {
  it('should parse valid TX_RESULT event', () => {
    const eventStr = 'TX_RESULT OK';
    const parsedEvent = parseTxResultEvent(eventStr);
    expect(parsedEvent).toBeDefined();
    expect(parsedEvent?.result).toEqual('OK');
  });

  it('should reject invalid TX_RESULT event', () => {
    expect(() => parseTxResultEvent('TX_RESULT')).toThrow('Protocol error. Invalid TX_RESULT event \'TX_RESULT\' received.');
  });
});
