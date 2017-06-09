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

#ifndef LULLABY_SYSTEMS_TEXT_INPUT_EDIT_TEXT_H_
#define LULLABY_SYSTEMS_TEXT_INPUT_EDIT_TEXT_H_

#include "lullaby/util/utf8_string.h"

namespace lull {

// Class that represents the edit text in the input field.
class EditText {
 public:
  // Sets text. Selection and composing region will be intact so long as they
  // are in bound. Out of bound indices will be updated to the nearest
  // valid values.
  void SetText(const char* utf8_cstr);
  void SetText(const std::string& utf8_str);

  // Gets utf8 encoded string at a given index.
  std::string CharAt(size_t index) const;

  // Clears text.
  void Clear();

  // Sets caret position. This is equivalent to set selection region with
  // same start and end index. Nearest valid position will be used if given
  // index is out of bound.
  void SetCaretPosition(size_t pos);

  // Gets caret position. If there is selection region, selection end index
  // will be returned.
  size_t GetCaretPosition() const;

  // Sets selection region. Out of bound positions will be clamped. Same
  // start and end value means nothing selected and caret is at a single
  // position.
  // @start index of the first UTF8 character in the selection.
  // @end One more than the index of the last UTF8 character in the selection.
  void SetSelectionRegion(size_t start, size_t end);

  // Clears selection region. Text that was previously selected remain intact.
  void ClearSelectionRegion();

  void GetSelectionRegion(size_t* start, size_t* end) const;

  // Returns true if any text is being selected.
  bool HasSelectionRegion() const;

  // Sets composing region, i.e the text that is being composed but not
  // finalized yet. Such text is usually underlined in the edit box.
  // Out of bound positions will be clamped. Same start and end value means no
  // text is being composed.
  // @start index of the first UTF8 character in the composing region.
  // @end One more than the index of the last UTF8 character in the composing
  //      region.
  void SetComposingRegion(size_t start, size_t end);

  // Cancels composing. This will set composing region to empty. Text that
  // was in composing region remains intact.
  void ClearComposingRegion();

  void GetComposingRegion(size_t* start, size_t* end) const;

  // Returns true if any text is being composed.
  bool HasComposingRegion() const;

  // When composing region is not empty, composing text will be replaced by
  // the given text. Composing region becomes empty. And this method returns
  // true. No-op if composing region is empty. And this method returns false.
  bool Commit(const char* utf8_cstr);
  bool Commit(const std::string& utf8_str);

  // Tries to commit the given text if composing region is not empty. Otherwise
  // if selection is not empty, selection will be replaced by given text. In
  // case nothing is being selected or composed, given text will be inserted at
  // where the caret is at.
  void CommitOrInsert(const char* utf8_cstr);
  void CommitOrInsert(const std::string& utf_str);

  // Updates text on backspace.
  //
  // When selection exists, entire selection will be erased.
  //
  // Otherwise one UTF8 character before caret will be erased. Composing region
  // will be updated if exists.
  //
  // Returns true if text has changed.
  bool Backspace();

  // Appends given text at where the caret is.
  //
  // When selection exists, entire selection will be replaced with given text.
  //
  // Otherwise text will be appended at where caret is.
  void Insert(const char* utf8_cstr);
  void Insert(const std::string& utf8_str);

  const std::string& str() const;
  const char* c_str() const;

  bool empty() const;
  size_t CharSize() const;

 private:
  void ClampRegions();
  void FixSelectionRegionForDeletion(size_t delete_index, size_t delete_len);
  void FixComposingRegionForDeletion(size_t delete_index, size_t delete_len);

  UTF8String text_;
  size_t selection_start_index_ = 0;
  size_t selection_end_index_ = 0;
  size_t composing_start_index_ = 0;
  size_t composing_end_index_ = 0;
};

}  // namespace lull

#endif  // LULLABY_SYSTEMS_TEXT_INPUT_EDIT_TEXT_H_
