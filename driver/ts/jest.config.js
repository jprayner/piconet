module.exports = {
  ...require('./jest.config.base'),
  roots: ['<rootDir>'],
  testPathIgnorePatterns: ['/node_modules/', '/dist/', '/src/api/*'],
};
