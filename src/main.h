#ifndef SRC_MAIN_H_
#define SRC_MAIN_H_

#include "napi.h"
#include "spellchecker.h"

class Spellchecker : public Napi::ObjectWrap<Spellchecker> {
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    explicit Spellchecker(const Napi::CallbackInfo& info);
    ~Spellchecker();

    Napi::Value SetSpellcheckerType(const Napi::CallbackInfo& info);
    Napi::Value SetDictionary(const Napi::CallbackInfo& info);
    Napi::Value IsMisspelled(const Napi::CallbackInfo& info);
    Napi::Value CheckSpelling(const Napi::CallbackInfo& info);
    void CheckSpellingAsync(const Napi::CallbackInfo& info);
    void Add(const Napi::CallbackInfo& info);
    void Remove(const Napi::CallbackInfo& info);
    Napi::Value GetAvailableDictionaries(const Napi::CallbackInfo& info);
    Napi::Value GetCorrectionsForMisspelling(const Napi::CallbackInfo& info);

  private:
    spellchecker::SpellcheckerImplementation* impl;
    void EnsureLoadedImplementation();
};

#endif  // SRC_MAIN_H_
