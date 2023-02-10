export enum RxMode {
  Stopped,
  Listening,
  Monitoring,
}

export type StatusEvent = {
  type: 'status';
  firmwareVersion: string;
  econetStation: number;
  statusRegister1: number;
  rxMode: RxMode;
};
