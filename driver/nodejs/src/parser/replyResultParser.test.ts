import { parseReplyResultEvent } from './replyResultParser';

describe('reply result message parser', () => {
  it('should parse valid REPLY_RESULT event', () => {
    const eventStr = 'REPLY_RESULT OK';
    const parsedEvent = parseReplyResultEvent(eventStr);
    expect(parsedEvent).toBeDefined();
    expect(parsedEvent?.result).toEqual('OK');
  });

  it('should reject invalid REPLY_RESULT event', () => {
    expect(() => parseReplyResultEvent('REPLY_RESULT')).toThrow(
      "Protocol error. Invalid REPLY_RESULT event 'REPLY_RESULT' received.",
    );
  });
});
