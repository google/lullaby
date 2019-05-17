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

#ifndef LULLABY_UTIL_ARG_PARSER_H_
#define LULLABY_UTIL_ARG_PARSER_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "lullaby/util/arg.h"
#include "lullaby/util/hash.h"
#include "lullaby/util/span.h"

namespace lull {

// A simple parser for command-line arguments.
class ArgParser {
 public:
  ArgParser() {}

  // Defines an argument that will be processed by the parser.
  Arg& AddArg(string_view name);

  // Parses the command line arguments and stores the processed results.
  // Returns false if an error was encountered while parsing the arguments.
  bool Parse(int argc, const char** argv);

  // Returns the list of errors encountered during parsing.
  const std::vector<std::string>& GetErrors() const;

  // Returns the usage string that can be displayed.
  std::string GetUsage() const;

  // Returns argv[0], the string describing the called program
  std::string GetProgram() const;

  // Returns true if the specified argument was set.
  bool IsSet(string_view name) const;

  // Returns the number of values set for a specified argument.
  size_t GetNumValues(string_view name) const;

  // Returns the value associated with the argument as an int.
  int GetInt(string_view name, size_t index = 0) const;

  // Returns the value associated with the argument as a bool.
  bool GetBool(string_view name) const;

  // Returns the value associated with the argument as a float.
  float GetFloat(string_view name, size_t index = 0) const;

  // Returns the value associated with the argument as a string.
  string_view GetString(string_view name, size_t index = 0) const;

  // Returns all values associated with an argument.
  Span<string_view> GetValues(string_view name) const;

  // Returns the list of arguments that were parsed but not defined/known by
  // the parser.
  const std::vector<string_view>& GetPositionalArgs() const;

 private:
  const Arg* FindArgByName(string_view name) const;
  const Arg* FindArgByShortName(string_view name) const;

  bool HasValue(string_view name) const;
  string_view GetValue(string_view name, size_t index) const;
  void AddValue(string_view name, string_view value);

  std::string program_;
  std::vector<Arg> args_;
  std::vector<std::string> errors_;
  std::vector<string_view> positional_values_;
  std::unordered_map<string_view, std::vector<string_view>, Hasher> values_;
};

}  // namespace lull

#endif  // LULLABY_UTIL_ARG_PARSER_H_
