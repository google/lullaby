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

#include "lullaby/modules/render/shader_snippets_selector.h"

#include <set>
#include <vector>
#include "lullaby/modules/render/sanitize_shader_source.h"

namespace lull {

namespace {
static constexpr int kUnspecifiedVersion = 0;
using StageSnippetsArray =
    std::array<std::vector<const ShaderSnippetDefT*>, kNumShaderStages>;

bool IsSubsetInSuperset(std::vector<HashValue> subset,
                        const std::set<HashValue>& superset) {
  std::sort(subset.begin(), subset.end());
  return std::includes(superset.cbegin(), superset.cend(), subset.cbegin(),
                       subset.cend());
}

ShaderSnippetVersionDefT GetSnippetVersion(const ShaderSnippetDefT& snippet,
                                           ShaderLanguage lang) {
  ShaderSnippetVersionDefT version;
  version.lang = lang;
  version.min_version = 0;
  version.max_version = 0;
  for (const auto& it : snippet.versions) {
    if (it.lang == lang) {
      return it;
    }
    if (it.lang == ShaderLanguage_GL_Compat) {
      version.min_version =
          ConvertShaderVersionFromCompat(it.min_version, lang);
      version.max_version =
          ConvertShaderVersionFromCompat(it.max_version, lang);
    }
  }
  return version;
}

int FindHighestMinimumVersion(const StageSnippetsArray& stages, int max_version,
                              ShaderLanguage shader_lang) {
  int highest_minimum = GetMinimumShaderVersion(shader_lang);
  for (const auto& snippets : stages) {
    for (const ShaderSnippetDefT* snippet : snippets) {
      const ShaderSnippetVersionDefT version =
          GetSnippetVersion(*snippet, shader_lang);
      if (version.min_version > highest_minimum &&
          (max_version == kUnspecifiedVersion ||
           version.min_version <= max_version)) {
        highest_minimum = version.min_version;
      }
    }
  }
  return highest_minimum ? highest_minimum : max_version;
}

std::vector<const ShaderSnippetDefT*> FindSnippetsForEnvironmentAndFeatures(
    const ShaderSelectionParams& params, const ShaderStageDefT& stage) {
  std::vector<const ShaderSnippetDefT*> supported_snippets;
  for (const auto& snippet : stage.snippets) {
    if (!IsSubsetInSuperset(snippet.environment, params.environment)) {
      continue;
    }
    if (!IsSubsetInSuperset(snippet.features, params.features)) {
      continue;
    }
    supported_snippets.push_back(&snippet);
  }
  return supported_snippets;
}

void RemoveUnsupportedSnippets(StageSnippetsArray* stages, int version,
                               ShaderLanguage shader_lang,
                               const ShaderSelectionParams& params) {
  for (auto& snippets : *stages) {
    std::set<HashValue> features_to_support = params.features;

    // Erase snipets according to criterias:
    snippets.erase(
        std::remove_if(
            snippets.begin(), snippets.end(),
            [&features_to_support, version,
             shader_lang](const ShaderSnippetDefT* snippet) {
              const ShaderSnippetVersionDefT snippet_version =
                  GetSnippetVersion(*snippet, shader_lang);
              if (snippet_version.min_version > version) {
                // Minimum version is higher than requested. Remove snippet.
                return true;
              }
              if (snippet_version.max_version != kUnspecifiedVersion &&
                  snippet_version.max_version <= version) {
                return true;
              }
              if (!IsSubsetInSuperset(snippet->features, features_to_support)) {
                // Features contained are unwanted. Remove this snippet.
                return true;
              }
              // Wanted snippet! Do not remove it.
              // Remove the features added by the snippet. This
              // guarantees features are only added once per shader
              // stage.
              for (const auto& feature : snippet->features) {
                features_to_support.erase(feature);
              }
              return false;
            }),
        snippets.end());
  }
}

StageSnippetsArray FindSupportedSnippets(const ShaderDefT& def,
                                         const ShaderSelectionParams& params,
                                         int* out_shader_version) {
  StageSnippetsArray supported_snippets;

  for (const auto& stage : def.stages) {
    supported_snippets[stage.type] =
        FindSnippetsForEnvironmentAndFeatures(params, stage);
  }
  // Find the shader version for snippets and compilation.
  const int shader_version = FindHighestMinimumVersion(
      supported_snippets, params.max_shader_version, params.lang);
  RemoveUnsupportedSnippets(&supported_snippets, shader_version, params.lang,
                            params);

  if (out_shader_version) {
    *out_shader_version = shader_version;
  }
  return supported_snippets;
}

bool ValidateSnippetInputsIncluded(
    const ShaderSnippetDefT& snippet,
    const std::vector<const ShaderSnippetDefT*>& snippets) {
  // Make sure that each input in the snippet is included at least once as an
  // output in the snippets vector.
  for (const auto& input : snippet.inputs) {
    bool found = false;
    for (const auto& snippet_iter : snippets) {
      // Is the input included in one of this snippet's outputs?
      if (std::find_if(
              snippet_iter->outputs.cbegin(), snippet_iter->outputs.cend(),
              [input](const ShaderAttributeDefT& def) {
                return (input.name == def.name && input.type == def.type);
              }) != snippet_iter->outputs.cend()) {
        // Found an output corresponding to the input. Break and move to the
        // next input.
        found = true;
        break;
      }
    }
    if (!found) {
      // An input was not found. Return false.
      return false;
    }
  }
  // All inputs have been found.
  return true;
}

bool ValidateSnippetOutputsIncluded(
    const ShaderSnippetDefT& snippet,
    const std::vector<const ShaderSnippetDefT*>& snippets) {
  // Make sure that each output in the snippet is included at least once as an
  // input in the snippets vector.
  for (const auto& output : snippet.outputs) {
    bool found = false;
    for (const auto& snippet_iter : snippets) {
      // Is the output included in one of this snippet's input?
      if (std::find_if(
              snippet_iter->inputs.cbegin(), snippet_iter->inputs.cend(),
              [output](const ShaderAttributeDefT& def) {
                return (output.name == def.name && output.type == def.type);
              }) != snippet_iter->inputs.cend()) {
        // Found an input corresponding to the output. Break and move to the
        // next output.
        found = true;
        break;
      }
    }
    if (!found) {
      // An output was not found. Return false.
      return false;
    }
  }
  // All outputs have been found.
  return true;
}

bool ShaderUniformDefsEquals(const ShaderUniformDefT& lhs,
                             const ShaderUniformDefT& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  } else if (lhs.array_size != rhs.array_size) {
    return false;
  } else if (lhs.fields.size() != rhs.fields.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.fields.size(); ++i) {
    if (!ShaderUniformDefsEquals(lhs.fields[i], rhs.fields[i])) {
      return false;
    }
  }
  return true;
}

bool ShaderSamplerDefsEqual(const ShaderSamplerDefT& lhs,
                            const ShaderSamplerDefT& rhs) {
  return lhs.usage_per_channel == rhs.usage_per_channel &&
         lhs.usage == rhs.usage && lhs.type == rhs.type;
}

bool CompareShaderAttributeDefs(const ShaderAttributeDefT& lhs,
                                const ShaderAttributeDefT& rhs) {
  return lhs.type == rhs.type && lhs.usage == rhs.usage;
}

bool GatherInputs(const ShaderSnippetDefT& snippet,
                  std::vector<ShaderAttributeDefT>* inputs) {
  // Add the input to the inputs list.
  for (const auto& input : snippet.inputs) {
    // Check for an existing input with the same name and that they match in
    // definition.
    if (!ValidateAndAddAttributeDef(input, inputs)) {
      LOG(DFATAL) << "Input validation failed.";
      return false;
    }
  }
  return true;
}

bool GatherOutputs(const ShaderSnippetDefT& snippet,
                   std::vector<ShaderAttributeDefT>* outputs) {
  // Add the input to the outputs list.
  for (const auto& output : snippet.outputs) {
    // Check for an existing input with the same name and that they match in
    // definition.
    if (!ValidateAndAddAttributeDef(output, outputs)) {
      LOG(DFATAL) << "Output validation failed.";
      return false;
    }
  }
  return true;
}

bool GatherUniforms(const ShaderSnippetDefT& snippet,
                    std::vector<ShaderUniformDefT>* uniforms) {
  // Add the uniforms to the shader description.
  for (const auto& uniform : snippet.uniforms) {
    // Check for an existing uniform with the same name and that they match
    // in definition.
    if (!ValidateAndAddUniformDef(uniform, uniforms)) {
      LOG(DFATAL) << "Uniform validation failed.";
      return false;
    }
  }
  return true;
}

bool GatherSamplers(const ShaderSnippetDefT& snippet,
                    std::vector<ShaderSamplerDefT>* samplers) {
  // Add the uniforms to the shader description.
  for (const auto& sampler : snippet.samplers) {
    // Check for an existing sampler with the same name and that they match
    // in definition.
    if (!ValidateAndAddSamplerDef(sampler, samplers)) {
      LOG(DFATAL) << "Sampler validation failed.";
      return false;
    }
  }
  return true;
}

void PrintShaderSnippetInfo(const ShaderSelectionParams& params,
                            const ShaderDefT& def,
                            const StageSnippetsArray& snippets) {
  std::stringstream ss;
  auto LogHash = [&](const char* indent, HashValue value) {
    #ifdef LULLABY_DEBUG_HASH
      ss << indent << value << " " << Unhash(value) << "\n";
    #else
      ss << indent << value << "\n";
    #endif
  };

  // Dump out features and environment for the selection parameters.
  ss << "Selection Parameters\n";
  ss << "  Features:\n";
  for (HashValue feature : params.features) {
    LogHash("    ", feature);
  }
  ss << "\n";
  ss << "  Environment:\n";
  for (HashValue environment : params.environment) {
    LogHash("    ", environment);
  }
  ss << "\n";

  // Dump out features and environments for each snippet in the shader def.
  ss << "Shader Snippets\n";
  for (const auto& stage : def.stages) {
    for (const auto& snippet : stage.snippets) {
      ss << "  Snippet: " << snippet.name << std::endl;
      ss << "    Features:\n";
      for (HashValue feature : snippet.features) {
        LogHash("      ", feature);
      }

      ss << "    Environment:\n";
      for (HashValue environment : snippet.environment) {
        LogHash("      ", environment);
      }
    }
    ss << "\n";
  }

  // Dump out list of snippets actually selected.
  ss << "Selection Results\n";
  for (size_t i = 0; i < snippets.size(); ++i) {
    ss << " Stage " << i << std::endl;
    for (const auto& snippet : snippets[i]) {
      ss << "    " << snippet->name << std::endl;
    }
  }

  LOG(INFO) << "\n" << ss.str();
}
}  // namespace

