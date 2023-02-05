import { hexDump } from './hexDump';

describe('hexDump', () => {
  it('should dump short buffer', () => {
    const buf1 = Buffer.from('this is a tést');
    const result = hexDump(buf1);
    console.log(result);
    expect(result).toEqual('00000000 | 74 68 69 73 20 69 73 20 61 20 74 c3 a9 73 74       | this is a test');
  });
});
