import { RxDataEvent } from './rxDataEvent';

/**
 * Fired asynchronously as frames are received by the ADLC whilst in `MONITOR` mode.
 * 
 * This event is fired regardless of the source or destination of the frame.
 */
export class MonitorEvent extends RxDataEvent {
  constructor(
    /**
     * The raw Econet frame.
     */
    public econetFrame: Buffer,
  ) {
    super();
  }
}
