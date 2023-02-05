import { parseRxImmediateEvent as parseRxImmediateEvent } from './rxImmediateParser';

describe('immediate message parser', () => {
  it('should parse valid RX_IMMEDIATE event', () => {
    const eventStr = 'RX_IMMEDIATE abcdef123= 123abcdef=';
    const result = parseRxImmediateEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.scoutFrame).toEqual(Buffer.from('abcdef123', 'base64'));
    expect(result?.dataFrame).toEqual(Buffer.from('123abcdef', 'base64'));
  });

  it('should reject invalid RX_IMMEDIATE event', () => {
    expect(() => parseRxImmediateEvent('RX_IMMEDIATE abcdef123')).toThrow('Protocol error. Invalid RX_IMMEDIATE event \'RX_IMMEDIATE abcdef123\' received.');
  });
});
