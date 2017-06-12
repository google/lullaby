# Integration Guide


[TOC]

## Introduction

This document provides a step-by-step guide for integrating Lullaby with your
project. It assumes you have read the [Introduction](introduction.md) and are
familiar with the basic concepts and technologies used by Lullaby.

Before we begin, it's important to understand a bit of Lullaby's design goals.

The Lullaby Engine is designed to simulate and render VR/AR objects on mobile
hardware, for a variety of different applications. Many of these apps were
already in development, with significant functionality already available. Each
app had their own use-case for what they needed out of Lullaby, varying from
overlaying 3D graphics on top of camera images, creating a VR UI for existing
content, or building a fully interactive 3D world.

Lullaby does not provide a platform-indirection layer. The responsibility of
creating the necessary windows and graphics context and setting up the main
event loop is left to the users of the library. Similarly, Lullaby does not
provide I/O abstractions, networking libraries, power management, etc. The focus
of Lullaby is purely on the simulation of the VR/AR "world".

In order to provide the level of flexibility and performance required, Lullaby
is organized as a collection of libraries. It is not a monolithic framework that
does "everything" in a single pass. Instead, the "main loop" is always in
control of the app developer; the responsibility of integrating and updating the
various Lullaby libraries is entirely in your control. This allows different
apps to optimize for their own use-cases by selectively integrating different
Lullaby libraries.

A downside to this level of control is that there isn't a one-step solution for
integrating Lullaby. Instead, quite a bit of thought and consideration is needed
to determine how to integrate the different parts of Lullaby into your app.

This document attempts to provide information that can help you make those
decisions. So let's get started.

## Basic Setup

### Create the Registry

