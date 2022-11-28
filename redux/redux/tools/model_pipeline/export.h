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

#ifndef REDUX_TOOLS_MODEL_PIPELINE_EXPORT_H_
#define REDUX_TOOLS_MODEL_PIPELINE_EXPORT_H_

#include "absl/types/span.h"
#include "redux/modules/base/data_container.h"
#include "redux/tools/common/log_utils.h"
#include "redux/tools/model_pipeline/model.h"

namespace redux::tool {

// Generates a DataContainer storing an ModelAssetDdef binary object from the
// provided models.
DataContainer ExportModel(absl::Span<const ModelPtr> lods, ModelPtr skeleton,
                          ModelPtr collidable, Logger& log);

}  // namespace redux::tool

#endif  // REDUX_TOOLS_MODEL_PIPELINE_EXPORT_H_
