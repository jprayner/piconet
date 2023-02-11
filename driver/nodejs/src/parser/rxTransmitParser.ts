import { RxTransmitEvent } from '../types/rxTransmitEvent';

export const parseRxTransmitEvent = (
  event: string,
): RxTransmitEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'RX_TRANSMIT') {
    return undefined;
  }

  if (terms.length < 4) {
    throw new Error(
      `Protocol error. Invalid RX_TRANSMIT event '${event}' received.`,
    );
  }
  const attributes = terms.slice(1);

  return {
    type: 'RxTransmitEvent',
    receiveId: parseInt(attributes[0], 10),
    scoutFrame: Buffer.from(attributes[1], 'base64'),
    dataFrame: Buffer.from(attributes[2], 'base64'),
  };
};
