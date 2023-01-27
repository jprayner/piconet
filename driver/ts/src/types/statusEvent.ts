export enum RxMode {
  Stopped,
  Listening,
  Monitoring,
}

export type StatusEvent = {
  type: 'status';
  driverVersion: string;
  firmwareVersion: string;
  econetStation: number;
  statusRegister1: number;
  rxMode: RxMode;
};
