import { ImmediateEvent } from '../types/immediateEvent';

export const parseImmediateEvent = (event: string): ImmediateEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'IMMEDIATE') {
    return undefined;
  }

  if (terms.length < 3) {
    throw new Error(`Protocol error. Invalid IMMEDIATE event '${event}' received.`);
  }
  const attibutes = terms.slice(1);

  const scout = attibutes[0];
  const data = attibutes[1];
  try {
    return {
      type: 'immediate',
      scoutFrame: Buffer.from(scout, 'base64'),
      dataFrame: Buffer.from(data, 'base64'),
    };
  } catch (e) {
    throw new Error(`Protocol error. Invalid IMMEDIATE event '${event}' received. Failed to parse base64 data.`);
  }
};
