// credit https://gist.github.com/Vbitz/ea301a2797e73616af8048d80f9d1aef

export const hexDump = (buff: Buffer): string => {
  let x = 0;

  let lineHex = '';
  let lineAscii = '';
  
  let ret = '';

  let offset = 0;

  for (const value of buff) {
    lineHex += value.toString(16).padStart(2, '0') + ' ';
    lineAscii += printValue(value);

    x += 1;

    if (x > 16) {
      ret += `${offset
        .toString(16)
        .padStart(8, '0')} | ${lineHex.trim()} | ${lineAscii}\n`;

      lineHex = '';
      lineAscii = '';

      x = 0;

      offset += 16;
    }
  }

  ret += `${offset.toString(16).padStart(8, '0')} | ${lineHex
    .trim()
    .padEnd(3 * 16 + 2, ' ')} | ${lineAscii}\n`;

  return ret;
};

const printValue = (value: number): string => {
  if (value >= 0x20 && value <= 0x7e) {
    return String.fromCharCode(value);
  } else {
    return '.';
  }
};
