# LullScript



## Introduction

LullScript is a light-weight scripting engine built into the Lullaby runtime.
It provides a convenient data-driven way of interacting with some of the
core concepts underlying the Lullaby runtime.

This document provides an overview of the syntax and features of LullScript.
It assumes the reader is familiar with programming in C++. It is not a guide for
learning to program, but rather a guide about the specifics of how to scripts
using LullScript.

LullScript is essentially a very simple parser and interpreter. It is intended
for simple, short scripts that interact directly with the Lullaby runtime. For
advanced use-cases, it is worth considering using the Lullaby Javascript and/or
Lua bindings.


## Basics

LullScript is a simple programming language loosely based on [LISP]
(https://en.wikipedia.org/wiki/Lisp_(programming_language)). It is structured
like LISP, but uses more C++ and Python like types and keywords.

The key distinguishing feature of the language is that it uses a fully
parenthesized prefix notation. Specifically, each statement/expression is
wrapped in parenthesis and structured like so:

```
(function arg arg arg)
```

Note that each token is separate by whitespace (there is no need for commas)
and that there is no semi-colon marking the end of the statement. The
corresponding structure in C-like languages would be:
```function(arg, arg, arg);```.)

All statements begin with a function followed by its arguments. This extends to
all operators which results in a prefix notation, for example:

```
(+ 1 2)
(* 3 4)
(< 3 5)
(= x 5)
(+ 1 (* 2 3))
```

This consistent and uniform syntax is the primary reason for using it as the
basis for LullScript as it allowed for a simple runtime implementation.

## Comments

