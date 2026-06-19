#include "worker.h"

#include "napi.h"
#include "spellchecker.h"

#include <string>
#include <vector>
#include <utility>

using namespace spellchecker;

CheckSpellingWorker::CheckSpellingWorker(
  std::vector<uint16_t>&& corpus,
  SpellcheckerImplementation* impl,
  Napi::Function& callback
) : AsyncWorker(callback), corpus(std::move(corpus)), impl(impl)
{
  // No-op
}

CheckSpellingWorker::~CheckSpellingWorker()
{
  // No-op
}

void CheckSpellingWorker::Execute() {
  std::unique_ptr<SpellcheckerThreadView> view = impl->CreateThreadView();
  misspelled_ranges = view->CheckSpelling(corpus.data(), corpus.size());
}

void CheckSpellingWorker::OnOK() {
  Napi::Env env = Env();
  Napi::HandleScope scope(env);

  Napi::Array result = Napi::Array::New(env);
  for (size_t index = 0; index < misspelled_ranges.size(); ++index) {
    const MisspelledRange& range = misspelled_ranges[index];

    Napi::Object misspelled_range = Napi::Object::New(env);
    misspelled_range.Set("start", Napi::Number::New(env, range.start));
    misspelled_range.Set("end", Napi::Number::New(env, range.end));
    result.Set(index, misspelled_range);
  }

  Callback().Call({ env.Null(), result });
}
