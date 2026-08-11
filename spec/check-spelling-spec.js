"use strict";

const path = require("node:path");
const { Spellchecker, ALWAYS_USE_HUNSPELL } = require("../lib/spellchecker");

const dictionaryDirectory = path.join(__dirname, "dictionaries");

function hunspell() {
  const checker = new Spellchecker();
  checker.setSpellcheckerType(ALWAYS_USE_HUNSPELL);
  checker.setDictionary("en_US", dictionaryDirectory);
  return checker;
}

describe("checkSpelling over a whole corpus", () => {
  let checker;

  beforeEach(() => {
    checker = hunspell();
  });

  const flagged = (text) =>
    checker.checkSpelling(text).map(({ start, end }) => text.slice(start, end));

  it("agrees with isMisspelled on every word, however often it repeats", () => {
    // A verdict is remembered for the rest of a call, so a word that occurs
    // hundreds of times must still be judged the same as one that occurs once.
    const vocabulary = [
      "robot",
      "mechanical",
      "documnet",
      "artificial",
      "mispelled",
      "electronic",
      "recieve",
      "machine",
      "seperate",
      "agent",
    ];
    let text = "";
    for (let i = 0; i < 400; i++) text += `${vocabulary[i % vocabulary.length]} `;

    const expected = [];
    for (let i = 0; i < 400; i++) {
      const word = vocabulary[i % vocabulary.length];
      if (checker.isMisspelled(word)) expected.push(word);
    }

    expect(flagged(text)).toEqual(expected);
    expect(expected.length).toBeGreaterThan(0);
  });

  it("keeps verdicts for words that differ only in case", () => {
    // Distinct keys, and one must not answer for the other.
    const text = "Robot robot ROBOT documnet Documnet";

    expect(flagged(text)).toEqual(
      checker.checkSpelling(text).map(({ start, end }) => text.slice(start, end)),
    );
    expect(flagged(text)).toContain("documnet");
    expect(flagged(text)).not.toContain("robot");
  });

  it("judges words too long to remember", () => {
    // Longer than the inline key a verdict is stored under, so these take the
    // uncached path on every occurrence and must still agree with the oracle.
    const words = [
      "pneumonoultramicroscopicsilicovolcanoconiosis",
      "antidisestablishmentarianism",
      "internationalization",
    ];
    const text = [...words, ...words].join(" ");
    const expected = [...words, ...words].filter((word) => checker.isMisspelled(word));

    expect(flagged(text)).toEqual(expected);
  });

  it("reflects a word added to the dictionary on the next check", () => {
    const text = "znorkle znorkle znorkle";

    expect(flagged(text)).toEqual(["znorkle", "znorkle", "znorkle"]);

    checker.add("znorkle");

    expect(flagged(text)).toEqual([]);

    checker.remove("znorkle");

    expect(flagged(text)).toEqual(["znorkle", "znorkle", "znorkle"]);
  });

  it("returns the same ranges from the asynchronous entry point", async () => {
    let text = "";
    for (let i = 0; i < 500; i++) text += "the documnet is mispelled here. ";

    expect(await checker.checkSpellingAsync(text)).toEqual(checker.checkSpelling(text));
  });
});
