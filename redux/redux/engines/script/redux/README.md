A simple, redux-centric LISP interpreter.

Redux Scripting can be used to create simple programs that are evaluated and
executed at runtime. A single statement is structured like so:

```
(function arg0 arg1 arg2 ...)
```

For example, the statement `(+ 1 1)` will evaluate to the value `2`.

Commas are treated as whitespace, so you can do this:

```
(function arg0, arg1, arg2, ...)
```

Single line comments start with a semicolon:

```
; this is a comment
(+ 1 1)  ; this is also a comment
```

The `?` function is a short cut for `LOG(INFO)` and can take a variable number
of arguments.

```
(? 'hello world')
```


Variables can be declared using the `var` keyword:

```
(var one 1)
(var two 2.0)
(var hello 'hello')
```

Redux scripting uses the C++ type system behind the scenes, so you get access
to the same standard types.

* `true` and `false` literals are treated as booleans
* whole numbers (e.g. 1, 5, 15, 39) are treated as int32_t
* decimal numbers (e.g. 1.2, 3.456, 0.87) are treated as doubles
* numbers suffixed with `f` (e.g. 1f, 2.3f) are treated as floats
* numbers suffixed with `u` (e.g. 2u) are treated as uint32_t
* numbers suffixed with `l` (e.g. 3l) are treated as int64_t
* numbers suffixed with `ul` (e.g. 4ul) are treated as uint64_t
* strings can be enclosed in single `'` or double quotes `"`
* the `null` literal is treated as a special "null" type

Values can be explicitly cast to other types using the following functions:
`int8`, `int16`, `int32`, `int64`, `uint8`, `uint16`, `uint32`, `uint64`,
`float`, and `double`.

For example, `(uint8 214)` will return a `uin8_t` with the value `214`.


A set of expressions can be evaluated in sequence using the `do` command.

```
(do
  (+ 1 1)
  (+ 2 2)
)
```

The result of the last expression within the `do`-block will be the result of
the block itself. You can use a `return` statement to stop execution of a
block.

```
(do
  (+ 1 1)
  (return (+ 2 2))
  (+ 3 3)          ; will not be evaluated
)
```

You can define functions using `def`:

```
(def add (x y) (
  (+ x y)
))
```

The body of the function is treated like a `do`-block and can contain multiple
statements.

Similarly, you can define a macro using `macro`:

```
(def maco (x y) (
  (+ x y)
))
```

The difference between a function and a macro is that the arguments to a
function are evaluated before they are passed to the function, whereas the
arguments to a macro are passed to the macro in an unevaluated form.

For example, consider the following:

```
(def my_function (x) (+ x x))
(macro my_macro (x) (+ x x))
```

Calling

```
(var my_var 1)
(my_function (= my_var (+ my_var 1)))
```

will return a value of `4`. This is because the value of `my_var` will be
assigned to `2` before the function is called.

Calling

```
(var my_var 1)
(my_macro (= my_var (+ my_var 1)))
```

In this case, the result is `5`.  That is because the entire expression
`(= my_var (+ my_var 1))` is passed into the macro which it then evaluated
twice before being added. In other words, the macro is doing:

```
(+
  (= my_var (+ my_var 1))  ; sets my_var to 2
  (= my_var (+ my_var 1))  ; sets my_var to 3
)
```

Redux scripting provides several other built-in functions as well, but they
are described in the individual header files under `redux/functions`. These
include (but are not limited to):

* Conditional/branching using `if` and `cond`
* Boolean algebra operators `and`, `or`, and `not`
* Comparison operators `==`, `!=`, `<`, `>`, `<=`, and `>=`
* Mathematical operators `+`, `-`, `*`, `/`, and `%`
* Vector math types `vec2`, `vec2i`, `vec3`, `vec3i`, `vec4`, `vec4i`, and
  `quat`
* Time types which can be created using `seconds` and `milliseconds`
* Entity IDs using `entity`
