import { BoardState, RxState } from '../types/statusEvent';
import { areVersionsCompatible, parseSemver } from './semver';
import config from './config';

export const parseEvent = (event: string): BoardState | undefined => {
  if (event.startsWith('STATUS ')) {
    // STATUS 0.1.1 32 04 0

    const terms = event.split(' ');
    if (terms.length < 2) {
      throw new Error(`Protocol error. Invalid STATUS event '${event}' received.`);
    }
    const attibutes = terms.slice(1);

    const driverVersionStr = config.version;
    const boardVersionStr = attibutes[0];
    try {
      parseSemver(boardVersionStr);
    } catch (e) {
      throw new Error(`Protocol error. Invalid STATUS event '${event}' received. Board reports invalid version '${boardVersionStr}'.`);
    }

    const boardVersion = parseSemver(boardVersionStr);
    const driverVersion = parseSemver(driverVersionStr);
    if (!areVersionsCompatible(boardVersion, driverVersion)) {
      throw new Error(`Driver version ${driverVersionStr} is not compatible with board version ${boardVersionStr}.`);
    }

    if (attibutes.length !== 4) {
      throw new Error(`Protocol error. Invalid STATUS event '${event}' received. Expected 4 attributes, got ${attibutes.length}`);
    }

    const econetStationStr = attibutes[1];
    if (isNaN(parseInt(econetStationStr, 10))
        || parseInt(econetStationStr, 10) < 0
        || parseInt(econetStationStr, 10) > 255) {
      throw new Error(`Protocol error. Invalid STATUS event '${event}' received. Invalid econet station '${econetStationStr}'.`);
    }

    const statusRegister1Str = attibutes[2];
    if (
        isNaN(parseInt(statusRegister1Str, 16))
        || parseInt(statusRegister1Str, 16) < 0x00
        || parseInt(statusRegister1Str, 16) > 0xff) {
      throw new Error(`Protocol error. Invalid STATUS event '${event}' received. Invalid status register 1 value '${statusRegister1Str}'.`);
    }

    const boardStateStr = terms[3];
    let boardState: RxState | undefined = undefined;
    switch (parseInt(boardStateStr, 10)) {
      case 0:
        boardState = RxState.Stop;
        break;
      case 1:
        boardState = RxState.Listen;
        break;
      case 2:
        boardState = RxState.Monitor;
        break;
      default:
        throw new Error(`Protocol error. Invalid STATUS event '${event}' received. Invalid board state value '${boardStateStr}'.`);
    }

    return {
      driverVersion: driverVersionStr,
      firmwareVersion: boardVersionStr,
      econetStation: parseInt(econetStationStr, 10),
      rxState: boardState,
    };
  }

  return undefined;
};