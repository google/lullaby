{
  // Convenience constructor for a lull.Color.
  rgba(r_, g_, b_, a_):: { r: r_, g: g_, b: b_, a: a_ },

  // Converts a hexadecimal string to a decimal integer.
  hex2dec(hex)::
    if std.type(hex) != "string" then
      error "hex2dec param must be a string, got " + std.type(hex)
    else
      local table = {
        "0": 0, "1": 1, "2": 2, "3": 3, "4": 4, "5": 5, "6": 6, "7": 7, "8": 8,
        "9": 9, "A": 10, "a": 10, "B": 11, "b": 11, "C": 12, "c": 12, "D": 13,
        "d": 13, "E": 14, "e": 14, "F": 15, "f": 15,
      };
      local aux(str) =
        if std.length(str) == 0 then
          0
        else
          local i = std.length(str) - 1;
          table[str[i]] + 16 * aux(std.substr(str, 0, i));
      aux(hex),

  // Helper for hex2rgb and hex2rgba.
  hex2rgba_helper(hex, exclude_alpha)::
    if hex[0] != "#" then
        error "hex2rgba_helper param must start with #"
    else
      local aux(i) = self.hex2dec(std.substr(hex, 2 * i + 1, 2)) / 255.0;
      if exclude_alpha && (std.length(hex) == 7 || std.length(hex) == 9) then
        std.makeArray(3, aux)
      else if std.length(hex) == 7 then
        std.makeArray(3, aux) + [1.0]
      else if std.length(hex) == 9 then
        std.makeArray(4, aux)
      else
        error "hex2rgba_helper param must be string of the form " +
            "#RRGGBB/#RGGBBAA",

  // Converts a hex color string to an RGBA float array.
  hex2rgba(hex)::
    self.hex2rgba_helper(hex, false),

  // Converts a hex color string to an RGB float array.
  hex2rgb(hex)::
    self.hex2rgba_helper(hex, true),

  // Converts RGBA on 255 scale to 1.0 scale float array.
  byte_rgba2float_rgba(byte)::
    if std.type(byte) != "object" then
      error "byte2float param must be a rgba channel: value object"
    else
       {[channel]:
       if byte[channel] > 0
       then "%0.3f"%(byte[channel]/255)
       else 0
       for channel in ["r", "g", "b", "a"]},

  // Converts a 4-element array into a vec4 object.
  array2vec4(array)::
    if std.type(array) != "array" || std.length(array) != 4 then
      error "array2vec4 param must be a 4-element array"
    else
      { x: array[0], y: array[1], z: array[2], w: array[3] },

  // Colors.
  kRed: {
    r: 1.0,
    g: 0.0,
    b: 0.0,
    a: 1.0
  },
  kGreen: {
    r: 0.0,
    g: 1.0,
    b: 0.0,
    a: 1.0
  },
  kBlue: {
    r: 0.0,
    g: 0.0,
    b: 1.0,
    a: 1.0
  },
  kWhite: {
    r: 1.0,
    g: 1.0,
    b: 1.0,
    a: 1.0
  },
  kRedHex: "#ff0000",
  kGreenHex: "#00ff00",
  kBlueHex: "##0000ff",
  kWhiteHex: "#ffffff"
}
