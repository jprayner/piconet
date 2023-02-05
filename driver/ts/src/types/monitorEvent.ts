export enum RxState {
  Stop,
  Listen,
  Monitor,
}

export type MonitorEvent = {
  type: 'MonitorEvent';
  econetFrame: Buffer;
};
