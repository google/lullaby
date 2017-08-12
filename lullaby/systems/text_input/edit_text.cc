/*
Copyright 2017 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "lullaby/systems/text_input/edit_text.h"

#include <algorithm>

namespace {

size_t FixIndexForDeletion(
    size_t index, size_t delete_index, size_t delete_len) {
  if (index <= delete_index) {
    return index;
  } else if (index <= delete_index + delete_len) {
    return delete_index;
  } else {
    return index - delete_len;
  }
}

}  // namespace

namespace lull {

void EditText::SetText(const char* utf8_cstr) {
  text_.Set(utf8_cstr);
  ClampRegions();
}

void EditText::SetText(const std::string& utf8_str) {
  SetText(utf8_str.c_str());
}

std::string EditText::CharAt(size_t index) const {
  return text_.CharAt(index);
}

void EditText::Clear() {
  SetText("");
}

void EditText::SetCaretPosition(size_t pos) {
  SetSelectionRegion(pos, pos);
}

size_t EditText::GetCaretPosition() const {
  return selection_end_index_;
}

void EditText::SetSelectionRegion(size_t start, size_t end) {
  selection_start_index_ = start;
  selection_end_index_ = end;
  ClampRegions();
}

bool EditText::HasSelectionRegion() const {
  return selection_start_index_ < selection_end_index_;
}

void EditText::SetComposingRegion(size_t start, size_t end) {
  composing_start_index_ = start;
  composing_end_index_ = end;
  ClampRegions();
}

bool EditText::HasComposingRegion() const {
  return composing_start_index_ < composing_end_index_;
}

bool EditText::Commit(const char* utf8_cstr) {
  if (!HasComposingRegion()) {
    return false;
  }
  const size_t index = composing_start_index_;
  const size_t delete_len = composing_end_index_ - composing_start_index_;

  text_.DeleteChars(index, delete_len);
  FixSelectionRegionForDeletion(index, delete_len);
  const size_t added = text_.Insert(index, utf8_cstr);
  if (index <= selection_start_index_) {
    selection_start_index_ += added;
  }
  if (index <= selection_end_index_) {
    selection_end_index_ += added;
  }

  composing_start_index_ = 0;
  composing_end_index_ = 0;
  return true;
}

bool EditText::Commit(const std::string& utf8_str) {
  return Commit(utf8_str.c_str());
}

void EditText::CommitOrInsert(const char* utf8_cstr) {
  if (Commit(utf8_cstr)) {
    return;
  }
  Insert(utf8_cstr);
}

void EditText::CommitOrInsert(const std::string& utf8_str) {
  CommitOrInsert(utf8_str.c_str());
}

bool EditText::Backspace() {
  // Text is empty or caret is at the beginning already.
  if (selection_end_index_ == 0) {
    return false;
  }

  const bool is_selected = HasSelectionRegion();
  const size_t index =
      is_selected ? selection_start_index_ : selection_end_index_ - 1;
  const size_t delete_len =
      is_selected ? selection_end_index_ - selection_start_index_ : 1;
  text_.DeleteChars(index, delete_len);

  FixComposingRegionForDeletion(index, delete_len);

  selection_start_index_ = index;
  selection_end_index_ = selection_start_index_;

  return true;
}

void EditText::Insert(const char* utf8_cstr) {
  if (HasSelectionRegion()) {
    const size_t delete_len = selection_end_index_ - selection_start_index_;
    text_.DeleteChars(selection_start_index_, delete_len);

    FixComposingRegionForDeletion(selection_start_index_, delete_len);
  }

  const size_t added = text_.Insert(selection_start_index_, utf8_cstr);

  if (selection_start_index_ <= composing_start_index_) {
    composing_start_index_ += added;
  }
  if (selection_start_index_ <= composing_end_index_) {
    composing_end_index_ += added;
  }

  SetCaretPosition(selection_start_index_ + added);
}

void EditText::Insert(const std::string& utf8_str) {
  Insert(utf8_str.c_str());
}

void EditText::ClearComposingRegion() {
  composing_start_index_ = 0;
  composing_end_index_ = 0;
}

const std::string& EditText::str() const {
  return text_.str();
}

const char* EditText::c_str() const {
  return text_.c_str();
}

void EditText::GetSelectionRegion(size_t* start, size_t* end) const {
  *start = selection_start_index_;
  *end = selection_end_index_;
}

void EditText::GetComposingRegion(size_t* start, size_t* end) const {
  *start = composing_start_index_;
  *end = composing_end_index_;
}

void EditText::FixSelectionRegionForDeletion(
    size_t delete_index, size_t delete_len) {
  selection_start_index_ =
      FixIndexForDeletion(selection_start_index_, delete_index, delete_len);
  selection_end_index_ =
      FixIndexForDeletion(selection_end_index_, delete_index, delete_len);
}

void EditText::FixComposingRegionForDeletion(
    size_t delete_index, size_t delete_len) {
  if (HasComposingRegion()) {
    composing_start_index_ =
        FixIndexForDeletion(composing_start_index_, delete_index, delete_len);
    composing_end_index_ =
        FixIndexForDeletion(composing_end_index_, delete_index, delete_len);
  }
}

void EditText::ClampRegions() {
  const size_t char_size = text_.CharSize();
  selection_start_index_ = std::min(selection_start_index_, char_size);
  selection_end_index_ = std::min(selection_end_index_, char_size);
  if (selection_end_index_ < selection_start_index_) {
    selection_end_index_ = selection_start_index_;
  }
  composing_start_index_ = std::min(composing_start_index_, char_size);
  composing_end_index_ = std::min(composing_end_index_, char_size);
  if (composing_end_index_ < composing_start_index_) {
    composing_end_index_ = composing_start_index_;
  }
}

bool EditText::empty() const {
  return text_.empty();
}

size_t EditText::CharSize() const {
  return text_.CharSize();
}

}  // namespace lull
