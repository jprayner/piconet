export type Semver = {
  major: number;
  minor: number;
  patch: number;
};

export const parseSemver = (version: string): Semver => {
  const parts = version.split('.');
  if (parts.length !== 3 || parts.some(part => isNaN(parseInt(part, 10)))) {
    throw new Error(`Invalid version string '${version}'`);
  }

  return {
    major: parseInt(parts[0], 10),
    minor: parseInt(parts[1], 10),
    patch: parseInt(parts[2], 10),
  };
};

export const areVersionsCompatible = (
  versionA: Semver,
  versionB: Semver,
): boolean => {
  return versionA.major === versionB.major && versionA.minor === versionB.minor;
};
