import { parseTxResultEvent } from './txResultParser';

describe('tx result message parser', () => {
  it('should parse valid, successful TX_RESULT event', () => {
    const eventStr = 'TX_RESULT OK';
    const parsedEvent = parseTxResultEvent(eventStr);
    expect(parsedEvent).toBeDefined();
    expect(parsedEvent?.success).toEqual(true);
    expect(parsedEvent?.description).toEqual('OK');
  });

  it('should parse valid, unsuccessful TX_RESULT event', () => {
    const eventStr = 'TX_RESULT OVERFLOW';
    const parsedEvent = parseTxResultEvent(eventStr);
    expect(parsedEvent).toBeDefined();
    expect(parsedEvent?.success).toEqual(false);
    expect(parsedEvent?.description).toEqual('OVERFLOW');
  });

  it('should reject invalid TX_RESULT event', () => {
    expect(() => parseTxResultEvent('TX_RESULT')).toThrow(
      "Protocol error. Invalid TX_RESULT event 'TX_RESULT' received.",
    );
  });
});
