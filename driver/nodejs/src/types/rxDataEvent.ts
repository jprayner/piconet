import { EconetEvent } from './econetEvent';

/**
 * Superclass for all events emitted by the Econet driver in response to incoming data:
 *
 * * {@link RxImmediateEvent}
 * * {@link RxTransmitEvent}
 * * {@link RxBroadcastEvent}
 * * {@link MonitorEvent}
 */
export class RxDataEvent extends EconetEvent {}
