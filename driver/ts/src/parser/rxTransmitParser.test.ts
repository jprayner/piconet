import { parseRxTransmitEvent } from './rxTransmitParser';

describe('rx transmit message parser', () => {
  it('should parse valid RX_TRANSMIT event', () => {
    const eventStr = 'RX_TRANSMIT abcdef123= 123abcdef=';
    const result = parseRxTransmitEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.scoutFrame).toEqual(Buffer.from('abcdef123', 'base64'));
    expect(result?.dataFrame).toEqual(Buffer.from('123abcdef', 'base64'));
  });

  it('should reject invalid RX_TRANSMIT event', () => {
    expect(() => parseRxTransmitEvent('RX_TRANSMIT abcdef123')).toThrow('Protocol error. Invalid RX_TRANSMIT event \'RX_TRANSMIT abcdef123\' received.');
  });
});
