# List of Systems

The following is a list of [Systems](ecs.md#system) that are available in the
Lullaby repository.  Detailed documentation for individual [Systems]
(ecs.md#system) is available in their respective folders.

System                                              | Description
--------------------------------------------------- | --------------------------
[animation](../src/lullaby/systems/animation)       | Animates component data (eg. position, rotation, color) values over time
[audio](../src/lullaby/systems/audio)               | Loads, plays, and maintains audio assets
[collision](../src/lullaby/systems/collision)       | Provides basic collision queries
[datastore](../src/lullaby/systems/datastore)       | Allows arbitrary data to be stored with an Entity
[deform](../src/lullaby/systems/deform)             | Handles deformation logic for transform and render systems
[dispatcher](../src/lullaby/systems/dispatcher)     | Provides a Dispatcher as a Component for each Entity
[layout](../src/lullaby/systems/layout)             | Automatically updates positions of all children based on layout parameters
[map_events](../src/lullaby/systems/map_events)     | Allows mapping of events in a data-driven manner
[name](../src/lullaby/systems/name)                 | Associates a name with an entity
[render](../src/lullaby/systems/render)             | Draws (renders) Entities on the screen
[reticle](../src/lullaby/systems/reticle)           | Provides a point-and-click style cursor
[script](../src/lullaby/systems/script)             | Allows scripts to be attached to entites
[scroll](../src/lullaby/systems/scroll)             | Updates the position of its children based on touch input
[text](../src/lullaby/systems/text)                 | Manages the rendering of i18n strings
[transform](../src/lullaby/systems/transform)       | Provides Entities with position, rotation, scale and volume
