# Base Technologies


This page describes several key libraries that are used in Lullaby. Additional
information about these libraries is available in their respective header files.

[TOC]

## TypeId


Lullaby's TypeId framework is a light-weight mechanism for getting [runtime type
information (RTTI)](https://en.wikipedia.org/wiki/Run-time_type_information).
Classes have to explicitly opt-in to the framework by using the
LULLABY_SETUP_TYPE macro. For example:

```c++
LULLABY_SETUP_TYPEID(OuterNamespace::InnerNamespace::MyClass);
```

This must be added after the class declaration (usually in a header file) and at
global scope. Once registered, the TypeId for a class can be requested by
calling GetTypeId<T>(). For example:

```c++
lull::TypeId type_id = lull::GetTypeId<MyClass>();
```

You can also get a string-representation of a registered type by calling
GetTypeName<T>(). For example:

```c++
std::cout << lull::GetTypeName<MyClass>();
```

## Registry


The Registry is used as a light-weight [dependency
injection](https://en.wikipedia.org/wiki/Dependency_injection) framework. All
"singleton-like" objects should be created and added to the registry during an
app's initialization routine. For example:

```
registry.Create<AssetLoader>();
registry.Create<Dispatcher>();
registry.Create<TransformSystem>(&registry);
registry.Create<RenderSystem>(&registry);
```

These objects can then be accessed at any point during the registry's lifetime.
For example:

```
AssetLoader* asset_loader = registry.Get<AssetLoader>();
TransformSystem* transform_system = registry.Get<TransformSystem>();
```

Because the Registry owns all these high-level runtime objects, it can be
considered the primary representation of Lullaby in an application's runtime.
All [Systems](ecs.md#system) and most other key libraries depend on the Registry
to interact with each other.

## Dispatcher


The Dispatcher is Lullaby's [message
passing](https://en.wikipedia.org/wiki/Message_passing) framework and is used to
enable [event-driven
programming](https://en.wikipedia.org/wiki/Event-driven_programming).

Events are defined as C++ structs that have been registered with the
[TypeId](#typeid) framework. They should also provide a Serialize function that
is used for handling complex data types. For example:

```c++
struct SomeEvent {
  int x;
  float y;
  std::string str;
  std::vector<std::string>> names;

  template <typename Archive>
  void Serialize(Archive archive) {
    archive(&x, Hash("x"));
    archive(&y, Hash("y"));
    archive(&str, Hash("str"));
    archive(&names, Hash("names"));
  }
};
LULLABY_SETUP_TYPEID(SomeEvent);
```

You can then connect an EventHandler with a Dispatcher to process specific
events. For example:

```c++
auto connection = dispatcher.Connect([](const SomeEvent& event) {
  std::cout << event.x << std::endl;
  std::cout << event.y << std::endl;
  std::cout << event.str << std::endl;
  for (auto& name : event.names) {
    std::cout << name << std::endl;
  }
});
```

You can then send an event instance which will, in turn, invoke all the handlers
associated with that event. For example:

```c++
SomeEvent event;
event.x = 1;
event.y = 2.f;
event.str = "foo";
event.names = {"hello", "world"};
dispatcher.Send(event);
```

There's a lot more functionality available in the Dispatcher framework, such as
the ability to queue messages (to support multithreaded communication),
integration with scripting languages, and the ability to send/receive events
from JSON data files.

## Config


TO-DO: Add documentation.

## InputManager


TO-DO: Add documentation.

## AssetLoader


TO-DO: Add documentation.
