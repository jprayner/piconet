import { RxImmediateEvent } from '../types/rxImmediateEvent';

export const parseRxImmediateEvent = (event: string): RxImmediateEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'RX_IMMEDIATE') {
    return undefined;
  }

  if (terms.length < 3) {
    throw new Error(`Protocol error. Invalid RX_IMMEDIATE event '${event}' received.`);
  }
  const attributes = terms.slice(1);

  const scout = attributes[0];
  const data = attributes[1];
  try {
    return {
      type: 'RxImmediateEvent',
      scoutFrame: Buffer.from(scout, 'base64'),
      dataFrame: Buffer.from(data, 'base64'),
    };
  } catch (e) {
    throw new Error(`Protocol error. Invalid RX_IMMEDIATE event '${event}' received. Failed to parse base64 data.`);
  }
};
