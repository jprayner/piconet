export enum RxState {
  Stop,
  Listen,
  Monitor,
}

export type BoardState = {
  driverVersion: string;
  firmwareVersion: string;
  econetStation: number;
  statusRegister1: number;
  rxState: RxState;
};
