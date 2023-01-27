import { MonitorEvent } from '../types/monitorEvent';

export const parseMonitorEvent = (event: string): MonitorEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'MON') {
    return undefined;
  }

  if (terms.length < 2) {
    throw new Error(`Protocol error. Invalid MONITOR event '${event}' received.`);
  }
  const attibutes = terms.slice(1);

  const data = attibutes[0];
  try {
    return {
      type: 'monitor',
      econetFrame: Buffer.from(data, 'base64'),
    };
  } catch (e) {
    throw new Error(`Protocol error. Invalid MONITOR event '${event}' received. Failed to parse base64 data.`);
  }
};
