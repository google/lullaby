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

#include <iomanip>
#include <sstream>
#include "lullaby/util/logging.h"

namespace lull {

static int ToInt(string_view str) {
  return static_cast<int>(strtol(str.data(), nullptr, 10));
}

static float ToFloat(string_view str) { return strtof(str.data(), nullptr); }

Arg& ArgParser::AddArg(string_view name) {
  args_.emplace_back(name);
  return args_.back();
}

bool ArgParser::Parse(int argc, const char** argv) {
  errors_.clear();
  if (argc > 0) {
    program_ = argv[0];
  }
  for (int i = 1; i < argc; ++i) {
    string_view argstr = argv[i];

    // Arg doesn't start with a hyphen, so just add it to the unnamed args.
    if (argstr[0] != '-' || argstr.length() == 1) {
      positional_values_.emplace_back(argstr);
      continue;
    }

    // Arg is multiple short-named arguments, so handle appropriately.
    if (argstr[1] != '-' && argstr.length() > 2) {
      argstr = argstr.substr(1);

      while (argstr.length() > 0) {
        const Arg* arg = FindArgByShortName(argstr);
        argstr = argstr.substr(1);

        if (arg == nullptr) {
          std::stringstream ss;
          ss << "No such flag: " << argstr[0];
          errors_.emplace_back(ss.str());
          continue;
        } else if (arg->HasAssociatedArgs()) {
          std::stringstream ss;
          ss << "Expected value following argument: " << argstr[0];
          errors_.emplace_back(ss.str());
          continue;
        }
        AddValue(arg->GetName(), "");
      }
      continue;
    }

    const Arg* arg = nullptr;
    if (argstr[1] != '-') {
      argstr = argstr.substr(1, 1);
      arg = FindArgByShortName(argstr);
    } else {
      argstr = argstr.substr(2);
      arg = FindArgByName(argstr);
    }

    if (arg == nullptr) {
      std::stringstream ss;
      ss << "Invalid argument: " << argstr;
      errors_.emplace_back(ss.str());
    } else if (arg->HasAssociatedArgs() && arg->GetDefaultValue().empty() &&
               i == argc - 1) {
      std::stringstream ss;
      ss << "Expected value following argument: " << argstr;
      errors_.emplace_back(ss.str());
    } else if (!arg->HasAssociatedArgs()) {
      AddValue(arg->GetName(), "");
    } else if (arg->IsVariableNumArgs()) {
      int n = 0;
      while (true) {
        const int index = n + i + 1;
        if (index >= argc) {
          break;
        }
        string_view value = argv[index];
        if (value[0] == '-') {
          break;
        }
        AddValue(arg->GetName(), value);
        ++n;
      }
      // Consume the found number of arguments.
      i += n;
    } else {
      for (int n = 0; n < arg->GetNumArgs(); ++n) {
        const int index = n + i + 1;
        if (index >= argc) {
          continue;
        }
        string_view value = argv[index];
        AddValue(arg->GetName(), value);
      }
      // Consume the specified number of arguments.
      i += arg->GetNumArgs();
    }
  }

  for (const Arg& arg : args_) {
    auto iter = values_.find(arg.GetName());
    if (iter == values_.end()) {
      if (arg.IsRequired()) {
        std::stringstream ss;
        ss << "Missing required argument: " << arg.GetName();
        errors_.emplace_back(ss.str());
      } else if (!arg.GetDefaultValue().empty()) {
        AddValue(arg.GetName(), arg.GetDefaultValue());
      }
    }
  }

  return errors_.empty();
}

bool ArgParser::IsSet(string_view name) const {
  return HasValue(name);
}

size_t ArgParser::GetNumValues(string_view name) const {
  auto iter = values_.find(name);
  if (iter == values_.end()) {
    return 0;
  } else {
    return iter->second.size();
  }
}

int ArgParser::GetInt(string_view name, size_t index) const {
  return ToInt(GetValue(name, index));
}

bool ArgParser::GetBool(string_view name) const {
  return HasValue(name);
}

float ArgParser::GetFloat(string_view name, size_t index) const {
  return ToFloat(GetValue(name, index));
}

string_view ArgParser::GetString(string_view name, size_t index) const {
  return GetValue(name, index);
}

const std::vector<string_view>& ArgParser::GetPositionalArgs() const {
  return positional_values_;
}

Span<string_view> ArgParser::GetValues(string_view name) const {
  auto iter = values_.find(name);
  if (iter != values_.end()) {
    return Span<string_view>(iter->second);
  } else {
    return Span<string_view>();
  }
}

const Arg* ArgParser::FindArgByName(string_view name) const {
  for (const Arg& arg : args_) {
    if (arg.GetName() == name) {
      return &arg;
    }
  }
  return nullptr;
}

const Arg* ArgParser::FindArgByShortName(string_view name) const {
  for (const Arg& arg : args_) {
    if (arg.GetShortName() == name[0]) {
      return &arg;
    }
  }
  return nullptr;
}

bool ArgParser::HasValue(string_view name) const {
  auto iter = values_.find(name);
  return (iter != values_.end());
}

string_view ArgParser::GetValue(string_view name, size_t index) const {
  auto iter = values_.find(name);
  if (iter != values_.end() && iter->second.size() > index) {
    return iter->second[index];
  } else {
    return string_view();
  }
}

void ArgParser::AddValue(string_view name, string_view value) {
  values_[name].push_back(value);
}

const std::vector<std::string>& ArgParser::GetErrors() const {
  return errors_;
}

std::string ArgParser::GetProgram() const {
  return program_;
}

std::string ArgParser::GetUsage() const {
  std::stringstream ss;
  auto GetArgNames = [](const Arg& arg) -> std::string {
    std::stringstream ss;
    ss << " --" << arg.GetName().data();
    if (arg.GetShortName()) {
      ss << ", -" << arg.GetShortName();
    }
    return ss.str();
  };
  int max_length = 0;
  for (auto& arg : args_) {
    if (!arg.IsDeprecated()) {
      max_length =
          std::max(max_length, static_cast<int>(GetArgNames(arg).size()));
    }
  }
  max_length++;
  for (auto& arg : args_) {
    if (!arg.IsDeprecated()) {
      ss << std::setw(max_length) << std::left << GetArgNames(arg);
      ss << arg.GetDescription().data();
      ss << std::endl;
    }
  }
  return ss.str();
}

}  // namespace lull
