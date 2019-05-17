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

#include "lullaby/viewer/src/widgets/file_dialog.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>

namespace lull {
namespace tool {

template <typename Fn>
static std::string RunQtDialog(Fn fn) {
  int argc = 1;
  const char* argv[] = {""};
  QApplication app(argc, (char**)argv);
  const QString result = fn();
  return result.toUtf8().constData();
}

std::string OpenFileDialog(const std::string& label,
                           const std::string& filter) {
  return RunQtDialog([&]() {
    return QFileDialog::getOpenFileName(nullptr, label.c_str(), "",
                                        filter.c_str());
  });
}

std::string OpenDirectoryDialog(const std::string& label) {
  return RunQtDialog([&]() {
    return QFileDialog::getExistingDirectory(
        nullptr, label.c_str(), "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  });
}

std::string SaveFileDialog(const std::string& label) {
  return RunQtDialog([&]() {
    return QFileDialog::getSaveFileName(nullptr, label.c_str(), "", "");
  });
}

}  // namespace tool
}  // namespace lull
