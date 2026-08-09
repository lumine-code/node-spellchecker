const js = require("@eslint/js");
const n = require("eslint-plugin-n");
const globals = require("globals");
const prettier = require("eslint-config-prettier");

module.exports = [
  { ignores: ["node_modules/**", "build/**", "vendor/**"] },
  js.configs.recommended,
  n.configs["flat/recommended-script"],
  {
    settings: { n: { version: ">=24.0.0" } },
    languageOptions: {
      ecmaVersion: "latest",
      sourceType: "commonjs",
      globals: { ...globals.node },
    },
    rules: {
      "no-unused-vars": ["error", { varsIgnorePattern: "^_", argsIgnorePattern: "^_" }],
    },
  },
  {
    files: ["lib/spellchecker.js"],
    rules: {
      "n/no-missing-require": "off",
      "n/no-unpublished-require": "off",
    },
  },
  {
    files: ["spec/**"],
    languageOptions: { globals: { ...globals.jasmine } },
    rules: {
      "n/no-unpublished-require": "off",
      "n/no-extraneous-require": "off",
    },
  },
  prettier,
];
