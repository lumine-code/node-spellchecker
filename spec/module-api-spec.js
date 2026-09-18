const fs = require("node:fs");
const path = require("node:path");

const modulePath = require.resolve("../lib/spellchecker");
const dictionaryDirectory = path.join(__dirname, "dictionaries");

function loadFreshSpellchecker() {
  delete require.cache[modulePath];
  return require(modulePath);
}

describe("the module-level spellchecker", () => {
  let spellchecker;

  beforeEach(() => {
    spellchecker = loadFreshSpellchecker();
  });

  afterEach(() => {
    delete require.cache[modulePath];
  });

  it("lazily constructs itself on first use", () => {
    expect(typeof spellchecker.isMisspelled("cheese")).toBe("boolean");
  });

  it("reuses the same instance across calls", () => {
    if (process.platform === "win32") return;

    expect(spellchecker.setDictionary("en_US", dictionaryDirectory)).toBe(true);
    expect(spellchecker.isMisspelled("wwoorrdd")).toBe(true);

    spellchecker.add("wwoorrdd");
    expect(spellchecker.isMisspelled("wwoorrdd")).toBe(false);

    spellchecker.remove("wwoorrdd");
    expect(spellchecker.isMisspelled("wwoorrdd")).toBe(true);
  });

  it("checks spelling through the singleton", () => {
    expect(spellchecker.setDictionary("en_US", dictionaryDirectory)).toBe(true);
    expect(spellchecker.checkSpelling("cat caat dog dooog")).toEqual([
      { start: 4, end: 8 },
      { start: 13, end: 18 },
    ]);
  });

  it("checks spelling asynchronously through the singleton", async () => {
    expect(spellchecker.setDictionary("en_US", dictionaryDirectory)).toBe(true);
    await expectAsync(spellchecker.checkSpellingAsync("cat caat dog dooog")).toBeResolvedTo([
      { start: 4, end: 8 },
      { start: 13, end: 18 },
    ]);
  });

  it("returns corrections through the singleton", () => {
    expect(spellchecker.setDictionary("en_US", dictionaryDirectory)).toBe(true);
    const corrections = spellchecker.getCorrectionsForMisspelling("worrd");
    expect(Array.isArray(corrections)).toBe(true);
    expect(corrections.length).toBeGreaterThan(0);
  });

  it("resolves the bundled dictionary directory", () => {
    const dictionaryPath = spellchecker.getDictionaryPath();
    expect(typeof dictionaryPath).toBe("string");
    expect(fs.existsSync(path.join(dictionaryPath, "en_US.dic"))).toBe(true);
  });
});