int GetMinimumShaderVersion(ShaderLanguage shader_lang) {
  if (shader_lang == ShaderLanguage_GLSL_ES ||
      shader_lang == ShaderLanguage_GL_Compat) {
    return 100;
  } else if (shader_lang == ShaderLanguage_GLSL) {
    return 110;
  } else {
    LOG(DFATAL) << "Undefined minimum shader for shader language: "
                << shader_lang;
    return 100;
  }
}

SnippetSelectionResult SelectShaderSnippets(
    const ShaderDefT& def, const ShaderSelectionParams& params) {

  SnippetSelectionResult result;

  // Find supported snippets from each stage.
  StageSnippetsArray snippets =
      FindSupportedSnippets(def, params, &result.shader_version);

#if defined(SHADER_DEBUG)
  PrintShaderSnippetInfo(params, def, snippets);
#endif

  // Remove mismatching inputs/outputs.
  bool first_nonempty_stage = true;
  for (int stage_index = 0; stage_index < kNumShaderStages; ++stage_index) {
    std::vector<const ShaderSnippetDefT*>& current_stage =
        snippets[stage_index];
    auto is_invalid_snippet = [&](const ShaderSnippetDefT* snippet) {
      if (snippet == nullptr) {
        return true;
      }
      // Check inputs of this snippet are included as outputs of snippets in the
      // previous stage (skip if this is the first non-empty shader stage).
      if (!first_nonempty_stage &&
          !ValidateSnippetInputsIncluded(*snippet, snippets[stage_index - 1])) {
        return true;
      }
      // Check that outputs of this snippet are included as outputs of next
      // stage (unless this is the final stage. The final shader stage is always
      // ShaderStageType_MAX [= ShaderStageType_FRAGMENT] so we only need to
      // make sure we don't attempt to access beyond that.
      if (stage_index < ShaderStageType_MAX &&
          !ValidateSnippetOutputsIncluded(*snippet,
                                          snippets[stage_index + 1])) {
        return true;
      }
      return false;
    };
    current_stage.erase(std::remove_if(current_stage.begin(),
                                       current_stage.end(), is_invalid_snippet),
                        current_stage.end());

    if (!current_stage.empty()) {
      first_nonempty_stage = false;
    }

    // Gather inputs and outputs:
    const ShaderStageType stage_type =
        static_cast<ShaderStageType>(stage_index);
    for (const auto& snippet : current_stage) {
      if (!GatherInputs(*snippet, &result.stages[stage_type].inputs)) {
        return result;
      }
      if (!GatherOutputs(*snippet, &result.stages[stage_type].outputs)) {
        return result;
      }
      if (!GatherUniforms(*snippet, &result.stages[stage_type].uniforms)) {
        return result;
      }
      if (!GatherSamplers(*snippet, &result.stages[stage_type].samplers)) {
        return result;
      }
      if (!snippet->name.empty()) {
        result.stages[stage_type].snippet_names.emplace_back(snippet->name);
      }
      if (!snippet->code.empty()) {
        result.stages[stage_type].code.emplace_back(snippet->code);
      }
      if (!snippet->main_code.empty()) {
        result.stages[stage_type].main.emplace_back(snippet->main_code);
      }
    }
  }

  return result;
}

