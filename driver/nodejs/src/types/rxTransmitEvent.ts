export type RxTransmitEvent = {
  type: 'RxTransmitEvent';
  scoutFrame: Buffer;
  dataFrame: Buffer;
};
