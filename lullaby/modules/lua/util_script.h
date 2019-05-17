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

#ifndef LULLABY_MODULES_LUA_UTIL_SCRIPT_H_
#define LULLABY_MODULES_LUA_UTIL_SCRIPT_H_

namespace lull {
namespace script {
namespace lua {
namespace detail {

constexpr char kUtilScript[] = R"(

-- Metatable for serializable objects. These objects are stored as tables
-- mapping lull::Hash keys to the corresponding values. The metatable allows
-- setting and getting by the string keys as well.
serializable_metatable = {
  __index = function(t, k)
    if type(k) == "string" then
      return rawget(t, hash(k))
    else
      return rawget(t, k)
    end
  end,
  __newindex = function(t, k, v)
    if type(k) == "string" then
      rawset(t, hash(k), v)
    else
      rawset(t, k, v)
    end
  end,
}

-- Returns a new table that copies the input table, but replaces all string keys
-- with their corresponding lull::Hash keys.
function hash_table_keys(t)
  s = {}
  for k, v in pairs(t) do
    if type(k) == "string" then
      rawset(s, hash(k), v)
    else
      rawset(s, k, v)
    end
  end
  return s
end

-- Metatable for vec2 objects. Duplicates mathfu functionality.
vec2_metatable = {
  __unm = function(a)
    return vec2(-a.x, -a.y)
  end,
  __add = function(a, b)
    if type(a) == "number" then
      return vec2(a + b.x, a + b.y)
    elseif type(b) == "number" then
      return vec2(a.x + b, a.y + b)
    elseif getmetatable(a) == vec2_metatable and
        getmetatable(b) == vec2_metatable then
      return vec2(a.x + b.x, a.y + b.y)
    else
      error('Unsupported types in vec2 addition')
    end
  end,
  __sub = function(a, b)
    if type(a) == "number" then
      return vec2(a - b.x, a - b.y)
    elseif type(b) == "number" then
      return vec2(a.x - b, a.y - b)
    elseif getmetatable(a) == vec2_metatable and
        getmetatable(b) == vec2_metatable then
      return vec2(a.x - b.x, a.y - b.y)
    else
      error('Unsupported types in vec2 subtraction')
    end
  end,
  __mul = function(a, b)
    if type(a) == "number" then
      return vec2(a * b.x, a * b.y)
    elseif type(b) == "number" then
      return vec2(a.x * b, a.y * b)
    elseif getmetatable(a) == vec2_metatable and
        getmetatable(b) == vec2_metatable then
      return vec2(a.x * b.x, a.y * b.y)
    else
      error('Unsupported types in vec2 multiplication')
    end
  end,
  __div = function(a, b)
    if type(a) == "number" then
      return vec2(a / b.x, a / b.y)
    elseif type(b) == "number" then
      return vec2(a.x / b, a.y / b)
    elseif getmetatable(a) == vec2_metatable and
        getmetatable(b) == vec2_metatable then
      return vec2(a.x / b.x, a.y / b.y)
    else
      error('Unsupported types in vec2 division')
    end
  end,
  __eq = function(a, b)
    if getmetatable(a) == vec2_metatable and
        getmetatable(b) == vec2_metatable then
      return a.x == b.x and a.y == b.y
    end
    return false
  end,
}

-- Metatable for vec3 objects. Duplicates mathfu functionality.
vec3_metatable = {
  __unm = function(a)
    return vec3(-a.x, -a.y, -a.z)
  end,
  __add = function(a, b)
    if type(a) == "number" then
      return vec3(a + b.x, a + b.y, a + b.z)
    elseif type(b) == "number" then
      return vec3(a.x + b, a.y + b, a.z + b)
    elseif getmetatable(a) == vec3_metatable and
        getmetatable(b) == vec3_metatable then
      return vec3(a.x + b.x, a.y + b.y, a.z + b.z)
    else
      error('Unsupported types in vec3 addition')
    end
  end,
  __sub = function(a, b)
    if type(a) == "number" then
      return vec3(a - b.x, a - b.y, a - b.z)
    elseif type(b) == "number" then
      return vec3(a.x - b, a.y - b, a.z - b)
    elseif getmetatable(a) == vec3_metatable and
        getmetatable(b) == vec3_metatable then
      return vec3(a.x - b.x, a.y - b.y, a.z - b.z)
    else
      error('Unsupported types in vec3 subtraction')
    end
  end,
  __mul = function(a, b)
    if type(a) == "number" then
      return vec3(a * b.x, a * b.y, a * b.z)
    elseif type(b) == "number" then
      return vec3(a.x * b, a.y * b, a.z * b)
    elseif getmetatable(a) == vec3_metatable and
        getmetatable(b) == vec3_metatable then
      return vec3(a.x * b.x, a.y * b.y, a.z * b.z)
    else
      error('Unsupported types in vec3 multiplication')
    end
  end,
  __div = function(a, b)
    if type(a) == "number" then
      return vec3(a / b.x, a / b.y, a / b.z)
    elseif type(b) == "number" then
      return vec3(a.x / b, a.y / b, a.z / b)
    elseif getmetatable(a) == vec3_metatable and
        getmetatable(b) == vec3_metatable then
      return vec3(a.x / b.x, a.y / b.y, a.z / b.z)
    else
      error('Unsupported types in vec3 division')
    end
  end,
  __eq = function(a, b)
    if getmetatable(a) == vec3_metatable and
        getmetatable(b) == vec3_metatable then
      return a.x == b.x and a.y == b.y and a.z == b.z
    end
    return false
  end,
}

-- Metatable for vec4 objects. Duplicates mathfu functionality.
vec4_metatable = {
  __unm = function(a)
    return vec4(-a.x, -a.y, -a.z, -a.w)
  end,
  __add = function(a, b)
    if type(a) == "number" then
      return vec4(a + b.x, a + b.y, a + b.z, a + b.w)
    elseif type(b) == "number" then
      return vec4(a.x + b, a.y + b, a.z + b, a.w + b)
    elseif getmetatable(a) == vec4_metatable and
        getmetatable(b) == vec4_metatable then
      return vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w)
    else
      error('Unsupported types in vec4 addition')
    end
  end,
  __sub = function(a, b)
    if type(a) == "number" then
      return vec4(a - b.x, a - b.y, a - b.z, a - b.w)
    elseif type(b) == "number" then
      return vec4(a.x - b, a.y - b, a.z - b, a.w - b)
    elseif getmetatable(a) == vec4_metatable and
        getmetatable(b) == vec4_metatable then
      return vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w)
    else
      error('Unsupported types in vec4 subtraction')
    end
  end,
  __mul = function(a, b)
    if type(a) == "number" then
      return vec4(a * b.x, a * b.y, a * b.z, a * b.w)
    elseif type(b) == "number" then
      return vec4(a.x * b, a.y * b, a.z * b, a.w * b)
    elseif getmetatable(a) == vec4_metatable and
        getmetatable(b) == vec4_metatable then
      return vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w)
    else
      error('Unsupported types in vec4 multiplication')
    end
  end,
  __div = function(a, b)
    if type(a) == "number" then
      return vec4(a / b.x, a / b.y, a / b.z, a / b.w)
    elseif type(b) == "number" then
      return vec4(a.x / b, a.y / b, a.z / b, a.w / b)
    elseif getmetatable(a) == vec4_metatable and
        getmetatable(b) == vec4_metatable then
      return vec4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w)
    else
      error('Unsupported types in vec4 division')
    end
  end,
  __eq = function(a, b)
    if getmetatable(a) == vec4_metatable and
        getmetatable(b) == vec4_metatable then
      return a.x == b.x and a.y == b.y and a.z == b.z and a.w == b.w
    end
    return false
  end,
}

-- Metatable for quat objects. Duplicates mathfu functionality. Unlike the vec
-- operations, the implementations are left in native code, because they're a
-- bit more complicated than the trivial vec operations.
quat_metatable = {
  __mul = function(a, b)
    if type(a) == "number" then
      return detail_MultiplyQuatByScalar(b, a)
    elseif type(b) == "number" then
      return detail_MultiplyQuatByScalar(a, b)
    elseif getmetatable(a) == quat_metatable and
        getmetatable(b) == quat_metatable then
      return detail_MultiplyQuatByQuat(a, b)
    elseif getmetatable(a) == quat_metatable and
        getmetatable(b) == vec3_metatable then
      return detail_MultiplyQuatByVec3(a, b)
    else
      error('Unsupported types in quat multiplication')
    end
  end,
}

-- Metatable for native callback objects. Deletes the std::function when the
-- object is being finalized.
callback_metatable = {
  __gc = function(o)
    detail_DeleteNativeFunction(o)
  end,
}

)";

}  // namespace detail
}  // namespace lua
}  // namespace script
}  // namespace lull

#endif  // LULLABY_MODULES_LUA_UTIL_SCRIPT_H_
