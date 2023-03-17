const defaultExtends = [
  'airbnb-typescript/base',
  'eslint:recommended',
  'plugin:jest/recommended',
  'plugin:@typescript-eslint/eslint-recommended',
  'plugin:@typescript-eslint/recommended',
  'plugin:@typescript-eslint/recommended-requiring-type-checking',
];

const defaultRules = {
  '@typescript-eslint/no-use-before-define': 'off',
  '@typescript-eslint/indent': 'off',
  'import/prefer-default-export': 'off',
  'import/no-extraneous-dependencies': ['error', { devDependencies: true }],
  'import/extensions': [
    'error',
    'ignorePackages',
    {
      js: 'never',
      ts: 'never',
    },
  ],
};

module.exports = {
  parser: '@typescript-eslint/parser',
  plugins: ['import', 'jest'],
  extends: defaultExtends,
  rules: defaultRules,
  overrides: [
    {
      files: '*.ts',
      parserOptions: {
        project: "tsconfig.json",
        tsconfigRootDir: __dirname,
      },
    },
    {
      files: '*.test.ts',
      parserOptions: {
        project: "tsconfig.json",
        tsconfigRootDir: __dirname,
      },
      rules: {
        ...defaultRules,
        '@typescript-eslint/unbound-method': 'off',
      },
    },
  ],
  env: {
    jest: true,
    node: true,
    es6: true,
  },
};
