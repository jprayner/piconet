import { areVersionsCompatible, parseSemver } from './semver';

describe('semver', () => {
  it('should parse valid string', () => {
    const result = parseSemver('1.2.3');
    expect(result.major).toBe(1);
    expect(result.minor).toBe(2);
    expect(result.patch).toBe(3);
  });

  it('should fail parse if invalid', () => {
    expect(() => parseSemver('1.2')).toThrow('Invalid version string \'1.2\'');
  });

  it('should recognise identical semvers as being compatible', () => {
    expect(areVersionsCompatible(parseSemver('1.2.3'), parseSemver('1.2.3'))).toBe(true);
  });

  it('should recognise semvers with differing patch versions as being compatible', () => {
    expect(areVersionsCompatible(parseSemver('1.2.3'), parseSemver('1.2.4'))).toBe(true);
  });

  it('should recognise semvers with differing minor revisions as being incompatible', () => {
    expect(areVersionsCompatible(parseSemver('1.2.3'), parseSemver('1.3.3'))).toBe(false);
  });

  it('should recognise semvers with differing major revisions as being incompatible', () => {
    expect(areVersionsCompatible(parseSemver('1.2.3'), parseSemver('2.2.3'))).toBe(false);
  });
});
