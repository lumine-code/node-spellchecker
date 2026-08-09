#ifndef SRC_WORKER_H_
#define SRC_WORKER_H_

#include <memory>
#include <vector>

#include <napi.h>

#include "spellchecker.h"

class CheckSpellingWorker : public Napi::AsyncWorker {
 public:
  CheckSpellingWorker(
      std::vector<uint16_t>&& corpus,
      spellchecker::SpellcheckerImplementation* implementation,
      const Napi::Function& callback,
      const Napi::Object& owner);
  ~CheckSpellingWorker() override = default;

  void Execute() override;
  void OnOK() override;

 private:
  const std::vector<uint16_t> corpus_;
  spellchecker::SpellcheckerImplementation* implementation_;
  std::vector<spellchecker::MisspelledRange> misspelled_ranges_;
  Napi::ObjectReference owner_;
};

#endif  // SRC_WORKER_H_
