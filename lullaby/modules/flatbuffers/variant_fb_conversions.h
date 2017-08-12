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

#ifndef LULLABY_MODULES_FLATBUFFERS_VARIANT_FB_CONVERSIONS_H_
#define LULLABY_MODULES_FLATBUFFERS_VARIANT_FB_CONVERSIONS_H_

#include "lullaby/util/variant.h"
#include "lullaby/generated/variant_def_generated.h"

namespace lull {

/// Converts a flatbuffer to a Variant.  The flatbuffer |type| can be any of the
/// Data* tables (ex. DataBool) defined in variant_def.fbs.  The |type| must
/// match |in| based on the VariantDef union.
bool VariantFromFbVariant(VariantDef type, const void* in, Variant* out);

/// Converts a flatbuffer VariantArrayDef to a VariantArray.
bool VariantArrayFromFbVariantArray(const VariantArrayDef* in,
                                    VariantArray* out);

/// Converts a flatbuffer VariantMapDef to a VariantMap.
bool VariantMapFromFbVariantMap(const VariantMapDef* in, VariantMap* out);

}  // namespace lull

#endif  // LULLABY_MODULES_FLATBUFFERS_VARIANT_FB_CONVERSIONS_H_
