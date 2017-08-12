# Introduction to Lullaby


[TOC]

## Overview

Lullaby is a collection of C++ libraries designed to provide an efficient way to
create and interact with objects in a virtual/augmented reality world.

It is the Engine that drives your VR/AR apps using the underlying SDKs.

<dl>
<dt>Your App: The Experience</dt>
<dd>The virtual/augemented reality experiences only you can give to your users
(eg. watching 360 videos, exploring the world, etc.)</dd>

<dt>Lullaby: The Engine</dt>
<dd>A collection of libraries designed for building VR experiences running on
top of a high-performance framework.</dd>

<dt>VR/AR SDKs: The Platform</dt>
<dd>Provides access to low-level hardware (eg. HMD, Controller, etc.) and
features (eg. OpenGL, Spatial Audio, Scanline Racing, etc.)</dd>
</dl>

## High-Level Architecture

At a high level, Lullaby's architecture can be divided into 3 broad categories:
[Systems](#systems), [Base Tech](#base-tech), and [Utilities](#utilities).

#### Systems

Lullaby's primary goal is to provide an efficient, data-driven way to create,
manipulate, and interact with objects in a "virtual" world. For this, Lullaby
uses an
[Entity-Component-System](https://en.wikipedia.org/wiki/Entity%E2%80%93component%E2%80%93system)
(ECS) architecture. An ECS consists of three parts: [Entities](ecs.md#entity),
[Components](ecs.md#component), and [Systems](ecs.md#system).

An [Entity](ecs.md#entity) represents a single, uniquely identifiable "thing" in
the virtual reality world. However, an [Entity](ecs.md#entity) is literally just
a number; it has no other data on its own. Instead, specific pieces of data
(like position, or velocity, or shape) are associated with an Entity at runtime.
These data objects are called [Components](ecs.md#component). Lastly,
[Systems](ecs.md#system) process and update [Components](ecs.md#component),
effectively giving [Entities](ecs.md#entity) their behaviours. In other words,
the behaviour of an [Entity](ecs.md#entity) is a result of the
[Components](ecs.md#components) associated with it and the
[Systems](ecs.md#system) which operate on those [Compeonents](ecs.md#component).

[Systems](ecs.md#system) provide all the actual functions and behaviours. Each
[System](ecs.md#system) is specialized to perform a specific job, such as
playing animations, rendering graphics or detecting collisions.
[Systems](ecs.md#system) perform these tasks by processing one or more types of
[Components](ecs.md#component). A [Component](ecs.md#component) defines a set of
properties, such as 3D position, render shape, or interaction state. A
collection of [Components](ecs.md#component) is associated with an
[Entity](ecs.md#entity). An [Entity](ecs.md#entity) represents a single,
uniquely identifiable "thing" in the virtual reality world. However, an
[Entity](ecs.md#entity) is literally just a number; it has no other data on its
own. The behaviour of an [Entity](ecs.md#entity) is defined by the
[Components](ecs.md#component) associated with it and the
[Systems](ecs.md#system) which operate on those [Components](ecs.md#component).

For example, a Physics System could update the position of all
[Entities](ecs.md#entity) with both position and velocity
[Components](ecs.md#component), or a Render System would render
[Entities](ecs.md#entity) using the positon and shape
[Components](ecs.md#component).

Lullaby also has an offline data representation that enables fast data-driven
iteration of its Entity-Component-System architecture. Applications use data
files known as [Blueprints](ecs.md#blueprint) to describe the state of an Entity
that can then be instantiated in the runtime. [Schema](ecs.md#blueprint) files
are used to describe the structure of the data contained in a
[Blueprint](ecs.md#blueprint).

The bulk of Lullaby's functionality is implemented as individual
[Systems](ecs.md#system), such as the
[TransformSystem](../src/systems/transform),
[AudioSystem](../src/systems/audio), [RenderSystem](../src/systems/render), and
[AnimationSystem](../src/systems/animation). A complete list of
[Systems](ecs.md#system) can be found [here](list-of-systems.md).

Details about Lullaby's ECS implementation can be found [here](ecs.md).

A list of available Systems can be found [here](list-of-systems.md).

#### Base Tech

Lullaby's [Systems](#systems) often require access to a common set of
functionality in order to work with each other. This functionality, along with
the ECS implementation, represent the "base" of the Lullaby Engine. It is
important for users of Lullaby to have a good understanding these libraries.

More information about these base libraries can be found [here](base-tech.md).

#### Utilities

Lastly, Lullaby provides several utility libraries. These range from simple
functions (eg. string hashes, bit manipulation, etc.), to classes (eg. variant
types, optional type, thread-safe containers, etc.) to complete modules (like a
JNI interface and integration with scripting languages like JavaScript and Lua).

Most of these Utilities are used internally by [Systems](#systems) and [Core
Tech](#core-tech), or are available as optional extensions to Lullaby's primary
functionality. Unfortunately, the only way to learn about these Utilities is to
explore the Lullaby codebase.

## Codebase

All C++ code is available under the src/ folder. It contains the following
subfolders:

*   base: The ECS implementation and the base tech libraries.
*   systems: The "core" Lullaby Systems that provide most of the critical
    functionality like animation, audio, and rendering. These libraries are
    developed and maintained by the main Lullaby team.
*   contrib: This contains useful Systems developed by various teams throughout
    Google. These tend to be more "mid-level" Systems, like dropdowns and
    tooltips.
*   util: Various utility libraries as described above.
*   schemas: Flatbuffer schema files.

Note: Some code that should be in util/ may actually be in base/ and vice-versa
due to legacy reasons. (Mainly because it's somewhat arbitrary as to what is a
"base" technology vs. what is a utility.)


Finally, the rest of the folders are:

*   g3doc: This documentation you are reading.

## Building

Instructions on building the Lullaby open source libraries is available
[here](building_github.md).

## Putting it all Together

Follow our step-by-step [Integration Guide](integration-guide.md) for how to
integrate Lullaby into your application.

