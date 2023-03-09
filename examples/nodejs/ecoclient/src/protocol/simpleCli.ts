import { driver } from "@jprayner/piconet-nodejs";
import { controlByte, directoryHandles, executeCliCommand, port, replyPort, standardTxMessage, stripCRs, waitForReceiveTxEvent } from "../common";

export const bye = async (serverStation: number) => executeCliCommand(serverStation, 'BYE');
export const cdir = async (serverStation: number, dirName: string) => executeCliCommand(serverStation, `CDIR ${dirName}`);
export const deleteFile = async (serverStation: number, filePath: string) => executeCliCommand(serverStation, `DELETE ${filePath}`);
export const access = async (serverStation: number, filePath: string, accessString: string) => executeCliCommand(serverStation, `ACCESS ${filePath} ${accessString}`);
