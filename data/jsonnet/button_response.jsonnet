local utils = import "lullaby/data/jsonnet/utils.jsonnet";
local responses = import "lullaby/data/jsonnet/responses.jsonnet";

{
    // Turns the "external" events (ie. lull::ClickEvent) into an "internal"
    // event (ie. LocalClickEvent) for all siblings (shadow) and children
    // (foreground and text).
    _map_events_def(event):: [{
      "def_type": "MapEventsToSiblingsDef",
      "def": {
        "events": {
          "input_events": [{
            "event": $._make_event_str(event, true),
            "local" : true,
          }],
          "output_events": [{
            "event": $._make_event_str(event, false),
            "local" : true,
          }],
        }
      }
    }, {
      "def_type": "MapEventsToDescendantsDef",
      "def": {
        "events": {
          "input_events": [{
            "event": $._make_event_str(event, true),
            "local" : true,
          }],
          "output_events": [{
            "event": $._make_event_str(event, false),
            "local" : true,
          }],
        }
      }
    }],

    // Concatinates event name with lull::Event if the receiving entity has
    // a Collision def on it. Otherwise makes a Local event.
    _make_event_str(event, collidable)::
      if collidable then
        "lull::%sEvent" % [event]
      else
        "Local%sEvent" % [event],

    // Helper function for getting the response array. Takes in |args.args|
    // which is a button part entity and a |args.collidable|
    // boolean which represents whether an entity is an original event receiver,
    // meaning every received event will be mapped to an internal copy of itself
    // with MapEventsToSiblingsDef.
    _response_helper(args, collidable)::
    {
      local events = {"on_hover":"StartHover", "on_stop_hover":"StopHover",
                      "on_click":"Click"},
      "components": std.flattenArrays([
        if std.objectHas(args, event) then
           responses.make_response($._make_event_str(events[event], collidable),
                                   args[event]) +
            if collidable then
              $._map_events_def(events[event])
            else []
        else [],
        for event in std.objectFields(events)
      ]),
    },

    // Takes in |args.args| which is a button part object (Background, Shadow,
    // etc.) and returns its specified response components if any- an array of
    // AnimationResponseDef and/or EventResponseDef and their mapped
    // MapEventsToSiblingsDefs.
    get_mapped_response(args)::
      $._response_helper(args, true).components,

    // Takes in |args.args| which is a button part object (Background, Shadow,
    // etc.) and returns its specified response components if any- an array of
    // AnimationResponseDef and/or EventResponseDef.
    get_response(args)::
      $._response_helper(args, false).components,
}


