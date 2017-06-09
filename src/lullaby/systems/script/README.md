# `ScriptSystem`


Allows scripts to be attached to entites, that are run when an event fires, or
every frame.

Projects that want to use a particular scripting language must be built with
build flags that link in that language. For example, JS support requires
`--define lullaby_script_js=1`.

