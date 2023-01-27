import { BroadcastEvent } from '../types/broadcastEvent';

export const parseBroadcastEvent = (event: string): BroadcastEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'BROADCAST') {
    return undefined;
  }

  if (terms.length < 2) {
    throw new Error(`Protocol error. Invalid BROADCAST event '${event}' received.`);
  }
  const attributes = terms.slice(1);

  const data = attributes[0];
  try {
    return {
      type: 'broadcast',
      econetFrame: Buffer.from(data, 'base64'),
    };
  } catch (e) {
    throw new Error(`Protocol error. Invalid BROADCAST event '${event}' received. Failed to parse base64 data.`);
  }
};
