import { StatusEvent, RxMode } from '../types/statusEvent';
import { parseSemver } from '../driver/semver';

export const parseStatusEvent = (event: string): StatusEvent | undefined => {
  const terms = event.split(' ');

  if (terms.length == 0 || terms[0] !== 'STATUS') {
    return undefined;
  }

  if (terms.length < 2) {
    throw new Error(
      `Protocol error. Invalid STATUS event '${event}' received.`,
    );
  }
  const attributes = terms.slice(1);

  const firmwareVersionStr = attributes[0];
  try {
    parseSemver(firmwareVersionStr);
  } catch (e) {
    throw new Error(
      `Protocol error. Invalid STATUS event '${event}' received. Board reports invalid version '${firmwareVersionStr}'.`,
    );
  }

  if (attributes.length !== 4) {
    throw new Error(
      `Protocol error. Invalid STATUS event '${event}' received. Expected 4 attributes, got ${attributes.length}`,
    );
  }

  const econetStationStr = attributes[1];
  if (
    isNaN(parseInt(econetStationStr, 10)) ||
    parseInt(econetStationStr, 10) < 0 ||
    parseInt(econetStationStr, 10) > 255
  ) {
    throw new Error(
      `Protocol error. Invalid STATUS event '${event}' received. Invalid econet station '${econetStationStr}'.`,
    );
  }

  const statusRegister1Str = attributes[2];
  if (
    isNaN(parseInt(statusRegister1Str, 16)) ||
    parseInt(statusRegister1Str, 16) < 0x00 ||
    parseInt(statusRegister1Str, 16) > 0xff
  ) {
    throw new Error(
      `Protocol error. Invalid STATUS event '${event}' received. Invalid status register 1 value '${statusRegister1Str}'.`,
    );
  }

  const boardStateStr = attributes[3];
  let rxState: RxMode | undefined = undefined;
  switch (parseInt(boardStateStr, 10)) {
    case 0:
      rxState = RxMode.STOP;
      break;
    case 1:
      rxState = RxMode.LISTEN;
      break;
    case 2:
      rxState = RxMode.MONITOR;
      break;
    default:
      throw new Error(
        `Protocol error. Invalid STATUS event '${event}' received. Invalid board state value '${boardStateStr}'.`,
      );
  }

  return new StatusEvent(
    firmwareVersionStr,
    parseInt(econetStationStr, 10),
    parseInt(statusRegister1Str, 16),
    rxState);
};
