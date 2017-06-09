# List of Systems

The following is a list of [Systems](ecs.md#system) that are available in the
Lullaby repository.  Detailed documentation for individual [Systems](ecs.md#system)
is available in their respective folders.

System                                              | Description
--------------------------------------------------- | --------------------------
[animation](../src/systems/animation)               | Animates component data (eg. position, rotation, color) values over time
[audio](../src/systems/audio)                       | Loads, plays, and maintains audio assets
[collision](../src/systems/collision)               | Provides basic collision queries
[datastore](../src/systems/datastore)               | Allows arbitrary data to be stored with an Entity
[deform](../src/systems/deform)                     | Handles deformation logic for transform and render systems
[dispatcher](../src/systems/dispatcher)             | Provides a Dispatcher as a Component for each Entity
[layout](../src/systems/layout)                     | Automatically updates positions of all children based on layout parameters
[map_events](../src/systems/map_events)             | Allows mapping of events in a data-driven manner
[name](../src/systems/name)                         | Associates a name with an entity
[render](../src/systems/render)                     | Draws (renders) Entities on the screen
[reticle](../src/systems/reticle)                   | Provides a point-and-click style cursor
[script](../src/systems/script)                     | Allows scripts to be attached to entites
[scroll](../src/systems/scroll)                     | Updates the position of its children based on touch input
[text](../src/systems/text)                         | Manages the rendering of i18n strings
[transform](../src/systems/transform)               | Provides Entities with position, rotation, scale and volume
