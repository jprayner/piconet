import { TxResultEvent } from '../types/txResultEvent';

export const parseTxResultEvent = (
  event: string,
): TxResultEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'TX_RESULT') {
    return undefined;
  }

  if (terms.length < 2) {
    throw new Error(
      `Protocol error. Invalid TX_RESULT event '${event}' received.`,
    );
  }
  const attributes = terms.slice(1);

  const result = attributes[0];
  return {
    type: 'TxResultEvent',
    result,
  };
};