bool ValidateAndAddUniformDef(const ShaderUniformDefT& uniform,
                              std::vector<ShaderUniformDefT>* uniforms) {
  if (!uniforms) {
    LOG(DFATAL) << "Did not supply uniforms vector.";
    return false;
  }

  auto iter = std::find_if(
      uniforms->begin(), uniforms->end(),
      [&](const ShaderUniformDefT& def) { return def.name == uniform.name; });
  if (iter == uniforms->end()) {
    uniforms->push_back(uniform);
  } else if (!ShaderUniformDefsEquals(*iter, uniform)) {
    LOG(DFATAL) << "Snippets contain uniforms with same name (" << uniform.name
                << ") but different definitions.";
    return false;
  }
  return true;
}

bool ValidateAndAddAttributeDef(const ShaderAttributeDefT& attribute,
                                std::vector<ShaderAttributeDefT>* attributes) {
  if (!attributes) {
    LOG(DFATAL) << "Did not supply attributes vector.";
    return false;
  }

  auto iter = std::find_if(attributes->begin(), attributes->end(),
                           [&](const ShaderAttributeDefT& def) {
                             return def.name == attribute.name;
                           });
  if (iter == attributes->end()) {
    attributes->push_back(attribute);
  } else if (!CompareShaderAttributeDefs(*iter, attribute)) {
    LOG(DFATAL) << "Snippets contain attributes with same name ("
                << attribute.name << ") but different definitions.";
    return false;
  }
  return true;
}

