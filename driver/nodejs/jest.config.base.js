module.exports = {
  testEnvironment: 'node',
  testMatch: ['**/*.test.ts', '**/*.test.js'],
  automock: false,
  resetMocks: false,
  transform: {
    '^.+\\.(t|j)sx?$': ['ts-jest'],
  },
  globals: {
    'ts-jest': {
      isolatedModules: true,
    },
  },
  setupFiles: ['./jest.setup.js'],
};
