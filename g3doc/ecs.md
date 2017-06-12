# Entity-Component-System


[TOC]

## Introduction

At its core, Lullaby uses an
[Entity-Component-System](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)
(ECS) architecture to manage the efficient creation and manipulation of in-world
objects. Furthermore, Lullaby's ECS is designed to be heavily data-driven to
allow for quick iteration and development.

This architecture is based on the [Composition over inheritance
principle](https://en.wikipedia.org/wiki/Composition_over_inheritance). It
allows for the behavior of an [Entity](#entity) to change at runtime by adding
or removing [Components](#component) dynamically. It also aligns with
[Data-Oriented Design
techniques](https://en.wikipedia.org/wiki/Data-oriented_design), allowing for an
efficient runtime.

This page describes the various classes and concepts that make up Lullaby's ECS
architecture. It assumes that you have read the brief ECS
[introduction](introduction.md#systems).

#### Terminology

It is unfortunate that the terms [Component](#component) and [System](#system)
are rather generic since they have many meanings in software engineering.
Unfortunately, that is the standard terminology used by Entity-Component-System
architectures and, as such, is the term used in Lullaby. When discussing
[Components](#component) and [Systems](#system) as they pertain to the
Entity-Component-System, we will captilize the terms, whereas we will just say
"component" and "system" when discussing other things.

## Entity

An **Entity** represents a single, uniquely identifiable "thing" in the virtual
reality world.

Internally, Lullaby represents an Entity as a unique integer value. An Entity is
literally just a number. In code, it is defined as:

```
using Entity = uint32_t;
```

There is no data or logic associated with an Entity. Instead, [Systems](#system)
store a mapping of Entities to specific instances of [Component](#component)
data. All [Components](#component) that map to the same Entity value can be
considered as "belonging" to that Entity.

## Component

**Components** encapsulate a set of data that represents a single aspect of an
[Entity](#entity).

A Component can be a single piece of data (eg. position) or it can a collection
of closely related data (eg. a "volumetric transform" which contains position,
rotation, scale, and bounding box). Some other potential examples of Component
data includes: velocity, mass, health, meshes, textures, sounds, timers, etc.
Basically, any data you may want associated with an [Entity](#entity) must be
stored in a Component.

A Component is just pure data; it provides no functionality or behaviour on its
own. Components are "plain-old-data" (POD) types. Two different
[Systems](#system) could rely on the same Component data to give an
[Entity](#entity) completely different behaviours. For example, a position
Component can be used by an Audio System to determine the loudness of an
[Entity](#entity), while the same position data can be used by a Collision
System to determine when two [Entities](#entity) are intersecting.

## System

A **System** provides the logic for a single aspect of an [Entity](#entity).
Systems operate on one or more [Components](#component) to provide
[Entities](#entity) with specific behaviours.

For example, a Render System would use the position, rotation, mesh, and texture
[Component](#component) data for an [Entity](#entity) to draw it on the screen.
A Physics System would update the position of an [Entity](#entity) using the
position and velocity of the [Entity](#entity).

As noted above, [Systems](#system) store a mapping of Entities to specific
instances of [Component](#component) data. In other words, Systems are
responsible for the actual storage and lifetime management of
[Components](#component). This also allows Systems to store
[Components](#component) using the most efficient data structure for their
particular use-case. As such, a System may actually store zero, one or more
[Component](#component) data instances for any given [Entity](#entity).

## Blueprint

A **Blueprint** represents the serialized state of an [Entity](#entity). This
serialized state is effectively a snapshot of all the [Component](#component)
data associated with an [Entity](#entity). This allows [Systems](#system) to
"build" [Components](#component) for an [Entity](#entity) using the data in a
Blueprint.

A Blueprint is not specific to a single [Entity](#entity) instance; multiple
[Entities](#entity) can be instantiated in the runtime from the same Blueprint.

## Schema

Lullaby uses the [FlatBuffers](https://google.github.io/flatbuffers) library to
serialize and store [Blueprints](#blueprint). The FlatBuffers library uses
**Schema** files (generally given a .fbs extension) that defines the structure
of the data to encode. It is similar to a class declaration, but for data
instead of code. All the data that can stored in a [Blueprint](#blueprint) must
be defined in a Schema file first.

Information on how to write a Schema can be found
[here](https://google.github.io/flatbuffers/flatbuffers_guide_writing_schema.html).

In Lullaby, most [Schemas](#schema) are named with a "Def" suffix because they
are data defintions; they describe the structure of data.

## EntityDefs and ComponentDefs

As described above, a [Blueprint](#blueprint) represents the serialized state of
an [Entity](#entity). The [Schema](#schema) that describes an Entity
[Blueprint](#blueprint) is called an **EntityDef** and is normally written as:

```
union ComponentDefType {
  // list of System-defined Defs.
}

table ComponentDef {
  def: ComponentDefType;
}

table EntityDef {
  components: [ComponentDef];
}
```

In the same way that the behaviour of an [Entity](#entity) is defined by the set
of [Components](#component) associated with it, an EntityDef is defined by the
list of **ComponentDefs** associated with it. The ComponentDef type is a
flatbuffer union of "Defs" defined by [Systems](#system).

As described above, [Systems](#system) are responsible for storing
[Component](#component) data in a runtime efficient manner. However,
ComponentDefs should be structured such that they are easy for a developer to
understand and update. It is the responsibility of the [System](#system) to map
between the runtime [Component](#component) data and its [Schema](#schema)-based
equivalent.

For example, a TransformDef could be described as:

```
table TransformDef {
  position: Vec3;
  rotation: Vec3;  // Euler angles
  scale: Vec3;
}
```

But, the data could be stored in a [System](#system) as:

```
class TransformSystem {
  std::vector<std::pair<Entity, vec3>> positions;
  std::vector<std::pair<Entity, quat>> rotations;
  std::vector<std::pair<Entity, vec3>> scales;
};
```

In this example, the Transform data is stored as three separate
[Components](#component) as a
[structure-of-arrays](https://en.wikipedia.org/wiki/AOS_and_SOA) for performance
reasons. Furthermore, the |rotation| is stored as euler angles in the
ComponentDef (as they are easy to understand), but are converted into
quaternions when stored in the [System](#system).

## Entity Factory

The **EntityFactory** is responsible for "creating" [Entities](#entity) from
[Blueprints](#blueprint). A [Blueprint](#blueprint) can exist in one of three
formats:

*   Flatbuffer binary tables: Encoding format defined by flatbuffers that can be
    used to store and transport data efficiently.

*   Native C++ runtime data: An instance of the data represented in memory using
    C++ classes and structs.

*   JSON text files: Human-readable and easily editable. These files are
    converted into flatbuffer binaries as part of the build process.

The C++ Blueprint class can be used to create, load and save
[Blueprints](#blueprint) in either the flatbuffer or native formats.

To "create" an [Entity](#entity), the EntityFactory first generates a new unique
number for the [Entity](#entity). It then iterates over the
[ComponentDefs](#entity-defs-and-component-defs) stored in the
[Blueprint](#blueprint), passing them to their owning [System](#system) along
with the newly generated [Entity](#entity) ID. The [System](#system) is then
responsible for creating the corresponding [Components](#components) internally
and associating them with the given [Entity](#entity).

The EntityFactory can also "destroy" an [Entity](#entity) by simply requesting
all [Systems](#systems) to destroy any [Components](#component) associated with
the given [Entity](#entity).
