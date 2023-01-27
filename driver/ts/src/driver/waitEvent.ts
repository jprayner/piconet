import { MonitorEvent } from '../types/monitorEvent';
import { StatusEvent } from '../types/statusEvent';
import { Connection, Listener } from './connection';

export type EventMatcher = (event: StatusEvent | MonitorEvent) => boolean;

export const waitForEvent = async (connection: Connection, matcher: EventMatcher, timeoutMs: number): Promise<StatusEvent | MonitorEvent> => {
  return new Promise((resolve, reject) => {
    // eslint-disable-next-line prefer-const
    let listener: Listener;

    const timer = setTimeout(() => {
      if (listener) {
        connection.removeListener(listener);
      }
      reject(new Error(`No matching event found within ${timeoutMs}ms`));
    }, timeoutMs);

    listener = (event: StatusEvent | MonitorEvent) => {
      console.log(`waitevent received event: ${JSON.stringify(event)}`);
      if (!matcher(event)) {
        console.log('nomatch');
        return;
      }
      console.log('match');

      connection.removeListener(listener);
      clearTimeout(timer);
      resolve(event);
    };
    connection.addListener(listener);
  });
};