The [Registry](base-tech.md#registry) owns all other high-level Lullaby objects
and allows them to access each other. In some ways, it can be thought of as a
proxy to Lullaby itself. When the [Registry](base-tech.md#registry) is
destroyed, it will destroy all objects that it owns.

You can create the [Registry](base-tech.md#registry) quite simply:

```c++
  lull::Registry registry;
```

### Create the Entity Factory

The [Entity Factory](ecs.md#entity-factory) is responsible for the creation of
[Entities](ecs.md#entity) and, as such, is a critical component of any Lullaby
integration.

```c++
  registry.Create<lull::EntityFactory>(&registry);
```

### Create the Dispatcher

The [Dispatcher](base-tech.md#dispatcher) enables Lullaby
[Systems](ecs.md#system) to communicate with each other. While the
[Registry](ecs.md#registry) allows [Systems](ecs.md#system) to directly access
and call functions on each other, the [Dispatcher](base-tech.md#dispatcher) can
be used to reduce
[coupling](https://en.wikipedia.org/wiki/Coupling_\(computer_programming\)) and
allow for greater flexibility in design. (For example, a [System](ecs.md#system)
can send a "ClickEvent" without having to know what other
[Systems](ecs.md#system) need to be notified.)

```c++
  registry.Create<lull::Dispatcher>();
```

### Create the Asset Loader

The [Asset Loader](base-tech.md#asset-loader) is used to load all assets (eg.
textures, shaders, audio files, [Blueprints](ecs.md#blueprint), etc.) in the
runtime. It supports both synchronous and asynchronous loading.

The [Asset Loader](base-tech.md#asset-loader) is optional if you do not require
any assets (though that seems unlikely as even the most basic rendering requires
loading a shader at the very least.)

```c++
  registry.Create<lull::AssetLoader>();
```

### Create the Input Manager

The [Input Manager](base-tech.md#input-manager) provides Lullaby libraries a way
to query input device (eg. head-mounted display, 3dof controller, etc.) state
with having to be aware of the actual specifics of the underlying hardware APIs.

The [Input Manager](base-tech.md#input-manager) is optional if you are not going
to use any Systems that process input, but for any type of interactive
application, you will need it.

```c++
  registry.Create<lull::InputManager>();
```

### Initialize the Entity Factory

Lastly, you must initialize the [Entity Factory](ecs.md#entity-factory). More
information about the purpose of this call, and why it is separate from the
[Entity Factory](ecs.md#entity-factory) constructor, is provided in the sections
further below.

```c++
  entity_factory->Initialize();
```

Remember that you can get a pointer to an object when you call
`Registry::Create()`.

```c++
  auto entity_factory = registry.Create<lull::EntityFactory>(&registry);
```

## Advanced Setup

### Queued Dispatcher

The [Dispatcher](base-tech.md#dispatcher) by default is an "immediate"
[Dispatcher](base-tech.md#dispatcher). When you send an Event to an "immediate"
[Dispatcher](base-tech.md#dispatcher), that Event is processed by the associated
event handlers during the call to `Dispatcher::Send`.

However, it is often desirable for the [Dispatcher](base-tech.md#dispatcher)
stored in the [Registry](base-tech.md#registry) to be a "queued"
[Dispatcher](base-tech.md#dispatcher) as this provides explicit control of when
the Events are actually processed. To create a "queued"
[Dispatcher](base-tech.md#dispatcher), instead of an "immediate"
[Dispatcher](base-tech.md#dispatcher), just do the following:

```c++
  lull::QueuedDispatcher* queued_dispatcher = new QueuedDispatcher();
  registry.Register(std::unique_ptr<Dispatcher>(queued_dispatcher));
```

You can then explicitly process the queued Events by calling:

```c++
  queued_dispatcher->Dispatch();
```

### Load Function

You can provide your own function that performs the actual loading done by the
[Asset Loader](base-tech.md#asset-loader).

### Input Devices

## Adding Systems

The majority of Lullaby's functionality is provided through its
[Systems](ecs.md#system). While Lullaby provides a variety of
[Systems](ecs.md#system), not all of them are needed for a given application.
You are free to choose which [Systems](ecs.md#system) you want to use.
Furthermore, you will probably need to write some [Systems](ecs.md#system) of
your own. Lullaby provides common, low-level [Systems](ecs.md#system), but the
high-level "business logic" [Systems](ecs.md#system) are provided by you.

Once you've decided that you need the functionality provided by a particular
[Systems](ecs.md#system), you can create it via the [Entity
Factory](ecs.md#entity-factory) like so:

```c++
  entity_factory->Create<lull::TransformSystem>(&registry);
```

Internally, the [Entity Factory] will instantiate the [System](ecs.md#system)
using the `Registry::Create()` function. It will also store a pointer to the
created [System](ecs.md#system) for its own use.

You must create all [Systems](ecs.md#system) before calling
`EntityFactory::Initialize()`.

Finally, keep in mind that different [Systems](ecs.md#system) have specific
requirements as to when and how they are to be used. For example, some may need
specific configuration, and some may need to be "updated" once per frame. Please
ensure you review and understand the API for all your [Systems](ecs.md#system)
that you plan to use.

## Data-driven Entity Creation

While it is possible create [Entities](ecs.md#entity) at runtime, it is
generally preferred that they are created via data
[Blueprints](ecs.md#blueprint). The following steps will allow you to enable
this functionality.

### Entity Schema

You first need to write a [FlatBuffers](https://google.github.io/flatbuffers)
[Schema
File](https://google.github.io/flatbuffers/flatbuffers_guide_writing_schema.html)
that defines the structure of the [Blueprints](ecs.md#blueprints)

```
union ComponentDefType {
  lull.TransformDef,
  lull.AudioDef,
  lull.RenderDef,
  lull.TextDef,
}
```

This file is specific to your application. In the same way that you decide which
[Systems](ecs.md#systems) you want to have in your runtime, you also decide
which [ComponentDefs](ecs.md#entity-def) you want to allow in your
[Blueprints](ecs.md#blueprint). Generally speaking, you want your
[ComponentDefs](ecs.md#entity-def) to match your [Systems](ecs.md#system).

Then, simple add the following "boiler-plate" to the rest of your schema file.

```
table ComponentDef {
  def: ComponentDefType;
}
table EntityDef {
  components: [ComponentDef];
}
root_type EntityDef;
```

### EntityFactory::Initialize

Because the [EntityDef](ecs.md#entity-def) is specific to your application, you
need to provide the [Entity Factory](ecs.md#entity-factory) with the necesssary
information to create [Entities](ecs.md#entity) from your
[Blueprints](ecs.md#blueprint). You do this by calling
`EntityFactory::Initialize()` in a slightly different way:

```c++
  entity_factory->Initialize<EntityDef, ComponentDef>(
      GetEntityDef, EnumNamesComponentDefType());
```

There are 2 template arguments and 2 function arguments:

*   EntityDef: The name of the table that contains the array of ComponentDefs.
*   ComponentDef: The name of the table that contains the ComponentDefType
    union.
*   GetEntityDef: Function generated by flatc that takes a byte array and
    returns the corresponding EntityDef.
*   EnumNamesComponentDefType: This function is generated by flatc and returns
    an array of names that corresponds to the ComponentDefType union.

With this information, the [Entity Factory](ecs.md#entity-factory) is able to
load convert a raw binary asset into a [Blueprint](ecs.md#blueprint) in order to
create an [Entity](ecs.md#entity).

## Updating Lullaby

Now that you've setup everything and can create [Entities](ecs.md#entity), the
only thing left to do is start updating them. The general "flow" of a single
frame update for Lullaby is something like:

1.  Calculate Delta Time
2.  Update Input Manager
3.  Process Assets
4.  Dispatch Queued Dispatchers
5.  Advance Simulation
6.  Present Output

### Calculate Delta Time

Many Lullaby [Systems](ecs.md#system) need to know the duration that has elapsed
since the last time they were updated. As such, it is important to track this
information in your main loop.

```c++
  lull::Clock::time_point curr_frame_time = lull::Clock::now();
  lull::Clock::duration delta_time = now - prev_frame_time;
  ...
  prev_frame_time = curr_frame_time;
```

### Update Input Manager

To update the [Input Manager](base-tech.md#input-manager), you first need to
read your hardware state (using whatever SDKs you want) and "update" the
corresponding device state in the input manager. For example:

```c++
  bool pressed = ...;
  bool repeated = ...;
  input_manager->UpdateButton(lull::InputManager::kController,
                              lull::InputManager::kPrimaryButton,
                              pressed, repeated);
```

Then, simply, "swap" the Input Manager buffers by calling:

```c++
  input_manager->AdvanceFrame(delta_time);
```

### Process Assets

```c++
  asset_loader->Finalize(1);
```

### Dispatch Queued Dispatchers

As mentioned above, you can dispatch any Events queued in your
[Dispatcher](base-tech.md#dispatcher) by calling:

```c++
  queued_dispatcher->Dispatch();
```

It is generally recommended to do this at the start of the "frame" before any
simulation begins.

### Advance Simulation

Call the appropriate `AdvanceFrame` (or equivalent) functions in the various
[Systems](ecs.md#system) you've created.

### Present Output

This step takes the current "state" of the simulation and pushes that data to
"outputs" like audio and graphics.

## Conclusion

And that's basically it.

