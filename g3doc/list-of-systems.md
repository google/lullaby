# List of Systems

The following is a list of [Systems](ecs.md#system) that are available in the
Lullaby repository.  Detailed documentation for individual
[Systems](ecs.md#system) is available in their respective folders.

System                               | Description
------------------------------------ | --------------------------
[animation][animation]               | Animates component data (eg. position, rotation, color) values over time
[audio][audio]                       | Loads, plays, and maintains audio assets
[backdrop][backdrop]                 | Creates an entity that acts as a backdrop to another
[clip][clip]                         | Allows an entity to act as a stencil mask for its descendents
[collision][collision]               | Provides basic collision queries
[controller][controller]             | Handles the visual representation of the controller
[datastore][datastore]               | Allows arbitrary data to be stored with an Entity
[deform][deform]                     | Handles deformation logic for transform and render systems
[dispatcher][dispatcher]             | Provides a Dispatcher as a Component for each Entity
[layout][layout]                     | Automatically updates positions of all children based on layout parameters
[map_events][map_events]             | Allows mapping of events in a data-driven manner
[name][name]                         | Associates a name with an entity
[nav_button][nav_button]             | Supports easy creation of navigation buttons
[render][render]                     | Draws (renders) Entities on the screen
[reticle][reticle]                   | Provides a point-and-click style cursor
[script][script]                     | Allows scripts to be attached to entites
[scroll][scroll]                     | Updates the position of its children based on touch input
[text][text]                         | Manages the rendering of i18n strings
[text_input][text_input]             | Updates Entity display text on input event
[track_hmd][track_hmd]               | Updates the transform of entities based on the HMD transform
[transform][transform]               | Provides Entities with position, rotation, scale and volume
[visibility][visibility]             | Hides and shows content on events


[animation]: https://github.com/google/lullaby/tree/master/lullaby/systems/animation
[audio]: https://github.com/google/lullaby/tree/master/lullaby/systems/audio
[backdrop]: https://github.com/google/lullaby/tree/master/lullaby/contrib/backdrop
[clip]: https://github.com/google/lullaby/tree/master/lullaby/contrib/clip
[collision]: https://github.com/google/lullaby/tree/master/lullaby/systems/collision
[controller]: https://github.com/google/lullaby/tree/master/lullaby/systems/controller
[datastore]: https://github.com/google/lullaby/tree/master/lullaby/systems/datastore
[deform]: https://github.com/google/lullaby/tree/master/lullaby/contrib/deform
[dispatcher]: https://github.com/google/lullaby/tree/master/lullaby/systems/dispatcher
[layout]: https://github.com/google/lullaby/tree/master/lullaby/contrib/layout
[map_events]: https://github.com/google/lullaby/tree/master/lullaby/contrib/map_events
[name]: https://github.com/google/lullaby/tree/master/lullaby/systems/name
[nav_button]: https://github.com/google/lullaby/blob/master/lullaby/contrib/nav_button
[render]: https://github.com/google/lullaby/tree/master/lullaby/systems/render
[reticle]: https://github.com/google/lullaby/tree/master/lullaby/contrib/reticle
[script]: https://github.com/google/lullaby/tree/master/lullaby/systems/script
[scroll]: https://github.com/google/lullaby/tree/master/lullaby/contrib/scroll
[text]: https://github.com/google/lullaby/tree/master/lullaby/systems/text
[text_input]: https://github.com/google/lullaby/tree/master/lullaby/contrib/text_input
[track_hmd]: https://github.com/google/lullaby/blob/master/lullaby/contrib/track_hmd
[transform]: https://github.com/google/lullaby/tree/master/lullaby/systems/transform
[visibility]: https://github.com/google/lullaby/blob/master/lullaby/contrib/visibility

