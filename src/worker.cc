#include "worker.h"

#include <memory>
#include <utility>

CheckSpellingWorker::CheckSpellingWorker(
    std::vector<uint16_t>&& corpus,
    spellchecker::SpellcheckerImplementation* implementation,
    const Napi::Function& callback,
    const Napi::Object& owner)
    : Napi::AsyncWorker(callback),
      corpus_(std::move(corpus)),
      implementation_(implementation),
      owner_(Napi::Persistent(owner)) {}

void CheckSpellingWorker::Execute() {
  std::unique_ptr<spellchecker::SpellcheckerThreadView> view =
      implementation_->CreateThreadView();
  misspelled_ranges_ = view->CheckSpelling(corpus_.data(), corpus_.size());
}

void CheckSpellingWorker::OnOK() {
  Napi::HandleScope scope(Env());
  Napi::Array result = Napi::Array::New(Env(), misspelled_ranges_.size());
  for (size_t index = 0; index < misspelled_ranges_.size(); ++index) {
    Napi::Object range = Napi::Object::New(Env());
    range.Set("start", Napi::Number::New(Env(), misspelled_ranges_[index].start));
    range.Set("end", Napi::Number::New(Env(), misspelled_ranges_[index].end));
    result.Set(index, range);
  }
  Callback().Call({Env().Null(), result});
  owner_.Reset();
}