Comments are marked by a hash (#) symbol and continue until the end of the line.
They can appear anywhere inside an expression.

```
(+ 1 1)  # returns the sum of 1 and 1
(? 'hello')  # prints "hello" to the console
(+  # add
 1  # one
 1  # and one
)
```

## Functions

Functions can be defined using the ```def``` keyword:

```
(def <NAME> (<ARGS>...) <STATEMENTS>...)
```

For example:

```
(def pi () 3.14159)
(def add (x y) (+ x y))
(def foo (x)
  (? 'hello')
  (? 'world')
  3.14159
)
```

The function's return value is the result of evaluating the last statement. You
can return a value from the function earlier by using the ```return``` keyword.
For example:

```
(def foo (x)
  (? 'hello')
  (return (* 2.0 3.14159))
  (? 'world')
)
```

## Do

The ```do``` function will evaluate a sequence of statements, returning the
result of the last statement. For example:

```
(do
  (foo 1)
  (bar 2)
  (baz 3)
)
```

## If and Cond

The ```if``` function allows you selectively choose one of two statements to
execute based on a condition value.

```
(if CONDITION
  TRUE_STATEMENT
  FALSE_STATEMENT
)
```

It is important to note that both branches are single statements. To perform
multiple statements, ```if``` statements are generally used with ```do```
statements. For example:

```
(if cond
  (do
    ...
    ...
  )
  (do
    ...
    ...
  )
)
```

For multiple conditions (similar to if-elif-else statements):

```
(if (< x 90)
  (do
    (? 'alpha quadrant)
    (return 0)
  )
(if (< x 180)
  (do
    (? 'beta quadrant)
    (return 1)
  )
(if (< x 270)
  (do
    (? 'gamma quadrant)
    (return 2)
  )
# else
  (do
    (? 'delta quadrant)
    (return 3)
  )
)))
```

This is rather clumsy. A more useful construct is the ```cond``` function which
will execute a set of statements based on the result of a condition.

```
(cond
  (CONDITION STATEMENTS..)
  (CONDITION STATEMENTS..)
  ...
)
```

Replicating the above example:

```
(cond
  ((< x 90)  (? 'alpha quadrant') 0)
  ((< x 180) (? 'beta quadrant')  1)
  ((< x 270) (? 'gamma quadrant') 2)
  (true      (? 'delta quadrant;) 3)
)
```

## Primitive Types

LullScript types are primarily wrappers around C++ types most commonly used by
Lullaby.

Integers are represented by numbers (with an optional leading ```-``` symbol
indicating a negative value). They correspond directly with the ```int``` (or
```int32_t``` types in C++. Examples:

```
1
123
-456
00123
```

Unsigned integers (corresponding to ```uint32_t``` types) can be specified using
a trailing ```u``` on integer types. Examples:

```
1u
123u
00123u
```

64-bit integers (corresponding to ```int64_t``` and ```uint64_t```) can be
specified using the ```l``` and ```ul``` suffixes. Examples:

```
1l
123l
-456l
00123ul
```

32-bit and 64-bit floating point numbers (corresponding to ```float``` and
```double``` types) are similar to integers, but have a decimal point. 32-bit
values require the ```f``` suffix, but 64-bit values do not. Examples:

```
1.2
-3.4
5.6f
-7.8f
```

All the various numeric types (eg. ```int8_t```, ```uint16_t```, etc.) can be
explicitly created using functions:

```
(int8 1)
(uint8 2)
(int16 3)
(uint16 4)
(int32 5)
(uint32 6)
(int64 7)
(uint64 8)
(float 9)
(double 10)
```

The keywords ```true``` and ```false``` are used for the corresponding boolean
```bool```-type constants.

Strings are any combination of characters enclosed in either single ```'```
or double ```"``` quotes. Internally, they are stored as ```std::string```
objects. For example:

```
'hello world'
"how are you today"
"what's up"
'she said "hi"'
```

More information about [Strings](#strings) in LullScript is available below.

Finally, Lullaby hash values can be generated directly at parse time by
prefixing a colon ```:``` to a word, or by explicitly calling the ```hash```
function. (The latter is useful when hashing strings with spaces.)  For
example:

```
:hello
(hash "hello")
```

All other tokens parsed by the parser are considered "symbols" to LullScript
itself. Symbols are evaluated differently depending on the context. (For
example, a symbol immediately after an opening parenthesis is considered a
function, whereas a symbol anywhere else is generally considered a variable.)


## Additional Types

Lullaby supports the most common mathfu types by using functions to create them.
For example:

```
(vec2i 1 2)
(vec2 1.0f 2.0f)
(vec3i 1 2 3)
(vec3 1.0f 2.0f 3.0f)
(vec4i 1 2 3 4)
(vec3 1.0f 2.0f 3.0f 4.0f)
(quat 0.0f 1.0f 0.0f 0.0f)
```

Lullaby's ```clock::Duration``` type is also supported using the ```seconds```
function. For example:

```
(seconds 1.0f)
```

LullScript also allows users to get the ```TypeId``` of a variable or value
using the ```typeof``` function. For example:

```
(typeof 123)
(typeof x)
```

## Operators

```+```
```-```
```*```
```/```
```==```
```!=```
```<```
```>```
```<=```
```>=```
```and```
```or```
```not```
```is```

## Strings

LullScript purposefully does not have a string manipulation library. The primary
use of strings in LullScript is for debugging purposes. Generating strings for
other reasons is best left to the runtime itself so it can perform appropriate
functions like localization.

## Arrays

You can create arrays using ```[``` and ```]```. This is primarily just
syntactic sugar around the ```make-array``` function. This function takes a list
of element values. For example:

```
(make-array 1 2 3)
[1 2 3]
````

You can manipulate arrays using the following functions:

```
(array-size <ARRAY>)
(array-empty <ARRAY>)
(array-push <ARRAY> <VALUE>)
(array-pop <ARRAY>)
(array-insert <ARRAY> <INDEX> <VALUE>)
(array-erase <ARRAY> <INDEX>)
(array-at <ARRAY> <INDEX>)
```

## Maps

You can create maps using ```{``` and ```}```. This is primarily just
syntactic sugar around the ```make-map``` function. This function takes a
sequence of tuples for the map elements, where each tuple is a key (either an
integer or hash value) and a value. For example:

```
{(:a 1) (:b 2) (:c 3)}
(make-map (:a 1) (:b 2) (:c 3))
```

You can manipulate maps using the following functions:

```
(map-size <MAP>)
(map-empty <MAP>)
(map-insert <MAP> <KEY> <VALUE>)
(map-erase <MAP> <KEY>)
(map-get <MAP> <KEY>)
```

## Events

## Macros

## Variables and Scoping

## ScriptEnv

## Native C++ Bindings

## Compile
