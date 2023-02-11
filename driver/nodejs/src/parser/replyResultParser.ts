import { ReplyResultEvent } from '../types/replyResultEvent';

export const parseReplyResultEvent = (
  event: string,
): ReplyResultEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'REPLY_RESULT') {
    return undefined;
  }

  if (terms.length < 2) {
    throw new Error(
      `Protocol error. Invalid REPLY_RESULT event '${event}' received.`,
    );
  }
  const attributes = terms.slice(1);

  const result = attributes[0];
  return {
    type: 'ReplyResultEvent',
    result,
  };
};
