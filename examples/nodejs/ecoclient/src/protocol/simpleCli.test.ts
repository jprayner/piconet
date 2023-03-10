import { controlByte, executeCliCommand, port } from '../common';
import { access, bye, cdir, deleteFile } from './simpleCli';

jest.mock('../common');

const executeCliCommandMock = jest.mocked(executeCliCommand);

describe('simpleCli protocol handler', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it(`should call executeCliCommand with the correct arguments for 'BYE' command `, async () => {
    await bye(254);
    expect(executeCliCommandMock).toHaveBeenCalledWith(254, 'BYE');
  });

  it(`should call executeCliCommand with the correct arguments for 'CDIR' command `, async () => {
    await cdir(254, 'test');
    expect(executeCliCommandMock).toHaveBeenCalledWith(254, 'CDIR test');
  });

  it(`should call executeCliCommand with the correct arguments for 'DELETE' command `, async () => {
    await deleteFile(254, 'test');
    expect(executeCliCommandMock).toHaveBeenCalledWith(254, 'DELETE test');
  });

  it(`should call executeCliCommand with the correct arguments for 'ACCESS' command `, async () => {
    await access(254, 'test', 'WR/R');
    expect(executeCliCommandMock).toHaveBeenCalledWith(254, 'ACCESS test WR/R');
  });
});
