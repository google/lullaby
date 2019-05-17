{
  // Convenience constructor for a lull.Vec2.
  vec2(x_, y_):: { x: x_, y: y_},

  // Convenience constructor for a lull.Vec3.
  vec3(x_, y_, z_):: { x: x_, y: y_, z: z_ },

  // Convenience constructor for a lull.Vec4.
  vec4(x_, y_, z_, w_):: { x: x_, y: y_, z: z_, w: w_ },

  // Convenience constructor for a lull.Rect.
  rect(x_, y_, w_, h_):: { x: x_, y: y_, w: w_, h: h_ },

  // Convenience constructor for a Color4ub.
  color(r_, g_, b_, a_):: {r: r_, g: g_, b: b_, a: a_},

  // Selects the value of the specified field if its exists in the object
  // else outputs the given default value.
  select(default_value, obj, field)::
    if std.objectHas(obj, field) then obj[field] else default_value,

  // Generates a dictionary from a field in object, if the field exists. The key
  // in the dictionary is the same as the field name in obj. This is typically
  // used to fill in arguments of defs with optional parameters, e.g.
  // {
  //   def_type: 'NameDef',
  //   def: utils.dict_from_field(args, 'name'),
  // }
  // so we don't need to specify default values (e.g. 'name' here) in jsonnet.
  dict_from_field(obj, field)::
    if std.objectHas(obj, field) then {[field]: obj[field]} else {},

  // Hash function based on lull::Hash.
  hash(str_)::
    local cast_32_bit = function(x) x & ((1 << 32) - 1);
    local bytes = std.makeArray(
            std.length(str_),
            function (i)
                local ch = std.codepoint(str_[i]);
                assert ch < 128 : "String must be ASCII";
                ch);
    local basis = cast_32_bit(2216829733);  // 0x84222325;
    local multiplier = cast_32_bit(435);    // 0x000001b3;
    local hash_helper = function(i, value)
      cast_32_bit((cast_32_bit(value ^ i)) * cast_32_bit(multiplier));
    std.foldl(hash_helper, bytes, basis),

  // Covert degrees into radians.
  deg2rad(deg)::
    0.01745329252 * deg,

  // Convert a Javascript object to a VariantMap.  Sadly, we can't distinguish
  // different numeric types from each other in Javascript, so all numbers map
  // to DataFloat, and we can't reliably distinguish objects representing vec4
  // from objects representing quat, so we map them to vec4 on the theory that
  // it's unlikely someone would construct a quaternion in Javascript.
  // This behavior can be overridden by specifying the value as a JavaScript
  // object where the single key is the variant type (one of "DataBool",
  // "DataInt", etc.) and the single value is the actual value.  So a quat could
  // be specified as { "DataQuat": { x: 0, y: 0, z: 0, w: 1 } }.
  variant_map(object_):: {
    values: [{ key: field_ } + $.variant(object_[field_])
             for field_ in std.objectFields(object_)]
  },
  variantTypes_:: ["DataBool", "DataInt", "DataFloat", "DataString",
                   "DataHashValue", "DataVec2", "DataVec3", "DataVec4",
                   "DataQuat"],
  variant(item_)::
    local type_name_ = std.type(item_);
    local explicit_type_ = (type_name_ == "object" &&
        std.length(item_) == 1 &&
        std.count($.variantTypes_, std.objectFieldsAll(item_)[0]) == 1);
    local variant_type_ = if explicit_type_ then std.objectFieldsAll(item_)[0]
      else if type_name_ == "boolean" then "DataBool"
      else if type_name_ == "number" then "DataFloat"
      else if type_name_ == "string" then "DataString"
      else if type_name_ == "object" then
      (if std.objectHas(item_, "x") then
       (if std.objectHas(item_, "y") then
        (if std.objectHas(item_, "z") then
         (if std.objectHas(item_, "w") then "DataVec4"
          else "DataVec3")
         else "DataVec2")
        else "object")
       else "object")
      else "unsupported";
  {
    value_type: variant_type_,
    value: {
      value: (if explicit_type_ then item_[variant_type_] else item_),
    }
  },
}
