#include <vector>
#include "napi.h"
#include "main.h"
#include "spellchecker.h"
#include "worker.h"

using namespace spellchecker;

Spellchecker::Spellchecker(const Napi::CallbackInfo& info)
  : Napi::ObjectWrap<Spellchecker>(info), impl(NULL) {
}

Spellchecker::~Spellchecker() {
  delete impl;
}

void Spellchecker::EnsureLoadedImplementation() {
  if (!impl) {
    impl = SpellcheckerFactory::CreateSpellchecker(USE_SYSTEM_DEFAULTS);
  }
}

Napi::Value Spellchecker::SetSpellcheckerType(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::Error::New(env, "Bad argument: missing mode").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // If we already have an implementation, then we want to complain because
  // we can't handle reinitializing the dictionary paths.
  if (impl) {
    Napi::Error::New(env, "Cannot call SetSpellcheckerType after the dictionary has been configured or used").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  // Make sure we have a sane value for our enumeration.
  int modeNumber = info[0].As<Napi::Number>().Int32Value();
  int spellcheckerType = USE_SYSTEM_DEFAULTS;

  switch (modeNumber) {
    case 0:
      break;
    case 1:
      spellcheckerType = ALWAYS_USE_SYSTEM;
      break;
    case 2:
      spellcheckerType = ALWAYS_USE_HUNSPELL;
      break;
    default:
      Napi::Error::New(env, "Bad argument: SetSpellcheckerType must be given 0, 1, or 2 as a parameter").ThrowAsJavaScriptException();
      return env.Undefined();
  }

  // Create a new one with the appropriate checker type.
  impl = SpellcheckerFactory::CreateSpellchecker(spellcheckerType);
  return env.Undefined();
}

Napi::Value Spellchecker::SetDictionary(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::Error::New(env, "Bad arguments").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string language = info[0].As<Napi::String>().Utf8Value();
  std::string directory = ".";
  if (info.Length() > 1) {
    directory = info[1].As<Napi::String>().Utf8Value();
  }

  EnsureLoadedImplementation();

  bool result = impl->SetDictionary(language, directory);
  return Napi::Boolean::New(env, result);
}

Napi::Value Spellchecker::IsMisspelled(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::Error::New(env, "Bad argument").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string word = info[0].As<Napi::String>().Utf8Value();

  EnsureLoadedImplementation();

  return Napi::Boolean::New(env, impl->IsMisspelled(word));
}

Napi::Value Spellchecker::CheckSpelling(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsString()) {
    Napi::Error::New(env, "Bad argument").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::Array result = Napi::Array::New(env);

  std::u16string text = info[0].As<Napi::String>().Utf16Value();
  if (text.empty()) {
    return result;
  }

  EnsureLoadedImplementation();

  // Include the implicit trailing null terminator that std::u16string
  // guarantees at data()[size()]; the state machine in CheckSpelling uses
  // it as a sentinel to flush the final word in the string.
  std::vector<MisspelledRange> misspelled_ranges =
    impl->CheckSpelling(reinterpret_cast<const uint16_t*>(text.data()), text.size() + 1);

  for (size_t index = 0; index < misspelled_ranges.size(); ++index) {
    const MisspelledRange& range = misspelled_ranges[index];

    Napi::Object misspelled_range = Napi::Object::New(env);
    misspelled_range.Set("start", Napi::Number::New(env, range.start));
    misspelled_range.Set("end", Napi::Number::New(env, range.end));
    result.Set(index, misspelled_range);
  }

  return result;
}

void Spellchecker::CheckSpellingAsync(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 2 || !info[0].IsString()) {
    Napi::Error::New(env, "Bad argument").ThrowAsJavaScriptException();
    return;
  }

  std::u16string corpus = info[0].As<Napi::String>().Utf16Value();
  Napi::Function callback = info[1].As<Napi::Function>();

  EnsureLoadedImplementation();

  // Include the trailing null terminator (see CheckSpelling above) so the
  // worker's final word gets flushed too.
  CheckSpellingWorker* worker = new CheckSpellingWorker(
    std::vector<uint16_t>(corpus.data(), corpus.data() + corpus.size() + 1), impl, callback);
  worker->Queue();
}

void Spellchecker::Add(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::Error::New(env, "Bad argument").ThrowAsJavaScriptException();
    return;
  }

  EnsureLoadedImplementation();

  std::string word = info[0].As<Napi::String>().Utf8Value();
  impl->Add(word);
}

void Spellchecker::Remove(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::Error::New(env, "Bad argument").ThrowAsJavaScriptException();
    return;
  }

  EnsureLoadedImplementation();

  std::string word = info[0].As<Napi::String>().Utf8Value();
  impl->Remove(word);
}

Napi::Value Spellchecker::GetAvailableDictionaries(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  EnsureLoadedImplementation();

  std::string path = ".";
  if (info.Length() > 0) {
    path = info[0].As<Napi::String>().Utf8Value();
  }

  std::vector<std::string> dictionaries = impl->GetAvailableDictionaries(path);

  Napi::Array result = Napi::Array::New(env, dictionaries.size());
  for (size_t i = 0; i < dictionaries.size(); ++i) {
    result.Set(i, Napi::String::New(env, dictionaries[i]));
  }

  return result;
}

Napi::Value Spellchecker::GetCorrectionsForMisspelling(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::Error::New(env, "Bad argument").ThrowAsJavaScriptException();
    return env.Undefined();
  }

  EnsureLoadedImplementation();

  std::string word = info[0].As<Napi::String>().Utf8Value();
  std::vector<std::string> corrections = impl->GetCorrectionsForMisspelling(word);

  Napi::Array result = Napi::Array::New(env, corrections.size());
  for (size_t i = 0; i < corrections.size(); ++i) {
    result.Set(i, Napi::String::New(env, corrections[i]));
  }

  return result;
}

Napi::Object Spellchecker::Init(Napi::Env env, Napi::Object exports) {
  Napi::Function func = DefineClass(env, "Spellchecker", {
    InstanceMethod("setSpellcheckerType", &Spellchecker::SetSpellcheckerType),
    InstanceMethod("setDictionary", &Spellchecker::SetDictionary),
    InstanceMethod("getAvailableDictionaries", &Spellchecker::GetAvailableDictionaries),
    InstanceMethod("getCorrectionsForMisspelling", &Spellchecker::GetCorrectionsForMisspelling),
    InstanceMethod("isMisspelled", &Spellchecker::IsMisspelled),
    InstanceMethod("checkSpelling", &Spellchecker::CheckSpelling),
    InstanceMethod("checkSpellingAsync", &Spellchecker::CheckSpellingAsync,
      static_cast<napi_property_attributes>(napi_writable | napi_configurable)),
    InstanceMethod("add", &Spellchecker::Add),
    InstanceMethod("remove", &Spellchecker::Remove),
  });

  exports.Set("Spellchecker", func);
  return exports;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  return Spellchecker::Init(env, exports);
}

NODE_API_MODULE(spellchecker, Init)
