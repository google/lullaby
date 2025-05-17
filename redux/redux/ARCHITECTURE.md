# Redux Architecture

Redux aims to provide a foundation for creating 3D simulation applications. It
is designed as a collection of C++ libraries that work cohesively together, but
can be used independently depending on the project's needs.

Redux leverages existing, open-source technologies (e.g. filament for rendering,
resonance for audio), but aims to provide a consistent, data-driven interface
to simplify development and ease-of-use.

The architecture of redux is split into 4 main sections: modules, engines,
systems, and tools.

*Modules* consist of small, self-contained libraries such as a 3D math library,
event messaging library and image and audio codecs. Furthermore, the "base"
module itself consists of even smaller utilities like hashing, logging, 
RTTI, and reflection.

*Engines* are large, complex libraries like rendering, audio, physics,
animation, and scripting. As mentioned above, many of these engines are actually
integrations with existing, open-source libraries, but some of them are
developed entirely for use by redux.

Redux uses an Entity-Component-System (ECS) architecture to tie together the
various modules and engines into a cohesive engine. While the core of the ECS is
under modules/ecs, the *Systems* are where individual features (e.g. rendering,
audio, etc.) are exposed to users in a consistent and easy-to-use way.

Lastly, *tools* are asset processing pipelines that are used to convert content
from DCC tools (e.g. gltf or png files) into optimized formats for the engine.

## Overview

Everything in Redux starts and ends with the *Registry* (modules/base/registry).
The Registry is an implementation of the Service Locator pattern. Services, in
this context, are things like the rendering or audio engines, or the asset 
loading class, or even individual ECS systems.  The Registry owns these objects
and makes them available to each other.

There are no global objects within Redux itself; everything you want must come
from the Registry. This means you can have multiple Registry instances which
allows Redux-based simulations to run independently of each other. However,
things get complicated when it comes to the hardware resources, like the
filesystem or the GPU, and related contention, so we cannot guarantee perfect
isolation. However, 



Maths

Engines (Render, Audio, Physics, Animation, Scripting)

Systems/ECS

Blueprints

Datafiles/Var

Scripting

Platform

Choreographer

AssetLoader

