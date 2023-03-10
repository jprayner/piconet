module.exports = {
  testEnvironment: 'node',
  testMatch: ['**/*.test.ts'],
  automock: false,
  resetMocks: false,
  transform: {
    '^.+\\.tsx?$': ['ts-jest'],
  },
  globals: {
    'ts-jest': {
      isolatedModules: true,
    },
  },
  setupFiles: ['./jest.setup.js'],
};
