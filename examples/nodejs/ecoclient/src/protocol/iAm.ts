import { executeCliCommand } from '../common';

export const iAm = async (
  serverStation: number,
  username: string,
  password: string,
) => {
  const serverReply = await executeCliCommand(
    serverStation,
    `I AM ${username} ${password}`,
  );

  if (serverReply.data.length < 4) {
    throw new Error(
      `Malformed response from station ${serverStation}: success but not enough data`,
    );
  }

  return {
    handleCurrentDir: serverReply.data[0],
    handleUserRootDir: serverReply.data[1],
    handleLibDir: serverReply.data[2],
    bootOption: serverReply.data[3],
  };
};
