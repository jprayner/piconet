/**
 * Fired by the firmware to indicate that an error has occurred.
 *
 * These events may be fired at any time, even if a command is not in-progress (for example, if a
 * received packet fails to parse).
 */
export class ErrorEvent {
  constructor(
    /**
     * A human-readable description of the error.
     */
    public description: string,
  ) {}

  public toString() {
    return `[${this.constructor.name} description='${this.description}']`;
  }
}
