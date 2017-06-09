# `LayoutSystem`


Controls all children's translation using the Layout utility.

LayoutBoxSystem stores data for separate steps of the Layout process so that
other systems such as Text or NinePatch can resize without getting stuck in an
infinite loop.
