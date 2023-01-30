import { parseImmediateEvent } from './immediate';

describe('immediate message parser', () => {
  it('should parse valid IMMEDIATE event', () => {
    const eventStr = 'IMMEDIATE abcdef123= 123abcdef=';
    const result = parseImmediateEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.scoutFrame).toEqual(Buffer.from('abcdef123', 'base64'));
    expect(result?.dataFrame).toEqual(Buffer.from('123abcdef', 'base64'));
  });

  it('should reject invalid IMMEDIATE event', () => {
    expect(() => parseImmediateEvent('IMMEDIATE abcdef123')).toThrow('Protocol error. Invalid IMMEDIATE event \'IMMEDIATE abcdef123\' received.');
  });
});
