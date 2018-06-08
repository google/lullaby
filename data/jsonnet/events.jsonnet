// Events functions to create EventDefs.

{
    // Returns an EventDef with a global lull::EnableEvent.
    enable_global()::
    {
        "_type_":: "event",
        "event": "lull::EnableEvent",
        "global": true
    },

    // Returns an EventDef with a global lull::DisableEvent.
    disable_global()::
    {
        "_type_":: "event",
        "event": "lull::DisableEvent",
        "global": true
    },

    // Returns an EventDef with a local lull::EnableEvent.
    enable_local()::
    {
        "_type_":: "event",
        "event": "lull::EnableEvent",
        "local": true
    },

    // Returns an EventDef with a local lull::DisableEvent.
    disable_local()::
    {
        "_type_":: "event",
        "event": "lull::DisableEvent",
        "local": true
    },

    // Returns a local EventDef with a name |event_name|.
    local_event(event_name)::
    {
      "_type_":: "event",
      "event": event_name,
      "local": true,
    },

    // Returns a global EventDef with a name |event_name|.
    global_event(event_name)::
    {
      "_type_":: "event",
      "event": event_name,
      "global": true,
    },
}
