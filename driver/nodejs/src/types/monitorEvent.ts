import { hexdump } from '@gct256/hexdump';
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

  public toString() {
    return (
      this.titleForFrame(this.econetFrame) +
      '        ' +
      hexdump(this.econetFrame).join('\n        ')
    );
  }
}
