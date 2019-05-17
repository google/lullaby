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

#include "lullaby/util/arg_parser.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lullaby/util/logging.h"

namespace lull {
namespace {

using ::testing::Eq;

TEST(ArgParserTest, Flag) {
  ArgParser parser;
  parser.AddArg("test");

  const char* args[] = {"test_program", "--test"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);

  EXPECT_TRUE(result);
  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(1)));
}

TEST(ArgParserTest, Arg) {
  ArgParser parser;
  parser.AddArg("test").SetNumArgs(1);

  const char* args[] = {"test_program", "--test", "foo"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);

  EXPECT_TRUE(result);
  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(1)));
  EXPECT_THAT(parser.GetString("test"), Eq(string_view("foo")));
}

TEST(ArgParserTest, ShortName) {
  ArgParser parser;
  parser.AddArg("test").SetShortName('t').SetNumArgs(1);

  const char* args[] = {"test_program", "-t", "foo"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);

  EXPECT_TRUE(result);
  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(1)));
  EXPECT_THAT(parser.GetString("test"), Eq(string_view("foo")));
}

TEST(ArgParserTest, MultiShortName) {
  ArgParser parser;
  parser.AddArg("test").SetShortName('t');
  parser.AddArg("foo").SetShortName('f');
  parser.AddArg("bar").SetShortName('b');
  parser.AddArg("moo").SetShortName('m');

  const char* args[] = {"test_program", "-tfm"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);

  EXPECT_TRUE(result);
  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_TRUE(parser.IsSet("foo"));
  EXPECT_FALSE(parser.IsSet("bar"));
  EXPECT_TRUE(parser.IsSet("moo"));

  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(1)));
  EXPECT_THAT(parser.GetNumValues("foo"), Eq(size_t(1)));
  EXPECT_THAT(parser.GetNumValues("bar"), Eq(size_t(0)));
  EXPECT_THAT(parser.GetNumValues("moo"), Eq(size_t(1)));
}

TEST(ArgParserTest, Positional) {
  ArgParser parser;

  const char* args[] = {"test_program", "foo", "bar", "baz"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);
  EXPECT_TRUE(result);

  const auto& position_args = parser.GetPositionalArgs();
  EXPECT_THAT(position_args.size(), Eq(size_t(3)));
  EXPECT_THAT(position_args[0], Eq(args[1]));
  EXPECT_THAT(position_args[1], Eq(args[2]));
  EXPECT_THAT(position_args[2], Eq(args[3]));
}

TEST(ArgParserTest, MultipleArgs) {
  ArgParser parser;
  parser.AddArg("test").SetNumArgs(1);
  parser.AddArg("bar");

  const char* args[] = {"test_program", "--test", "foo", "--bar", "baz"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);
  EXPECT_TRUE(result);

  EXPECT_TRUE(parser.IsSet("bar"));
  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_THAT(parser.GetNumValues("bar"), Eq(size_t(1)));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(1)));
  EXPECT_THAT(parser.GetString("test"), Eq(string_view("foo")));

  const auto& position_args = parser.GetPositionalArgs();
  EXPECT_THAT(position_args.size(), Eq(size_t(1)));
  EXPECT_THAT(position_args[0], Eq(args[4]));
}

TEST(ArgParserTest, MultipleValues) {
  ArgParser parser;
  parser.AddArg("test").SetNumArgs(1);

  const char* args[] = {"test_program", "--test", "foo", "--test", "meh"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);
  EXPECT_TRUE(result);

  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(2)));
  EXPECT_THAT(parser.GetString("test", 0), Eq(string_view("foo")));
  EXPECT_THAT(parser.GetString("test", 1), Eq(string_view("meh")));
}

TEST(ArgParserTest, MultipleValuesSingleFlag) {
  ArgParser parser;
  parser.AddArg("test").SetNumArgs(2);

  const char* args[] = {"test_program", "--test", "foo", "meh"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);
  EXPECT_TRUE(result);

  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(2)));
  EXPECT_THAT(parser.GetString("test", 0), Eq(string_view("foo")));
  EXPECT_THAT(parser.GetString("test", 1), Eq(string_view("meh")));
}

TEST(ArgParserTest, GetVariableValues) {
  ArgParser parser;
  parser.AddArg("test").SetVariableNumArgs();
  parser.AddArg("moo").SetVariableNumArgs();

  const char* args[] = {"test_program", "--test", "foo", "bar",
                        "baz",          "--moo", "woo", "meh"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);
  EXPECT_TRUE(result);

  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_TRUE(parser.IsSet("moo"));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(3)));
  EXPECT_THAT(parser.GetNumValues("moo"), Eq(size_t(2)));
  EXPECT_THAT(parser.GetString("test", 0), Eq(string_view("foo")));
  EXPECT_THAT(parser.GetString("test", 1), Eq(string_view("bar")));
  EXPECT_THAT(parser.GetString("test", 2), Eq(string_view("baz")));
  EXPECT_THAT(parser.GetString("moo", 0), Eq(string_view("woo")));
  EXPECT_THAT(parser.GetString("moo", 1), Eq(string_view("meh")));
}

TEST(ArgParserTest, GetValues) {
  ArgParser parser;
  parser.AddArg("test").SetNumArgs(1);
  parser.AddArg("moo").SetVariableNumArgs();

  const char* args[] = {"test_program", "--test", "foo", "--test", "meh",
                        "--moo",        "woo",    "dog", "cat"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);
  EXPECT_TRUE(result);

  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_TRUE(parser.IsSet("moo"));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(2)));
  EXPECT_THAT(parser.GetNumValues("moo"), Eq(size_t(3)));

  Span<string_view> test_span = parser.GetValues("test");
  EXPECT_THAT(test_span.size(), Eq(size_t(2)));
  EXPECT_THAT(test_span[0], Eq(string_view("foo")));
  EXPECT_THAT(test_span[1], Eq(string_view("meh")));

  Span<string_view> moo_span = parser.GetValues("moo");
  EXPECT_THAT(moo_span.size(), Eq(size_t(3)));
  EXPECT_THAT(moo_span[0], Eq(string_view("woo")));
  EXPECT_THAT(moo_span[1], Eq(string_view("dog")));
  EXPECT_THAT(moo_span[2], Eq(string_view("cat")));
}

TEST(ArgParserTest, Required) {
  ArgParser parser;
  parser.AddArg("test").SetRequired();

  const char* args[] = {"test_program", "foo", "bar", "baz"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);

  EXPECT_FALSE(result);
}

TEST(ArgParserTest, Default) {
  ArgParser parser;
  parser.AddArg("test").SetNumArgs(1).SetDefault("foo");

  const char* args[] = {"test_program", "--test"};
  const int num_args = sizeof(args) / sizeof(args[0]);
  const bool result = parser.Parse(num_args, args);

  const auto& errors = parser.GetErrors();
  for (auto& e : errors) {
    LOG(ERROR) << e;
  }

  EXPECT_TRUE(result);
  EXPECT_TRUE(parser.IsSet("test"));
  EXPECT_THAT(parser.GetNumValues("test"), Eq(size_t(1)));
  EXPECT_THAT(parser.GetString("test"), Eq(string_view("foo")));}

}  // namespace
}  // namespace lull
