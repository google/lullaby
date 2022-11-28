The datafile library provides support for processing a custom text-based file
format that can be used for configuration of modules and definition of entity
blueprints.

The datafile format is similar to JSON; in fact, it is a superset of strict
JSON. It also supports the two common "non-strict" features of allowing unquoted
keys and trailing commas. Semi-colons (;) can be used for single-line comments
as well.

The main differentiator between redux datafiles and other JSON-like datafiles is
that it supports expression types as well. An expression is a value enclosed in
parenthesis and is evaluated by the redux scripting language [1] at runtime.

Expressions allow us to express values in more meaningful ways. Some common
examples:

; angles are normally stored as radians angle: (degrees 20),

; rotations are normally stored as quaternions rotation: (euler-angles 0, 90, 0)
rotation: (around-y-axis 90),

; colors are usually stored as 4 floating point rgba values. color: (color.red),
color: (hex-color '#ff0000'),

Domain-specific functions can be implemented as well for more expressive data.

[1] Learn more about redux scripting here: engines/script/redux.
