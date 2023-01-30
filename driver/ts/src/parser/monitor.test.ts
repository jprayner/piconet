import { parseMonitorEvent } from './monitor';

describe('monitor message parser', () => {
  it('should parse valid MONITOR event', () => {
    const eventStr = 'MONITOR abcdef123=';
    const result = parseMonitorEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.econetFrame).toEqual(Buffer.from('abcdef123', 'base64'));
  });

  it('should reject invalid MONITOR event', () => {
    expect(() => parseMonitorEvent('MONITOR')).toThrow('Protocol error. Invalid MONITOR event \'MONITOR\' received.');
  });
});
