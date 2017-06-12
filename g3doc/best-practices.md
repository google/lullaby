# Best Practices


### General

**Google Style**: Code should follow google conventions and style. This should
also be extended to non-C++ code (eg. glsl shaders, fbs schemas, etc.) as best
as possible.


**Config**: Use the lull::Config class to enable/disable functionality. This is
useful for new features that are in development.

### Systems


**Small-scope**: Systems should be responsible for a single aspect of an
Entity's behaviour and should follow the [single responsibility
principle](https://en.wikipedia.org/wiki/Single_responsibility_principle). The
purpose of a System should be describable using one or two sentences.

**Data-driven**: System behaviour should be driven by JSON data. Most const
values defined in code itself should generally be exposed as default values in
the schema file.


**Register Dependencies**: Register dependencies on other systems using
RegisterDependency() in your constructor.

**Register Functions**: The public functions of a System should be registered
with the Function Registry. This will automatically expose the System to the
Lullaby Java API as well as any scripting engine used by an App.

**Write Docs**: Add a README.md for your System. Add your System to the
list-of-systems.md doc in g3doc.

**Write Tests**: Ensure your System is well tested.


### Code Smells

**ComponentPool vs std::unordered_map**: ComponentPools are great for when your
System is planning to iterate over the list of all Components. However, in many
cases, an unordered_map<Entity, SystemComponent> is strictly a better choice.
Pick the right structure for your use-case.
