export type TransmitEvent = {
  type: 'transmit';
  scoutFrame: Buffer;
  dataFrame: Buffer;
};
