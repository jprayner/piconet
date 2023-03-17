import { ErrorEvent } from '../types/errorEvent';

export const parseErrorEvent = (event: string): ErrorEvent | undefined => {
  if (event.indexOf('ERROR ') !== 0) {
    return undefined;
  }

  const description = event.substring('ERROR '.length);
  return new ErrorEvent(description);
};
