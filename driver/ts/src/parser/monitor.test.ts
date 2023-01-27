import { parseMonitorEvent } from './monitor';
import { hexDump } from '../util/hexDump';

describe('monitor message parser', () => {
  it('should parse valid MON event', () => {
    const eventStr = 'MON abcdef123=';
    const result = parseMonitorEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.econetFrame).toEqual(Buffer.from('abcdef123', 'base64'));
  });

  it('should reject invalid MON event', () => {
    expect(() => parseMonitorEvent('MON')).toThrow('Protocol error. Invalid MONITOR event \'MON\' received.');
  });
});
