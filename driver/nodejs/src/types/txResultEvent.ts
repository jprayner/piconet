import { EconetEvent } from './econetEvent';

/**
 * Generated in response to a `TRANSMIT` command.
 */
export class TxResultEvent extends EconetEvent {
  constructor(
    /**
     * `true` if the `TRANSMIT` operation was successful or `false` if it failed.
     */
    public success: boolean,

    /**
     * A description of the result:
     *
     * `OK` — Packet was successfully sent and acknowledged
     *
     * `UNINITIALISED` — Firmware issue — should never happen
     *
     * `OVERFLOW` — Indicates that size of message exceeds TX_DATA_BUFFER_SZ
     *
     * `UNDERRUN` — Indicates that firmware is failing to keep up with ADLC when sending data
     *
     * `LINE_JAMMED` — Suggests that a station is 'flag-filling' and preventing transmission
     *
     * `NO_SCOUT_ACK` — Remote station failed to acknowledge scout frame (disconnected or not listening on port?) — consider retrying
     *
     * `NO_DATA_ACK` — Remote station failed to acknowledge data frame — consider retrying
     *
     * `TIMEOUT` — Other timeout condition e.g. in communication with ADLC
     *
     * `MISC` — Logic error e.g. in protocol decode
     *
     * `UNEXPECTED` — Firmware issue — should never happen
     */
    public description: string,
  ) {
    super();
  }

  public toString() {
    return `[${this.constructor.name} success=${
      this.success ? 'true' : 'false'
    } description='${this.description}']`;
  }
}
