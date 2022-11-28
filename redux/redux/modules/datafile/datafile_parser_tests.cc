/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "redux/modules/datafile/datafile_parser.h"
#include "redux/modules/var/var.h"

namespace redux {
namespace {

using ::testing::Eq;

enum Token {
  kKey,
  kBeginObject,
  kEndObject,
  kBeginArray,
  kEndArray,
  kNull,
  kBoolean,
  kNumber,
  kString,
  kExpression,
  kError,
};

struct TestDatafileParserCallbacks : DatafileParserCallbacks {
  void Key(std::string_view value) override {
    tokens.push_back(kKey);
    values.emplace_back(std::string(value));
  }

  void BeginObject() override {
    tokens.push_back(kBeginObject);
    values.push_back(Var());
  }

  void EndObject() override {
    tokens.push_back(kEndObject);
    values.emplace_back();
  }

  void BeginArray() override {
    tokens.push_back(kBeginArray);
    values.emplace_back();
  }

  void EndArray() override {
    tokens.push_back(kEndArray);
    values.emplace_back();
  }

  void Null() override {
    tokens.push_back(kNull);
    values.emplace_back();
  }

  void Boolean(bool value) override {
    tokens.push_back(kBoolean);
    values.emplace_back(value);
  }

  void Number(double value) override {
    tokens.push_back(kNumber);
    values.emplace_back(value);
  }

  void String(std::string_view value) override {
    tokens.push_back(kString);
    values.emplace_back(std::string(value));
  }

  void Expression(std::string_view value) override {
    tokens.push_back(kExpression);
    values.emplace_back(std::string(value));
  }

  void ParseError(std::string_view context, std::string_view message) override {
    LOG(ERROR) << message;
    tokens.push_back(kError);
    values.emplace_back(std::string(message));
  }

