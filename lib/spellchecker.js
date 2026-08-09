"use strict";

const fs = require("node:fs");
const path = require("node:path");
const bindings = require("../build/Release/spellchecker.node");

const { Spellchecker } = bindings;
const checkSpellingAsyncCallback = Spellchecker.prototype.checkSpellingAsyncCallback;

Spellchecker.prototype.checkSpellingAsync = function checkSpellingAsync(corpus) {
  return new Promise((resolve, reject) => {
    checkSpellingAsyncCallback.call(this, corpus, (error, result) => {
      if (error) reject(error);
      else resolve(result);
    });
  });
};

let defaultSpellchecker = null;

function getDictionaryPath() {
  let dictionaryPath = path.join(__dirname, "..", "vendor", "hunspell_dictionaries");
  const unpackedPath = dictionaryPath.replace(`.asar${path.sep}`, `.asar.unpacked${path.sep}`);
  if (fs.existsSync(unpackedPath)) dictionaryPath = unpackedPath;
  return dictionaryPath;
}

function ensureDefaultSpellchecker() {
  if (defaultSpellchecker) return defaultSpellchecker;
  const language = process.env.LANG?.split(".")[0] || "en_US";
  defaultSpellchecker = new Spellchecker();
  defaultSpellchecker.setDictionary(language, getDictionaryPath());
  return defaultSpellchecker;
}

module.exports = {
  setDictionary(language, dictionaryPath) {
    return ensureDefaultSpellchecker().setDictionary(language, dictionaryPath);
  },
  add(...args) {
    return ensureDefaultSpellchecker().add(...args);
  },
  remove(...args) {
    return ensureDefaultSpellchecker().remove(...args);
  },
  isMisspelled(...args) {
    return ensureDefaultSpellchecker().isMisspelled(...args);
  },
  checkSpelling(...args) {
    return ensureDefaultSpellchecker().checkSpelling(...args);
  },
  checkSpellingAsync(...args) {
    return ensureDefaultSpellchecker().checkSpellingAsync(...args);
  },
  getAvailableDictionaries(...args) {
    return ensureDefaultSpellchecker().getAvailableDictionaries(...args);
  },
  getCorrectionsForMisspelling(...args) {
    return ensureDefaultSpellchecker().getCorrectionsForMisspelling(...args);
  },
  getDictionaryPath,
  Spellchecker,
  USE_SYSTEM_DEFAULTS: 0,
  ALWAYS_USE_SYSTEM: 1,
  ALWAYS_USE_HUNSPELL: 2,
};
