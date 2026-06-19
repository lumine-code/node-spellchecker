const path = require('path');
const fs = require('fs');
const Promise = require('any-promise');
const bindings = require('../build/Release/spellchecker.node');

const Spellchecker = bindings.Spellchecker;

const checkSpellingAsyncCb = Spellchecker.prototype.checkSpellingAsync

Spellchecker.prototype.checkSpellingAsync = function (corpus) {
  return new Promise(function (resolve, reject) {
    checkSpellingAsyncCb.call(this, corpus, function (err, result) {
      if (err) {
        reject(err);
      } else {
        resolve(result);
      }
    });
  }.bind(this));
};

let defaultSpellcheck = null;

function ensureDefaultSpellCheck () {
  if (!defaultSpellcheck) return;

  let lang = process.env.LANG || undefined;
  lang = lang?.split('.')[0] ?? 'en_US';
  defaultSpellcheck = new Spellchecker();
  setDictionary(lang, getDictionaryPath());
  return defaultSpellcheck;
}

function setDictionary (lang, dictPath) {
  return ensureDefaultSpellCheck().setDictionary(lang, dictPath);
}

function isMisspelled (...args) {
  return ensureDefaultSpellCheck().isMisspelled(...args);
}

function checkSpelling (...args) {
  return ensureDefaultSpellCheck().checkSpelling(...args);
}

function checkSpellingAsync (...args) {
  return ensureDefaultSpellCheck().checkSpellingAsync(...args);
}

function add (...args) {
  return ensureDefaultSpellCheck().add(...args);
}

function remove (...args) {
  return ensureDefaultSpellCheck().remove(...args);
}

function getCorrectionsForMisspelling (...args) {
  return ensureDefaultSpellCheck().getCorrectionsForMisspelling(...args);
}

function getAvailableDictionaries (...args) {
  return ensureDefaultSpellCheck().getAvailableDictionaries(...args);
}

function getDictionaryPath () {
  let dict = path.join(__dirname, '..', 'vendor', 'hunspell_dictionaries');
  try {
    let unpacked = dict.replace(`.asar${path.sep}`, `.asar.unpacked${path.sep}`);
    if (fs.statSync(unpacked)) {
      dict = unpacked;
    }
  } catch (error) {
    // When the dictionary isn't contained within an .asar, return the original
    // path.
  }
  return dict;
}


module.exports = {
  setDictionary,
  add,
  remove,
  isMisspelled,
  checkSpelling,
  checkSpellingAsync,
  getAvailableDictionaries,
  getCorrectionsForMisspelling,
  getDictionaryPath,
  Spellchecker,
  USE_SYSTEM_DEFAULTS: 0,
  ALWAYS_USE_SYSTEM: 1,
  ALWAYS_USE_HUNSPELL: 2,
};
