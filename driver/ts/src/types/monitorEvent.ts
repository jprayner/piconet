export enum RxState {
  Stop,
  Listen,
  Monitor,
}

export type MonitorEvent = {
  type: 'monitor';
  econetFrame: Buffer;
};
