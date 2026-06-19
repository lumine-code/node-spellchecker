#ifndef WORKER_H
#define WORKER_H

#include "napi.h"
#include "spellchecker.h"

#include <vector>

class CheckSpellingWorker : public Napi::AsyncWorker {
public:
  CheckSpellingWorker(std::vector<uint16_t>&& corpus, spellchecker::SpellcheckerImplementation* impl, Napi::Function& callback);
  ~CheckSpellingWorker();

  void Execute();
  void OnOK();
private:
  const std::vector<uint16_t> corpus;
  spellchecker::SpellcheckerImplementation* impl;
  std::vector<spellchecker::MisspelledRange> misspelled_ranges;
};

#endif
