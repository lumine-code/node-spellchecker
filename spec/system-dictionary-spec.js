"use strict";

const { Spellchecker, ALWAYS_USE_SYSTEM } = require("../lib/spellchecker");

// The system service exists on Windows and macOS; Linux has Hunspell only.
const hasSystemService = process.platform === "win32" || process.platform === "darwin";

describe("setDictionary with no language", () => {
  it("selects a system default rather than failing", () => {
    if (!hasSystemService) {
      pending("no system spelling service on this platform");
      return;
    }

    const checker = new Spellchecker();
    checker.setSpellcheckerType(ALWAYS_USE_SYSTEM);

    // An empty language means "whatever the system is set to". Both platforms
    // that have a system service honour it.
    expect(checker.setDictionary("", "")).toBe(true);
    expect(checker.isMisspelled("mispelled")).toBe(true);
    expect(checker.isMisspelled("misspelled")).toBe(false);
  });

  it("still rejects a language the system does not have", () => {
    if (!hasSystemService) {
      pending("no system spelling service on this platform");
      return;
    }

    const checker = new Spellchecker();
    checker.setSpellcheckerType(ALWAYS_USE_SYSTEM);

    expect(checker.setDictionary("zz-ZZ", "")).toBe(false);
  });
});
