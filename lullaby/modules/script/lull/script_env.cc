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

#include "lullaby/modules/script/lull/script_env.h"
#include "lullaby/modules/script/lull/functions/functions.h"
#include "lullaby/modules/script/lull/script_ast_builder.h"
#include "lullaby/modules/script/lull/script_compiler.h"
#include "lullaby/modules/script/lull/script_frame.h"
#include "lullaby/modules/script/lull/script_parser.h"

namespace lull {

static ScriptFunctionEntry* g_fn = nullptr;
ScriptFunctionEntry::ScriptFunctionEntry(ScriptFunction fn, string_view name)
    : fn(fn), name(name) {
  next = g_fn;
  g_fn = this;
}

ScriptEnv::ScriptEnv() {
  auto eval_fn = [this](ScriptFrame* frame) {
    frame->Return(Eval(frame->GetArgs()));
  };
  auto set_fn = [this](ScriptFrame* frame) {
    frame->Return(SetImpl(frame->GetArgs(), kPrimitive));
  };
  auto def_fn = [this](ScriptFrame* frame) {
    frame->Return(SetImpl(frame->GetArgs(), kFunction));
  };
  auto mac_fn = [this](ScriptFrame* frame) {
    frame->Return(SetImpl(frame->GetArgs(), kMacro));
  };
  auto ret_fn = [this](ScriptFrame* frame) {
    frame->Return(Create(DefReturn(frame->EvalNext())));
  };
  auto do_fn = [this](ScriptFrame* frame) {
    frame->Return(DoImpl(frame->Next()));
  };
  auto print_fn = [this](ScriptFrame* frame) {
    std::stringstream ss;
    while (frame->HasNext()) {
      ss << Stringify(frame->EvalNext()) << " ";
    }
    const std::string str = ss.str();
    if (print_fn_) {
      print_fn_(str);
    } else {
      LOG(INFO) << str;
    }
    frame->Return(str);
  };

  Register("=", NativeFunction{set_fn});
  Register("do", NativeFunction{do_fn});
  Register("def", NativeFunction{def_fn});
  Register("eval", NativeFunction{eval_fn});
  Register("macro", NativeFunction{mac_fn});
  Register("return", NativeFunction{ret_fn});
  Register("?", NativeFunction{print_fn});
  for (auto* fn = g_fn; fn != nullptr; fn = fn->next) {
    Register(fn->name, NativeFunction{fn->fn});
  }
}

void ScriptEnv::SetFunctionCallHandler(FunctionCall::Handler handler) {
  call_handler_ = std::move(handler);
}

void ScriptEnv::SetPrintFunction(PrintFn fn) {
  print_fn_ = std::move(fn);
}

void ScriptEnv::Register(string_view id, NativeFunction fn) {
  SetValue(Symbol(id), Create(fn));
}

void ScriptEnv::Error(const char* msg, const ScriptValue& context) {
  ScriptFrame frame(this, context);
  LOG(INFO) << "Script Error:";
  LOG(INFO) << "  Message: " << msg;
  LOG(INFO) << "  Context: " << Stringify(&frame);
}

ScriptByteCode ScriptEnv::Compile(string_view src) {
  ScriptByteCode code;
  ScriptCompiler compiler(&code);
  ParseScript(src, &compiler);
  return code;
}

ScriptValue ScriptEnv::Load(const ScriptByteCode& code) {
  ScriptCompiler compiler(const_cast<ScriptByteCode*>(&code));
  ScriptAstBuilder builder(this);
  compiler.Build(&builder);
  return Create(builder.GetRoot());
}

ScriptValue ScriptEnv::LoadOrRead(Span<uint8_t> code) {
  if (ScriptCompiler::IsByteCode(code)) {
    return Load(ScriptByteCode(code.data(), code.data() + code.size()));
  } else {
    const char* str = reinterpret_cast<const char*>(code.data());
    return Read(string_view(str, code.size()));
  }
}

ScriptValue ScriptEnv::Read(string_view src) {
  ScriptAstBuilder builder(this);
  ParseScript(src, &builder);
  return Create(builder.GetRoot());
}

ScriptValue ScriptEnv::Exec(string_view src) {
  return Eval(Read(src));
}

void ScriptEnv::SetValue(const Symbol& symbol, ScriptValue value) {
  table_.SetValue(symbol, std::move(value));
}

ScriptValue ScriptEnv::GetValue(const Symbol& symbol) const {
  return table_.GetValue(symbol);
}

ScriptValue ScriptEnv::Eval(ScriptValue script) {
  ScriptValue result;
  if (const AstNode* node = script.Get<AstNode>()) {
    const AstNode* child = node->first.Get<AstNode>();
    if (child) {
      result = CallInternal(child->first, child->rest);
    } else {
      result = Eval(node->first);
    }
  } else if (const Symbol* symbol = script.Get<Symbol>()) {
    result = Eval(GetValue(*symbol));
  } else {
    result = script;
  }
  return result;
}

ScriptValue ScriptEnv::CallInternal(ScriptValue fn, const ScriptValue& args) {
  ScriptValue result;

  if (const AstNode* node = fn.Get<AstNode>()) {
    fn = Eval(fn);
  }

  if (const Symbol* symbol = fn.Get<Symbol>()) {
    ScriptValue value = GetValue(*symbol);
    if (!value.IsNil()) {
      fn = value;
    }
  }

  // Execute the function depending on what kind of callable type it is.
  if (const NativeFunction* script = fn.Get<NativeFunction>()) {
    ScriptFrame frame(this, args);
    script->fn(&frame);
    result = frame.GetReturnValue();
  } else if (const Lambda* lambda = fn.Get<Lambda>()) {
    table_.PushScope();
    if (AssignArgs(lambda->params, args, true)) {
      result = DoImpl(lambda->body);
    }
    table_.PopScope();
  } else if (const Macro* macro = fn.Get<Macro>()) {
    if (AssignArgs(macro->params, args, false)) {
      result = DoImpl(macro->body);
    }
  } else if (const Symbol* symbol = fn.Get<Symbol>()) {
    result = InvokeFunctionCall(*symbol, args);
  } else {
    Error("Expected callable type.", fn);
  }
  return result;
}

ScriptValue ScriptEnv::InvokeFunctionCall(const Symbol& id,
                                          const ScriptValue& args) {
  ScriptValue result;
  if (call_handler_) {
    FunctionCall call(id.name);

    ScriptArgList arg_list(this, args);
    while (arg_list.HasNext()) {
      ScriptValue value = arg_list.EvalNext();
      if (!value.IsNil()) {
        call.AddArg(*value.GetVariant());
      } else {
        call.AddArg(Variant());
      }
    }

    call_handler_(&call);

    result = ScriptValue::Create(0);
    result.SetFromVariant(call.GetReturnValue());
  }
  return result;
}

bool ScriptEnv::AssignArgs(ScriptValue params, ScriptValue args, bool eval) {
  // Track the values that will be assigned to the parameter variables within
  // the scope of the function or macro call.  We need to evaluate all the
  // arguments before assigning them.
  static const int kMaxArgs = 16;
  int count = 0;
  Symbol symbols[kMaxArgs];
  ScriptValue values[kMaxArgs];

  while (!args.IsNil() && !params.IsNil()) {
    const AstNode* args_node = args.Get<AstNode>();
    if (args_node == nullptr) {
      Error("Expected a node for the arguments.", args);
      return false;
    }

    const AstNode* params_node = params.Get<AstNode>();
    if (params_node == nullptr) {
      Error("Expected a node for the parameters.", params);
      return false;
    }

    const Symbol* symbol = params_node->first.Get<Symbol>();
    if (!symbol) {
      Error("Parameter should be a symbol.", params);
      return false;
    }

    if (count >= kMaxArgs) {
      Error("Too many arguments, limit of 16.", args);
      return false;
    }

    // For lambdas/functions, the argument needs to be evaluated before being
    // assigned to the parameter. For macros, the parameter should be set to the
    // AstNode passed in as the argument.
    if (eval) {
      values[count] = Eval(args);
    } else {
      values[count] = args;
    }
    symbols[count] = *symbol;
    ++count;

    // Go to the next parameter and argument.
    args = args_node->rest;
    params = params_node->rest;
  }

  if (!args.IsNil()) {
    Error("Too many arguments.", args);
    return false;
  } else if (!params.IsNil()) {
    Error("Too few arguments.", params);
    return false;
  }

  // Assign the evaluated argument values to the parameters.
  for (int i = 0; i < count; ++i) {
    SetValue(symbols[i], values[i]);
  }

  return true;
}

ScriptValue ScriptEnv::DoImpl(const ScriptValue& body) {
  ScriptValue result;
  if (!body.Is<AstNode>()) {
    return body;
  }

  ScriptValue iter = body;
  while (auto* node = iter.Get<AstNode>()) {
    ScriptValue tmp = Eval(iter);

    if (auto* def_return = tmp.Get<DefReturn>()) {
      result = def_return->value;
      break;
    }

    result = std::move(tmp);
    iter = node->rest;
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
  if (type == kPrimitive) {
    result = Eval(node->rest);
  } else {
    ScriptValue params = node->rest.Get<AstNode>()->first;
    ScriptValue body = node->rest.Get<AstNode>()->rest;

    if (type == kFunction) {
      result = Create(Lambda(params, body));
    } else {
      result = Create(Macro(params, body));
    }
  }

  if (!result.IsNil()) {
    // Get the symbol to which the value will be assigned.
    const Symbol* symbol = node->first.Get<Symbol>();
    SetValue(*symbol, result);
  }

  return result;
}

ScriptValue ScriptEnv::CallWithArray(string_view id, Span<ScriptValue> args) {
  return CallWithArray(Symbol(id), args);
}

ScriptValue ScriptEnv::CallWithArray(const Symbol& id, Span<ScriptValue> args) {
  ScriptValue script_args;
  for (size_t i = 0; i < args.size(); ++i) {
    script_args = Create(AstNode(args[args.size() - i - 1], script_args));
  }
  ScriptValue fn = Create(Symbol(id));
  return CallInternal(fn, script_args);
}

ScriptValue ScriptEnv::CallWithMap(string_view id, const VariantMap& kwargs) {
  return CallWithMap(Symbol(id), kwargs);
}

ScriptValue ScriptEnv::CallWithMap(const Symbol& id, const VariantMap& kwargs) {
  ScriptValue callable = GetValue(id);

  ScriptValue params;
  if (Lambda* lambda = callable.Get<Lambda>()) {
    params = lambda->params;
  } else if (Macro* macro = callable.Get<Macro>()) {
    params = macro->params;
  } else {
    Error("Expected a lambda or macro", callable);
    return ScriptValue();
  }

  std::vector<ScriptValue> values;
  while (!params.IsNil()) {
    const AstNode* node = params.Get<AstNode>();
    if (node == nullptr) {
      Error("Parameter list should be an ast node.", params);
      return ScriptValue();
    }

    const Symbol* symbol = node->first.Get<Symbol>();
    if (symbol == nullptr) {
      Error("Parameter should be a symbol.", params);
      return ScriptValue();
    }

    auto iter = kwargs.find(symbol->value);
    if (iter == kwargs.end()) {
      Error("No matching symbol in variant map.", callable);
      return ScriptValue();
    }
    values.emplace_back(Create(iter->second));

    params = node->rest;
  }

  ScriptValue script_args;
  for (auto iter = values.rbegin(); iter != values.rend(); ++iter) {
    script_args = Create(AstNode(std::move(*iter), script_args));
  }

  ScriptValue fn = Create(Symbol(id));
  return CallInternal(fn, script_args);
}

void ScriptEnv::PushScope() { table_.PushScope(); }

void ScriptEnv::PopScope() { table_.PopScope(); }

}  // namespace lull
