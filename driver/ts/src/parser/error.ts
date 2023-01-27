import { ErrorEvent } from '../types/errorEvent';

export const parseErrorEvent = (event: string): ErrorEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'ERROR') {
    return undefined;
  }

  if (terms.length < 2) {
    throw new Error(`Protocol error. Invalid ERROR event '${event}' received.`);
  }
  const attibutes = terms.slice(1);

  const errorDesc = attibutes[0];
  try {
    return {
      type: 'error',
      description: errorDesc
    };
  } catch (e) {
    throw new Error(`Protocol error. Invalid ERROR event '${event}' received. Failed to parse base64 data.`);
  }
};
