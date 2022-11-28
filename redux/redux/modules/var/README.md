The Var library provides a flexible dynamically-typed variant object type.

Vars are similar to `std::any`. They can be used to store arbitary value types
like ints, floats, vec3s, and strings. However, unlike `std::any`, only types
that have redux TypeIds can be stored in a Var. Furthermore, Vars implement
small object optimization to handle our common use-cases (e.g. math types like
mat4).

Two additional, very useful types are provided in this library: VarArray and
VarTable. VarArray is basically an `std::vector<Var>` and VarTable is
effectively an `absl::flat_hash_map<HashValue, Var>`.

Things get interesting when you store VarTables in Vars; you basically end up
with a powerful variant type similar to JSON objects, Python objects or Lua
tables. Vars provide overloads for `operator[]` that makes it easy to navigate
such tree-like structures.

Finally, Vars also support "smarter" casting when you use the FromVar and ToVar
functions. Specifically, these functions will handle things like setting values
from `std::optional` types. FromVar will even automatically handle some common
conversions (like reading a `float` value when the Var itself is storing an
`int`.)
