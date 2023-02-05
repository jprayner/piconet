import config from '../config';
import { RxMode } from '../types/statusEvent';
import { parseStatusEvent } from './statusParser';

describe('status message parser', () => {
  it('should parse valid string', () => {
    const eventStr = 'STATUS 0.1.1 32 10 1';
    const result = parseStatusEvent(eventStr);
    expect(result).toBeDefined();
    expect(result?.firmwareVersion).toBe('0.1.1');
    expect(result?.econetStation).toBe(32);
    expect(result?.statusRegister1).toBe(16);
    expect(result?.rxMode).toBe(RxMode.Listening);
  });

  it('should return no match (undefined) due to empty string', () => {
    const eventStr = '';
    expect(parseStatusEvent(eventStr)).toBeUndefined();
  });

  it('should return no match (undefined) due to non-match on event name', () => {
    const eventStr = 'MON abcdef123';
    expect(parseStatusEvent(eventStr)).toBeUndefined();
  });

  it('should fail to parse due to no attributes', () => {
    const eventStr = 'STATUS';
    expect(() => parseStatusEvent(eventStr)).toThrow(`Protocol error. Invalid STATUS event '${eventStr}' received`);
  });

  it('should fail to parse due to invalid semver in firmware version', () => {
    const eventStr = 'STATUS x.1.1 32 10 1';
    expect(() => parseStatusEvent(eventStr)).toThrow(`Protocol error. Invalid STATUS event '${eventStr}' received`);
  });

  it('should fail to parse due to wrong number of attibutes', () => {
    const eventStr = `STATUS ${config.version} 32 10 1 i_shouldnt_be_here`;
    expect(() => parseStatusEvent(eventStr)).toThrow(`Protocol error. Invalid STATUS event '${eventStr}' received. Expected 4 attributes, got 5`);
  });

  it('should fail to parse due to invalid econet station number', () => {
    const eventStr = `STATUS ${config.version} xxx 10 1`;
    expect(() => parseStatusEvent(eventStr)).toThrow(`Protocol error. Invalid STATUS event '${eventStr}' received. Invalid econet station 'xxx'.`);
  });

  it('should fail to parse due to invalid status register value', () => {
    const eventStr = `STATUS ${config.version} 32 xxx 1`;
    expect(() => parseStatusEvent(eventStr)).toThrow(`Protocol error. Invalid STATUS event '${eventStr}' received. Invalid status register 1 value 'xxx'.`);
  });
});
