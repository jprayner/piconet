import { parseTransmitEvent } from './transmit';

describe('transmit message parser', () => {
  it('should parse valid TRANSMIT event', () => {
    const eventStr = 'TRANSMIT abcdef123= 123abcdef=';
    const result = parseTransmitEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.scoutFrame).toEqual(Buffer.from('abcdef123', 'base64'));
    expect(result?.dataFrame).toEqual(Buffer.from('123abcdef', 'base64'));
  });

  it('should reject invalid IMMEDIATE event', () => {
    expect(() => parseTransmitEvent('TRANSMIT abcdef123')).toThrow('Protocol error. Invalid TRANSMIT event \'TRANSMIT abcdef123\' received.');
  });
});
