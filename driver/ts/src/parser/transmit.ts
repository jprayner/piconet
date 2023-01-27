import { TransmitEvent } from '../types/transmitEvent';

export const parseTransmitEvent = (event: string): TransmitEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'TRANSMIT') {
    return undefined;
  }

  if (terms.length < 3) {
    throw new Error(`Protocol error. Invalid TRANSMIT event '${event}' received.`);
  }
  const attributes = terms.slice(1);

  const scout = attributes[0];
  const data = attributes[1];
  try {
    return {
      type: 'transmit',
      scoutFrame: Buffer.from(scout, 'base64'),
      dataFrame: Buffer.from(data, 'base64'),
    };
  } catch (e) {
    throw new Error(`Protocol error. Invalid TRANSMIT event '${event}' received. Failed to parse base64 data.`);
  }
};