bool ValidateAndAddSamplerDef(const ShaderSamplerDefT& sampler,
                              std::vector<ShaderSamplerDefT>* samplers) {
  if (!samplers) {
    LOG(DFATAL) << "Did not supply samplers vector.";
    return false;
  }

  auto iter = std::find_if(
      samplers->begin(), samplers->end(),
      [&](const ShaderSamplerDefT& def) { return def.name == sampler.name; });
  if (iter == samplers->end()) {
    samplers->push_back(sampler);
  } else if (!ShaderSamplerDefsEqual(*iter, sampler)) {
    LOG(DFATAL) << "Snippets contain samplers with same name (" << sampler.name
                << ") but different definitions.";
    return false;
  }

  return true;
}

ShaderDescription CreateShaderDescription(string_view shading_model,
                                          const ShaderStagesArray& stages) {
  ShaderDescription desc;
  desc.shading_model = std::string(shading_model);

  // Add the uniforms and samplers.
  for (const auto& stage : stages) {
    for (const auto& uniform : stage.uniforms) {
      ValidateAndAddUniformDef(uniform, &desc.uniforms);
    }

    for (const auto& sampler : stage.samplers) {
      ValidateAndAddSamplerDef(sampler, &desc.samplers);
    }
  }

  // Copy the vertex stage inputs to the shader description.
  for (const auto& input : stages[ShaderStageType_Vertex].inputs) {
    ValidateAndAddAttributeDef(input, &desc.attributes);
  }

  return desc;
}

}  // namespace lull
