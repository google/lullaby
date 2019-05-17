/*
Copyright 2017-2019 Google Inc. All Rights Reserved.

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

#include "lullaby/contrib/text_input/edit_text.h"
#include "gtest/gtest.h"

namespace lull {
namespace {

TEST(SetTextTest, SetTextTest) {
  EditText edit_text;

  EXPECT_TRUE(edit_text.empty());

  edit_text.SetText("0123456789");
  EXPECT_EQ(edit_text.str(), "0123456789");
}

TEST(ClearTextTest, ClearTextTest) {
  EditText edit_text;
  edit_text.SetText("abc");
  edit_text.SetComposingRegion(0UL, 3UL);
  edit_text.SetSelectionRegion(1UL, 2UL);
  edit_text.Clear();
  size_t start, end;
  edit_text.GetComposingRegion(&start, &end);
  EXPECT_EQ(0UL, start);
  EXPECT_EQ(0UL, end);
  EXPECT_FALSE(edit_text.HasComposingRegion());
  edit_text.GetSelectionRegion(&start, &end);
  EXPECT_EQ(0UL, start);
  EXPECT_EQ(0UL, end);
  EXPECT_FALSE(edit_text.HasSelectionRegion());
}

TEST(ComposingRegionTest, ComposingRegionTest) {
  EditText edit_text;
  edit_text.SetText("0123456789");
  edit_text.SetComposingRegion(1, 3);
  size_t start, end;
  edit_text.GetComposingRegion(&start, &end);
  EXPECT_EQ(1UL, start);
  EXPECT_EQ(3UL, end);
}

TEST(SetComposingTextTest, SetComposingTextTest) {
  EditText edit_text;
  edit_text.SetText("0123456789");
  size_t composition_start, composition_end, selection_start, selection_end;

  // With previous composition and caret selection.
  edit_text.SetComposingRegion(1, 3);
  edit_text.SetComposingText("abc");
  edit_text.GetComposingRegion(&composition_start, &composition_end);
  edit_text.GetSelectionRegion(&selection_start, &selection_end);
  EXPECT_EQ(1UL, composition_start);
  EXPECT_EQ(4UL, composition_end);
  EXPECT_EQ(4UL, selection_start);
  EXPECT_EQ(4UL, selection_end);

  // With previous composition and range selection.
  edit_text.SetSelectionRegion(4, 6);
  edit_text.SetComposingText("12");
  edit_text.GetComposingRegion(&composition_start, &composition_end);
  edit_text.GetSelectionRegion(&selection_start, &selection_end);
  EXPECT_EQ(1UL, composition_start);
  EXPECT_EQ(3UL, composition_end);
  EXPECT_EQ(3UL, selection_start);
  EXPECT_EQ(3UL, selection_end);

  // Without previous composition and with caret selection.
  edit_text.Commit("12");
  edit_text.SetComposingText("abc");
  edit_text.GetComposingRegion(&composition_start, &composition_end);
  edit_text.GetSelectionRegion(&selection_start, &selection_end);
  EXPECT_EQ(3UL, composition_start);
  EXPECT_EQ(6UL, composition_end);
  EXPECT_EQ(6UL, selection_start);
  EXPECT_EQ(6UL, selection_end);

  // Without previous composition and with range selection.
  edit_text.Commit("abc");
  edit_text.SetSelectionRegion(3, 6);
  edit_text.SetComposingText("de");
  edit_text.GetComposingRegion(&composition_start, &composition_end);
  edit_text.GetSelectionRegion(&selection_start, &selection_end);
  EXPECT_EQ(3UL, composition_start);
  EXPECT_EQ(5UL, composition_end);
  EXPECT_EQ(5UL, selection_start);
  EXPECT_EQ(5UL, selection_end);
}

TEST(CommitTest, CommitTest) {
  EditText edit_text;
  edit_text.SetText("0123456789");
  edit_text.SetComposingRegion(1, 3);
  bool committed = edit_text.Commit("hello");
  EXPECT_TRUE(committed);
  EXPECT_EQ(edit_text.str(), "0hello3456789");
  size_t start, end;
  edit_text.GetComposingRegion(&start, &end);
  EXPECT_EQ(start, end);
}

TEST(BackspaceTest, BackspaceTest) {
  EditText edit_text;
  edit_text.SetText("0123456789");
  edit_text.SetComposingRegion(1, 3);
  edit_text.Commit("hello");
  edit_text.SetComposingRegion(1, 6);
  edit_text.SetCaretPosition(3);
  edit_text.Backspace();
  EXPECT_EQ(edit_text.str(), "0hllo3456789");
  size_t start, end;
  edit_text.GetComposingRegion(&start, &end);
  EXPECT_EQ(1UL, start);
  EXPECT_EQ(5UL, end);
}

TEST(InsertTestWithSelection, InsertTestWithSelection) {
  EditText edit_text;
  edit_text.SetText("0123456789");
  edit_text.SetSelectionRegion(1, 4);
  edit_text.Insert("abcd");
  EXPECT_EQ(edit_text.str(), "0abcd456789");
  size_t start, end;
  edit_text.GetSelectionRegion(&start, &end);
  EXPECT_EQ(start, end);
  EXPECT_FALSE(edit_text.HasSelectionRegion());
  EXPECT_EQ(start, 5UL);
}

TEST(InsertTestWithoutSelection, InsertTestWithoutSelection) {
  EditText edit_text;
  edit_text.SetText("0123456789");
  edit_text.SetCaretPosition(7);
  edit_text.Insert("abcd");
  EXPECT_EQ(edit_text.str(), "0123456abcd789");
  size_t start, end;
  edit_text.GetSelectionRegion(&start, &end);
  EXPECT_EQ(start, end);
  size_t caret_pos = edit_text.GetCaretPosition();
  EXPECT_FALSE(edit_text.HasSelectionRegion());
  EXPECT_EQ(caret_pos, 11UL);
}

TEST(InsertAffectsComposingRegion, InsertAffectsComposingRegion) {
  size_t start, end;
  EditText edit_text;
  edit_text.SetText("0123456789");
  edit_text.SetSelectionRegion(2, 4);
  edit_text.SetComposingRegion(1, 7);
  edit_text.Insert("abcd");
  edit_text.GetComposingRegion(&start, &end);
  EXPECT_EQ(1UL, start);
  EXPECT_EQ(9UL, end);

  edit_text.SetText("0123456789");
  edit_text.SetSelectionRegion(3, 4);
  edit_text.SetComposingRegion(1, 2);
  edit_text.Insert("abcd");
  edit_text.GetComposingRegion(&start, &end);
  EXPECT_EQ(1UL, start);
  EXPECT_EQ(2UL, end);

  edit_text.SetText("0123456789");
  edit_text.SetSelectionRegion(3, 4);
  edit_text.SetComposingRegion(6, 7);
  edit_text.Insert("abcd");
  edit_text.GetComposingRegion(&start, &end);
  EXPECT_EQ(9UL, start);
  EXPECT_EQ(10UL, end);
}

TEST(CommitAffectsSelectionRegion, CommitAffectsSelectionRegion) {
  size_t start, end;
  EditText edit_text;
  edit_text.SetText("0123456789");
  edit_text.SetSelectionRegion(1, 7);
  edit_text.SetComposingRegion(2, 4);
  edit_text.Commit("abcd");
  edit_text.GetSelectionRegion(&start, &end);
  EXPECT_EQ(1UL, start);
  EXPECT_EQ(9UL, end);

  edit_text.SetText("0123456789");
  edit_text.SetSelectionRegion(1, 2);
  edit_text.SetComposingRegion(3, 4);
  edit_text.Commit("abcd");
  edit_text.GetSelectionRegion(&start, &end);
  EXPECT_EQ(1UL, start);
  EXPECT_EQ(2UL, end);

  edit_text.SetText("0123456789");
  edit_text.SetSelectionRegion(6, 7);
  edit_text.SetComposingRegion(3, 4);
  edit_text.Commit("abcd");
  edit_text.GetSelectionRegion(&start, &end);
  EXPECT_EQ(9UL, start);
  EXPECT_EQ(10UL, end);
}

TEST(CommitOrInsertTest, CommitOrInsertTest) {
  EditText edit_text;
  edit_text.SetText("abcdefg");
  edit_text.SetSelectionRegion(5, 5);
  edit_text.SetComposingRegion(3, 5);
  edit_text.CommitOrInsert("01");
  EXPECT_EQ(edit_text.str(), "abc01fg");
  size_t start, end;
  edit_text.GetSelectionRegion(&start, &end);
  EXPECT_EQ(5UL, start);
  EXPECT_EQ(5UL, end);
  edit_text.GetComposingRegion(&start, &end);
  EXPECT_EQ(start, end);
}

}  // namespace
}  // namespace lull