  std::vector<Token> tokens;
  std::vector<Var> values;
};

TEST(DatafileParserTest, Empty) {
  TestDatafileParserCallbacks callbacks;
  ParseDatafile("", &callbacks);
  EXPECT_TRUE(callbacks.tokens.empty());
}

TEST(DatafileParserTest, EmptyObject) {
  const char* txt = "{}";
  std::vector<Token> expected = {
      kBeginObject,
      kEndObject,
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, BasicParsing) {
  const char* txt =
      "{"
      "  'Null': null,"
      "  'True': true,"
      "  'False': false,"
      "  'Number': 123.456,"
      "  'Text': 'hello',"
      "  'Array': [1, 2],"
      "  'Obj': {'key':'value'},"
      "  'Expr': (+ 1 1),"
      "}";

  std::vector<Token> expected = {
      kBeginObject, kKey,    kNull,      kKey,      kBoolean,    kKey,
      kBoolean,     kKey,    kNumber,    kKey,      kString,     kKey,
      kBeginArray,  kNumber, kNumber,    kEndArray, kKey,        kBeginObject,
      kKey,         kString, kEndObject, kKey,      kExpression, kEndObject,
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
  EXPECT_THAT(callbacks.values[4].ValueOr(false), Eq(true));
  EXPECT_THAT(callbacks.values[6].ValueOr(true), Eq(false));
  EXPECT_THAT(callbacks.values[8].ValueOr(0.0), Eq(123.456));
  EXPECT_THAT(callbacks.values[10].ValueOr(std::string()), Eq("hello"));
  EXPECT_THAT(callbacks.values[13].ValueOr(0.0), Eq(1.0));
  EXPECT_THAT(callbacks.values[14].ValueOr(0.0), Eq(2.0));
  EXPECT_THAT(callbacks.values[19].ValueOr(std::string()), Eq("value"));
  EXPECT_THAT(callbacks.values[22].ValueOr(std::string()), Eq("(+ 1 1)"));

  EXPECT_THAT(callbacks.values[1].ValueOr(std::string()), Eq("Null"));
  EXPECT_THAT(callbacks.values[3].ValueOr(std::string()), Eq("True"));
  EXPECT_THAT(callbacks.values[5].ValueOr(std::string()), Eq("False"));
  EXPECT_THAT(callbacks.values[7].ValueOr(std::string()), Eq("Number"));
  EXPECT_THAT(callbacks.values[9].ValueOr(std::string()), Eq("Text"));
  EXPECT_THAT(callbacks.values[11].ValueOr(std::string()), Eq("Array"));
  EXPECT_THAT(callbacks.values[16].ValueOr(std::string()), Eq("Obj"));
  EXPECT_THAT(callbacks.values[18].ValueOr(std::string()), Eq("key"));
  EXPECT_THAT(callbacks.values[21].ValueOr(std::string()), Eq("Expr"));
}

TEST(DatafileParserTest, EscapedQuotes) {
  const char* txt = "{Key : 'hello\\'world'}";
  std::vector<Token> expected = {
    kBeginObject, kKey, kString, kEndObject
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, UnquotedKey) {
  const char* txt = "{Key : 0}";
  std::vector<Token> expected = {
    kBeginObject, kKey, kNumber, kEndObject
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, SingleUnquotedKey) {
  const char* txt = "{'Key' : 0}";
  std::vector<Token> expected = {
    kBeginObject, kKey, kNumber, kEndObject
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, DoubleUnquotedKey) {
  const char* txt = "{\"Key\" : 0}";
  std::vector<Token> expected = {
    kBeginObject, kKey, kNumber, kEndObject
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, NoSpaceBetweenKeyValue) {
  const char* txt = "{Key:0}";
  std::vector<Token> expected = {
    kBeginObject, kKey, kNumber, kEndObject
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, NoWhitespace) {
  const char* txt = "{'Array':[1,2]}";
  std::vector<Token> expected = {
    kBeginObject, kKey, kBeginArray, kNumber, kNumber, kEndArray, kEndObject
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, ObjectInArray) {
  const char* txt = "{'Array': [{'Key': 1}]}";
  std::vector<Token> expected = {
      kBeginObject, kKey,       kBeginArray, kBeginObject, kKey,
      kNumber,      kEndObject, kEndArray,   kEndObject,
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, NestedArrays) {
  const char* txt = "{'Array': [0,[1,[2],3],4]}";
  std::vector<Token> expected = {
      kBeginObject, kKey,        kBeginArray, kNumber,    kBeginArray,
      kNumber,      kBeginArray, kNumber,     kEndArray,  kNumber,
      kEndArray,    kNumber,     kEndArray,   kEndObject,
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, CommentIgnored) {
  const char* txt = "{'Array': ; ignored \n [1, 2]}";
  std::vector<Token> expected = {
    kBeginObject, kKey, kBeginArray, kNumber, kNumber, kEndArray, kEndObject
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, ExpressionContainsStringWithMarkers) {
  const char* txt = "{'Expr': (? 'hello)}]')}";
  std::vector<Token> expected = {
    kBeginObject, kKey, kExpression, kEndObject
  };

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens, Eq(expected));
}

TEST(DatafileParserTest, ExpressionContainsUnfinishedString) {
  const char* txt = "{'Expr': (? 'hello)}";

  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, ObjectNotFinished) {
  const char* txt = "{'Object': {'Number': 123.456}";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, ArrayNotFinished) {
  const char* txt = "{'Array': [1, 2, 3";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, ExpressionNotFinished) {
  const char* txt = "{'Expr': (+ 1 2}";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, ExpressionDoubleFinished) {
  const char* txt = "{'Expr': (+ 1 2))}";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, StringNotFinished) {
  const char* txt = "{'Name': 'Hello}";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, BadObjectScoping) {
  const char* txt = "{'Array': [1, 2, 3]]";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, BadArrayScoping) {
  const char* txt = "{'Array': [1, 2, 3}}";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, UnfinishedString) {
  const char* txt = "{\"Key': 1}";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}

TEST(DatafileParserTest, InvalidKey) {
  const char* txt = "{Key:: 1}";
  TestDatafileParserCallbacks callbacks;
  ParseDatafile(txt, &callbacks);
  EXPECT_THAT(callbacks.tokens.back(), Eq(kError));
}


}  // namespace
}  // namespace redux
