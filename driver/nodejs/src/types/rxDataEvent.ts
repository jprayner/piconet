import { EconetEvent } from './econetEvent';

/**
 * Superclass for events emitted by the Econet driver in response to incoming data.
 */
export class RxDataEvent extends EconetEvent {
  protected titleForFrame(frame: Buffer) {
    const toStation = frame[0];
    const toNet = frame[1];
    const fromStation = frame[2];
    const fromNet = frame[3];
    return `${this.constructor.name} ${fromNet}.${fromStation} --> ${toNet}.${toStation}\n`;
  }
}
