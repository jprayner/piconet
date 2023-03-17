import { RxDataEvent } from './rxDataEvent';

/**
 * Fired asynchronously whilst in `LISTEN` mode as `IMMEDIATE` operation packets are received for the
 * local Econet station.
 */
export class RxImmediateEvent extends RxDataEvent {
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
}
