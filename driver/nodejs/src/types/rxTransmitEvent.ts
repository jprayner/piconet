import { hexdump } from '@gct256/hexdump';
import { RxDataEvent } from './rxDataEvent';

/**
 * Fired asynchronously whilst in `LISTEN` mode as `TRANSMIT` operation packets are received for the
 * local Econet station.
 */
export class RxTransmitEvent extends RxDataEvent {
  constructor(
    /**
     * The raw scout frame.
     */
    public scoutFrame: Buffer,
    /**
     * The raw data frame.
     */
    public dataFrame: Buffer,
  ) {
    super();
  }

  public toString() {
    return (
      this.titleForFrame(this.scoutFrame) +
      '[SCOUT] ' +
      hexdump(this.scoutFrame).join('\n        ') +
      '\n[DATA]  ' +
      hexdump(this.dataFrame).join('\n        ')
    );
  }
}
