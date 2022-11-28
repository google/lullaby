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

#include "redux/engines/script/redux/script_env.h"

#include <optional>

#include "redux/engines/script/redux/functions/array.h"
#include "redux/engines/script/redux/functions/cond.h"
#include "redux/engines/script/redux/functions/hash.h"
#include "redux/engines/script/redux/functions/map.h"
#include "redux/engines/script/redux/functions/math.h"
#include "redux/engines/script/redux/functions/message.h"
#include "redux/engines/script/redux/functions/operators.h"
#include "redux/engines/script/redux/functions/typeof.h"
#include "redux/engines/script/redux/script_ast_builder.h"
#include "redux/engines/script/redux/script_frame.h"
#include "redux/engines/script/redux/script_parser.h"
#include "redux/modules/base/hash.h"
#include "redux/modules/ecs/entity.h"
#include "redux/modules/math/quaternion.h"
#include "redux/modules/math/vector.h"

namespace redux {
namespace {

template <typename T>
void CastFn(ScriptFrame* frame) {
  const ScriptValue arg = frame->EvalNext();
  T res{};
  if (arg.GetAs(&res)) {
    frame->Return(res);
    return;
  }
  CHECK(false) << "Unable to cast type";
}

template <typename T, typename... Args>
T CreateTypeFn(Args... args) {
  return T(std::forward<Args>(args)...);
}
}  // namespace

std::string Stringify(ScriptFrame* frame);
std::string Stringify(const ScriptValue& value);

ScriptEnv::ScriptEnv(ScriptStack* globals)
    : globals_(globals), rng_engine_(std::random_device()()) {
  auto eval_fn = [](ScriptFrame* frame) {
    frame->Return(frame->GetEnv()->Eval(frame->GetArgs()));
  };
  auto let_fn = [](ScriptFrame* frame) {
    frame->Return(frame->GetEnv()->SetImpl(frame->GetArgs(), kLetPrimitive));
  };
  auto set_fn = [](ScriptFrame* frame) {
    frame->Return(frame->GetEnv()->SetImpl(frame->GetArgs(), kSetPrimitive));
  };
  auto def_fn = [](ScriptFrame* frame) {
    frame->Return(frame->GetEnv()->SetImpl(frame->GetArgs(), kFunction));
  };
  auto mac_fn = [](ScriptFrame* frame) {
    frame->Return(frame->GetEnv()->SetImpl(frame->GetArgs(), kMacro));
  };
  auto ret_fn = [](ScriptFrame* frame) {
    frame->Return(
        DefReturn(frame->HasNext() ? frame->EvalNext() : ScriptValue()));
  };
  auto do_fn = [](ScriptFrame* frame) {
    frame->Return(frame->GetEnv()->DoImpl(frame->Next()));
  };
  auto begin_fn = [](ScriptFrame* frame) {
    frame->GetEnv()->PushScope();
    frame->Return(frame->GetEnv()->DoImpl(frame->Next()));
    frame->GetEnv()->PopScope();
  };
  auto lambda_fn = [](ScriptFrame* frame) {
    const AstNode* node = frame->GetArgs().Get<AstNode>();
    if (!node) {
      frame->GetEnv()->Error("Invalid lambda definition.", frame->GetArgs());
      return;
    }
    if (node->first.Is<AstNode>() == false) {
      frame->GetEnv()->Error("Expected arguments.", node->first);
      return;
    } else if (node->rest.Is<AstNode>() == false) {
      frame->GetEnv()->Error("Expected expression.", node->rest);
      return;
    }

    frame->Return(Lambda(node->first, node->rest));
  };
  auto print_fn = [](ScriptFrame* frame) {
    std::stringstream ss;
    while (frame->HasNext()) {
      ss << Stringify(frame->EvalNext());
      if (frame->HasNext()) {
        ss << " ";
      }
    }
    auto str = ss.str();
    LOG(INFO) << str;
    frame->Return(str);
  };
  auto randi_fn = [this](ScriptFrame* frame) {
    int min = 0;
    int max = 0;
    frame->EvalNext().GetAs(&min);
    frame->EvalNext().GetAs(&max);
    const int result =
        std::uniform_int_distribution<int>(min, max)(this->rng_engine_);
    frame->Return(result);
  };
  auto randf_fn = [this](ScriptFrame* frame) {
    float min = 0.f;
    float max = 0.f;
    frame->EvalNext().GetAs(&min);
    frame->EvalNext().GetAs(&max);
    const float result =
        std::uniform_real_distribution<float>(min, max)(this->rng_engine_);
    frame->Return(result);
  };

  RegisterFunction(ConstHash("="), set_fn);
  RegisterFunction(ConstHash("do"), do_fn);
  RegisterFunction(ConstHash("begin"), begin_fn);
  RegisterFunction(ConstHash("def"), def_fn);
  RegisterFunction(ConstHash("var"), let_fn);
  RegisterFunction(ConstHash("eval"), eval_fn);
  RegisterFunction(ConstHash("macro"), mac_fn);
  RegisterFunction(ConstHash("lambda"), lambda_fn);
  RegisterFunction(ConstHash("return"), ret_fn);
  RegisterFunction(ConstHash("?"), print_fn);
  RegisterFunction(ConstHash("randi"), randi_fn);
  RegisterFunction(ConstHash("randf"), randf_fn);

  RegisterFunction(ConstHash("int8"), CastFn<int8_t>);
  RegisterFunction(ConstHash("int16"), CastFn<int16_t>);
  RegisterFunction(ConstHash("int32"), CastFn<int32_t>);
  RegisterFunction(ConstHash("int64"), CastFn<int64_t>);
  RegisterFunction(ConstHash("uint8"), CastFn<uint8_t>);
  RegisterFunction(ConstHash("uint16"), CastFn<uint16_t>);
  RegisterFunction(ConstHash("uint32"), CastFn<uint32_t>);
  RegisterFunction(ConstHash("uint64"), CastFn<uint64_t>);
  RegisterFunction(ConstHash("float"), CastFn<float>);
  RegisterFunction(ConstHash("double"), CastFn<double>);

  RegisterFunction(ConstHash("is?"), script_functions::IsFn);
  RegisterFunction(ConstHash("nil?"), script_functions::IsNilFn);
  RegisterFunction(ConstHash("typeof"), script_functions::TypeOfFn);

  RegisterFunction(ConstHash("=="), script_functions::EqFn);
  RegisterFunction(ConstHash("!="), script_functions::NeFn);
  RegisterFunction(ConstHash("<="), script_functions::LeFn);
  RegisterFunction(ConstHash("<"), script_functions::LtFn);
  RegisterFunction(ConstHash(">="), script_functions::GeFn);
  RegisterFunction(ConstHash(">"), script_functions::GtFn);
  RegisterFunction(ConstHash("+"), script_functions::AddFn);
  RegisterFunction(ConstHash("-"), script_functions::SubFn);
  RegisterFunction(ConstHash("*"), script_functions::MulFn);
  RegisterFunction(ConstHash("/"), script_functions::DivFn);
  RegisterFunction(ConstHash("%"), script_functions::ModFn);
  RegisterFunction(ConstHash("and"), script_functions::AndFn);
  RegisterFunction(ConstHash("or"), script_functions::OrFn);
  RegisterFunction(ConstHash("not"), script_functions::NotFn);
  RegisterFunction(ConstHash("cond"), script_functions::CondFn);
  RegisterFunction(ConstHash("if"), script_functions::IfFn);

  RegisterFunction(ConstHash("hash"), script_functions::HashFn);
  RegisterFunction(ConstHash("entity"),
                   CreateTypeFn<Entity, uint32_t>);

  RegisterFunction(ConstHash("vec2i"), CreateTypeFn<vec2i, int, int>);
  RegisterFunction(ConstHash("vec3i"), CreateTypeFn<vec3i, int, int, int>);
  RegisterFunction(ConstHash("vec4i"), CreateTypeFn<vec4i, int, int, int, int>);
  RegisterFunction(ConstHash("vec2"), CreateTypeFn<vec2, float, float>);
  RegisterFunction(ConstHash("vec3"), CreateTypeFn<vec3, float, float, float>);
  RegisterFunction(ConstHash("vec4"),
                   CreateTypeFn<vec4, float, float, float, float>);
  RegisterFunction(ConstHash("quat"),
                   CreateTypeFn<quat, float, float, float, float>);
  RegisterFunction(ConstHash("get-x"), script_functions::GetElementFn<0>);
  RegisterFunction(ConstHash("get-y"), script_functions::GetElementFn<1>);
  RegisterFunction(ConstHash("get-z"), script_functions::GetElementFn<2>);
  RegisterFunction(ConstHash("get-w"), script_functions::GetElementFn<3>);
  RegisterFunction(ConstHash("set-x"), script_functions::SetElementFn<0>);
  RegisterFunction(ConstHash("set-y"), script_functions::SetElementFn<1>);
  RegisterFunction(ConstHash("set-z"), script_functions::SetElementFn<2>);
  RegisterFunction(ConstHash("set-w"), script_functions::SetElementFn<3>);
  // quat.from_euler_angles ?
  RegisterFunction(ConstHash("seconds"), absl::Seconds<int>);
  RegisterFunction(ConstHash("milliseconds"), absl::Milliseconds<int>);

  RegisterFunction(ConstHash("make-array"), script_functions::ArrayCreateFn);
  RegisterFunction(ConstHash("array-size"), script_functions::ArraySizeFn);
  RegisterFunction(ConstHash("array-empty"), script_functions::ArrayEmptyFn);
  RegisterFunction(ConstHash("array-push"), script_functions::ArrayPushFn);
  RegisterFunction(ConstHash("array-pop"), script_functions::ArrayPopFn);
  RegisterFunction(ConstHash("array-insert"), script_functions::ArrayInsertFn);
  RegisterFunction(ConstHash("array-erase"), script_functions::ArrayEraseFn);
  RegisterFunction(ConstHash("array-set"), script_functions::ArraySetFn);
  RegisterFunction(ConstHash("array-at"), script_functions::ArrayAtFn);
  RegisterFunction(ConstHash("array-foreach"),
                   script_functions::ArrayForEachFn);

  RegisterFunction(ConstHash("make-map"), script_functions::MapCreateFn);
  RegisterFunction(ConstHash("map-size"), script_functions::MapSizeFn);
  RegisterFunction(ConstHash("map-empty"), script_functions::MapEmptyFn);
  RegisterFunction(ConstHash("map-insert"), script_functions::MapInsertFn);
  RegisterFunction(ConstHash("map-erase"), script_functions::MapEraseFn);
  RegisterFunction(ConstHash("map-get"), script_functions::MapGetFn);
  RegisterFunction(ConstHash("map-get-or"), script_functions::MapGetOrFn);
  RegisterFunction(ConstHash("map-set"), script_functions::MapSetFn);
  RegisterFunction(ConstHash("map-foreach"), script_functions::MapForEachFn);

  RegisterFunction(ConstHash("make-msg"), script_functions::MessageCreateFn);
  RegisterFunction(ConstHash("msg-type"), script_functions::MessageTypeFn);
  RegisterFunction(ConstHash("msg-get"), script_functions::MessageGetFn);
  RegisterFunction(ConstHash("msg-get-or"), script_functions::MessageGetOrFn);
}

void ScriptEnv::Error(const char* msg, const ScriptValue& context) {
  ScriptFrame frame(this, context);
  LOG(ERROR) << "Script Error:";
  LOG(ERROR) << "  Message: " << msg;
  LOG(ERROR) << "  Context: " << Stringify(&frame);
  CHECK(false) << msg;
}

ScriptValue ScriptEnv::Read(std::string_view src) {
  ScriptAstBuilder builder;
  ParseScript(src, &builder);
  return ScriptValue(*builder.GetRoot());
}

ScriptValue ScriptEnv::Exec(std::string_view src) { return Eval(Read(src)); }

void ScriptEnv::SetValue(HashValue id, ScriptValue value) {
  stack_.SetValue(id, std::move(value));
}

void ScriptEnv::LetValue(HashValue id, ScriptValue value) {
  stack_.LetValue(id, std::move(value));
}

ScriptValue ScriptEnv::GetValue(HashValue id) {
  auto value = stack_.GetValue(id);
  if (!value && globals_) {
    value = globals_->GetValue(id);
  }
  return value;
}

ScriptValue ScriptEnv::Eval(const ScriptValue& script) {
  ScriptValue result;
  if (const AstNode* node = script.Get<AstNode>()) {
    const AstNode* child = node->first.Get<AstNode>();
    if (child) {
      result = CallInternal(child->first, child->rest);
    } else {
      result = Eval(node->first);
    }
  } else if (const Symbol* symbol = script.Get<Symbol>()) {
    result = Eval(GetValue(symbol->value));
  } else {
    result = script;
  }
  return result;
}

ScriptValue ScriptEnv::CallInternal(const ScriptValue& callable,
                                    const ScriptValue& args) {
  ScriptValue func = callable;
  if (const AstNode* node = func.Get<AstNode>()) {
    func = Eval(func);
  }

  if (const Symbol* symbol = func.Get<Symbol>()) {
    ScriptValue value = GetValue(symbol->value);
    if (value && !value.IsNil()) {
      func = value;
    }
  }

  // Execute the function depending on what kind of callable type it is.
  ScriptValue result;
  if (const NativeFunction* script = func.Get<NativeFunction>()) {
    ScriptFrame frame(this, args);
    script->fn(&frame);
    result = frame.ReleaseReturnValue();
  } else if (const Lambda* lambda = func.Get<Lambda>()) {
    stack_.PushScope();
    if (AssignArgs(&lambda->params, &args, true)) {
      result = DoImpl(lambda->body);
    }
    stack_.PopScope();
  } else if (const Macro* macro = func.Get<Macro>()) {
    if (AssignArgs(&macro->params, &args, false)) {
      result = DoImpl(macro->body);
    }
  } else {
    Error("Expected callable type.", func);
  }
  return result;
}

bool ScriptEnv::AssignArgs(const ScriptValue* params, const ScriptValue* args,
                           bool eval) {
  // Track the values that will be assigned to the parameter variables within
  // the scope of the function or macro call.  We need to evaluate all the
  // arguments before assigning them.
  static const int kMaxArgs = 16;
  int count = 0;
  HashValue symbols[kMaxArgs];
  ScriptValue values[kMaxArgs];
  while (!args->IsNil() && !params->IsNil()) {
    const AstNode* args_node = args->Get<AstNode>();
    if (args_node == nullptr) {
      Error("Expected a node for the arguments.", *args);
      return false;
    }

    const AstNode* params_node = params->Get<AstNode>();
    if (params_node == nullptr) {
      Error("Expected a node for the parameters.", *params);
      return false;
    }

    const Symbol* symbol = params_node->first.Get<Symbol>();
    if (!symbol) {
      Error("Parameter should be a symbol.", *params);
      return false;
    }

    if (count >= kMaxArgs) {
      Error("Too many arguments, limit of 16.", *args);
      return false;
    }

    // For lambdas/functions, the argument needs to be evaluated before being
    // assigned to the parameter. For macros, the parameter should be set to the
    // AstNode passed in as the argument.
    if (eval) {
      values[count] = Eval(*args);
    } else {
      values[count] = *args;
    }
    symbols[count] = symbol->value;
    ++count;

    // Go to the next parameter and argument.
    args = &args_node->rest;
    params = &params_node->rest;
  }

  if (!args->IsNil()) {
    Error("Too many arguments.", *args);
    return false;
  } else if (!params->IsNil()) {
    Error("Too few arguments.", *params);
    return false;
  }

  // Assign the evaluated argument values to the parameters.
  for (int i = 0; i < count; ++i) {
    LetValue(symbols[i], std::move(values[i]));
  }

  return true;
}

ScriptValue ScriptEnv::DoImpl(const ScriptValue& body) {
  if (body.IsNil()) {
    return body;
  }
  if (!body.Is<AstNode>()) {
    return body;
  }

  ScriptValue result;
  const ScriptValue* iter = &body;
  while (const AstNode* node = iter->Get<AstNode>()) {
    result = Eval(*iter);
    if (result) {
      if (auto* def_return = result.Get<DefReturn>()) {
        result = def_return->value;
        break;
      }
    }

    iter = &node->rest;
    if (iter == nullptr || iter->IsNil()) {
      break;
    }
  }
  return result;
}

ScriptValue ScriptEnv::SetImpl(const ScriptValue& args, ValueType type) {
  const AstNode* node = args.Get<AstNode>();
  if (!node) {
    Error("Invalid argument type.", args);
    return ScriptValue();
  } else if (node->first.Is<Symbol>() == false) {
    Error("Expected symbol.", node->first);
    return ScriptValue();
  } else if (node->rest.Is<AstNode>() == false) {
    Error("Expected expression.", node->rest);
    return ScriptValue();
  }

  ScriptValue result;
  if (type == kSetPrimitive || type == kLetPrimitive) {
    result = Eval(node->rest);
  } else {
    const AstNode* rest = node->rest.Get<AstNode>();
    const ScriptValue* params = &rest->first;
    const ScriptValue* body = &rest->rest;

    if (type == kFunction) {
      result = ScriptValue(Lambda(*params, *body));
    } else {
      result = ScriptValue(Macro(*params, *body));
    }
  }

  // Get the symbol to which the value will be assigned.
  const Symbol* symbol = node->first.Get<Symbol>();
  if (type == kLetPrimitive) {
    LetValue(symbol->value, result);
  } else {
    SetValue(symbol->value, result);
  }

  return result;
}

ScriptValue ScriptEnv::CallVarSpan(HashValue id, absl::Span<Var> args) {
  ScriptValue script_args;
  for (size_t i = 0; i < args.size(); ++i) {
    ScriptValue first = ScriptValue(args[args.size() - i - 1]);
    AstNode node(first, std::move(script_args));
    script_args = node;
  }
  ScriptValue callable = Symbol(id);
  return CallInternal(callable, script_args);
}

ScriptValue ScriptEnv::CallVarTable(HashValue id, const VarTable& kwargs) {
  ScriptValue callable = GetValue(id);
  const ScriptValue* params = nullptr;
  if (Lambda* lambda = callable.Get<Lambda>()) {
    params = &lambda->params;
  } else if (Macro* macro = callable.Get<Macro>()) {
    params = &macro->params;
  } else {
    Error("Expected a lambda or macro", callable);
    return ScriptValue();
  }

  std::vector<Var> args;
  while (!params->IsNil()) {
    const AstNode* node = params->Get<AstNode>();
    if (node == nullptr) {
      Error("Parameter list should be an ast node.", *params);
      return ScriptValue();
    }

    const Symbol* symbol = node->first.Get<Symbol>();
    if (symbol == nullptr) {
      Error("Parameter should be a symbol.", *params);
      return ScriptValue();
    }

    const Var* var = kwargs.TryFind(symbol->value);
    if (var == nullptr) {
      Error("No matching symbol in variant map.", callable);
      return ScriptValue();
    }
    args.emplace_back(*var);
    params = &node->rest;
  }
  return CallVarSpan(id, absl::Span<Var>(args));
}

void ScriptEnv::PushScope() { stack_.PushScope(); }

void ScriptEnv::PopScope() { stack_.PopScope(); }

}  // namespace redux
