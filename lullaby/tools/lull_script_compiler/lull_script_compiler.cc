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

#include <fstream>
#include <iostream>
#include <string>
#include "lullaby/modules/lullscript/script_compiler.h"

// Reads the contents of the given file into a std::string.
std::string ReadFile(const char* filename) {
  std::ifstream fin(filename);
  std::string data((std::istreambuf_iterator<char>(fin)),
                   std::istreambuf_iterator<char>());
  fin.close();
  return data;
}

// Writes the given byte array to the specified file.
void WriteFile(const char* filename, const uint8_t* data, size_t size) {
  std::fstream fout(filename, std::ios::out | std::ios::binary);
  fout.write(reinterpret_cast<const char*>(data), size);
  fout.close();
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: script_compiler [input file] [output file]"
              << std::endl;
    return -1;
  }

  const std::string source = ReadFile(argv[1]);
  if (source.empty()) {
    std::cerr << "Error reading file: " << argv[1] << std::endl;
    return -1;
  }


  lull::ScriptByteCode buffer;
  lull::ScriptCompiler compiler(&buffer);
  lull::ParseScript(source, &compiler);

  if (buffer.empty()) {
    std::cerr << "Error compiling file: " << argv[1] << std::endl;
    return -1;
  }

  WriteFile(argv[2], buffer.data(), buffer.size());
  return 0;
}
