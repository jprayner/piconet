import { EconetEvent } from './econetEvent';

/**
 * Fired asynchronously whilst in `LISTEN` mode as `TRANSMIT` operation packets are received for the
 * local Econet station.
 */
export class RxTransmitEvent extends EconetEvent {
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
