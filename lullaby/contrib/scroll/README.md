# `ScrollSystem`


Translates its descendents in response to touchpad input. Depends on
`DispatcherSystem`, `AnimationSystem`, and `TransformSystem`.

The following systems are also required in order to create a scrollable view
that responds to touchpad events when hovered over:

*   `ReticleSystem` (for tracking where the remote is pointing). It's probably
    also desireable to create an entity from `data:blueprints/reticle.json` to
    actually draw the reticle. That blueprint depends on `data:reticle_shader`.
*   `CollisionSystem` (for tracking if the remote is pointing at any specific
    entity, depended on by `ReticleSystem`). The scrollable entity will need
    a `CollisionDef` in its blueprint, or else it won't respond to the hover.

No extra systems are requried to create a scrollable view that responds to
touchpad input regardless of whether or not it's currently being hovered on.
Instead, set `active_priority` to any positive int in the entity's `ScrollDef`.
