#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <napi.h>

#include "spellchecker.h"
#include "worker.h"

using spellchecker::ALWAYS_USE_HUNSPELL;
using spellchecker::ALWAYS_USE_SYSTEM;
using spellchecker::MisspelledRange;
using spellchecker::SpellcheckerFactory;
using spellchecker::SpellcheckerImplementation;
using spellchecker::USE_SYSTEM_DEFAULTS;

namespace {

Napi::Array RangesToArray(Napi::Env env, const std::vector<MisspelledRange>& ranges) {
  Napi::Array result = Napi::Array::New(env, ranges.size());
  for (size_t index = 0; index < ranges.size(); ++index) {
    Napi::Object range = Napi::Object::New(env);
    range.Set("start", Napi::Number::New(env, ranges[index].start));
    range.Set("end", Napi::Number::New(env, ranges[index].end));
    result.Set(index, range);
  }
  return result;
}

std::vector<uint16_t> ToUtf16Buffer(const Napi::Value& value) {
  const std::u16string text = value.As<Napi::String>().Utf16Value();
  std::vector<uint16_t> buffer;
  buffer.reserve(text.size() + 1);
  for (const char16_t character : text) {
    buffer.push_back(static_cast<uint16_t>(character));
  }
  buffer.push_back(0);
  return buffer;
}

class Spellchecker : public Napi::ObjectWrap<Spellchecker> {
 public:
  static Napi::Function Define(Napi::Env env) {
    return DefineClass(
        env,
        "Spellchecker",
        {
            InstanceMethod("setSpellcheckerType", &Spellchecker::SetSpellcheckerType),
            InstanceMethod("setDictionary", &Spellchecker::SetDictionary),
            InstanceMethod("getAvailableDictionaries", &Spellchecker::GetAvailableDictionaries),
            InstanceMethod("getCorrectionsForMisspelling", &Spellchecker::GetCorrectionsForMisspelling),
            InstanceMethod("isMisspelled", &Spellchecker::IsMisspelled),
            InstanceMethod("checkSpelling", &Spellchecker::CheckSpelling),
            InstanceMethod("checkSpellingAsyncCallback", &Spellchecker::CheckSpellingAsync),
            InstanceMethod("add", &Spellchecker::Add),
            InstanceMethod("remove", &Spellchecker::Remove),
        });
  }

  explicit Spellchecker(const Napi::CallbackInfo& info)
      : Napi::ObjectWrap<Spellchecker>(info), implementation_(nullptr) {}

  ~Spellchecker() override = default;

 private:
  std::unique_ptr<SpellcheckerImplementation> implementation_;

  SpellcheckerImplementation* EnsureImplementation() {
    if (!implementation_) {
      implementation_.reset(SpellcheckerFactory::CreateSpellchecker(USE_SYSTEM_DEFAULTS));
    }
    return implementation_.get();
  }

  Napi::Value SetSpellcheckerType(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsNumber()) {
      Napi::TypeError::New(env, "Bad argument: missing mode").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    if (implementation_) {
      Napi::Error::New(
          env,
          "Cannot call setSpellcheckerType after the dictionary has been configured or used")
          .ThrowAsJavaScriptException();
      return env.Undefined();
    }

    int type = USE_SYSTEM_DEFAULTS;
    switch (info[0].As<Napi::Number>().Int32Value()) {
      case USE_SYSTEM_DEFAULTS:
        break;
      case ALWAYS_USE_SYSTEM:
        type = ALWAYS_USE_SYSTEM;
        break;
      case ALWAYS_USE_HUNSPELL:
        type = ALWAYS_USE_HUNSPELL;
        break;
      default:
        Napi::RangeError::New(env, "Spellchecker type must be 0, 1, or 2")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    implementation_.reset(SpellcheckerFactory::CreateSpellchecker(type));
    return env.Undefined();
  }

  Napi::Value SetDictionary(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsString()) {
      Napi::TypeError::New(env, "Bad arguments").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    const std::string language = info[0].As<Napi::String>().Utf8Value();
    const std::string directory = info[1].As<Napi::String>().Utf8Value();
    return Napi::Boolean::New(env, EnsureImplementation()->SetDictionary(language, directory));
  }

  Napi::Value IsMisspelled(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::TypeError::New(env, "Bad argument").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    return Napi::Boolean::New(
        env, EnsureImplementation()->IsMisspelled(info[0].As<Napi::String>().Utf8Value()));
  }

  Napi::Value CheckSpelling(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::TypeError::New(env, "Bad argument").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    if (info[0].As<Napi::String>().Utf16Value().empty()) {
      return Napi::Array::New(env);
    }
    const std::vector<uint16_t> text = ToUtf16Buffer(info[0]);
    return RangesToArray(env, EnsureImplementation()->CheckSpelling(text.data(), text.size()));
  }

  Napi::Value CheckSpellingAsync(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2 || !info[0].IsString() || !info[1].IsFunction()) {
      Napi::TypeError::New(env, "Bad arguments").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    auto* worker = new CheckSpellingWorker(
        ToUtf16Buffer(info[0]),
        EnsureImplementation(),
        info[1].As<Napi::Function>(),
        info.This().As<Napi::Object>());
    worker->Queue();
    return env.Undefined();
  }

  Napi::Value Add(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::TypeError::New(env, "Bad argument").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    EnsureImplementation()->Add(info[0].As<Napi::String>().Utf8Value());
    return env.Undefined();
  }

  Napi::Value Remove(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::TypeError::New(env, "Bad argument").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    EnsureImplementation()->Remove(info[0].As<Napi::String>().Utf8Value());
    return env.Undefined();
  }

  Napi::Value GetAvailableDictionaries(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string dictionary_path = ".";
    if (info.Length() > 0) {
      if (!info[0].IsString()) {
        Napi::TypeError::New(env, "Bad argument").ThrowAsJavaScriptException();
        return env.Undefined();
      }
      dictionary_path = info[0].As<Napi::String>().Utf8Value();
    }

    const std::vector<std::string> dictionaries =
        EnsureImplementation()->GetAvailableDictionaries(dictionary_path);
    Napi::Array result = Napi::Array::New(env, dictionaries.size());
    for (size_t index = 0; index < dictionaries.size(); ++index) {
      result.Set(index, Napi::String::New(env, dictionaries[index]));
    }
    return result;
  }

  Napi::Value GetCorrectionsForMisspelling(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
      Napi::TypeError::New(env, "Bad argument").ThrowAsJavaScriptException();
      return env.Undefined();
    }
    const std::vector<std::string> corrections = EnsureImplementation()->GetCorrectionsForMisspelling(
        info[0].As<Napi::String>().Utf8Value());
    Napi::Array result = Napi::Array::New(env, corrections.size());
    for (size_t index = 0; index < corrections.size(); ++index) {
      result.Set(index, Napi::String::New(env, corrections[index]));
    }
    return result;
  }
};

Napi::Object Initialize(Napi::Env env, Napi::Object exports) {
  exports.Set("Spellchecker", Spellchecker::Define(env));
  return exports;
}

}  // namespace

NODE_API_MODULE(spellchecker, Initialize)
