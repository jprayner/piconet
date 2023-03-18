import { hexdump } from '@gct256/hexdump';
import { RxDataEvent } from './rxDataEvent';

/**
 * Fired asynchronously whilst in `LISTEN` mode as broadcast packets are received.
 */
export class RxBroadcastEvent extends RxDataEvent {
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
