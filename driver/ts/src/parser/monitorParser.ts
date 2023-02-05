import { MonitorEvent } from '../types/monitorEvent';

export const parseMonitorEvent = (event: string): MonitorEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'MONITOR') {
    return undefined;
  }

  if (terms.length < 2) {
    throw new Error(`Protocol error. Invalid MONITOR event '${event}' received.`);
  }
  const attributes = terms.slice(1);

  const data = attributes[0];
  try {
    return {
      type: 'MonitorEvent',
      econetFrame: Buffer.from(data, 'base64'),
    };
  } catch (e) {
    throw new Error(`Protocol error. Invalid MONITOR event '${event}' received. Failed to parse base64 data.`);
  }
};
