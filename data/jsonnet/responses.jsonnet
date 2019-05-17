local utils = import "lullaby/data/jsonnet/utils.jsonnet";

{
    // Constructor of AnimationResponseDefs, EventResponseDefs and
    // AudioResponseDefs. Takes in |event| to respond to and |args| of
    // AnimationDefs, EventDefs and AudioRespondDefs parameters respectively.
    local response = {
      animation: function(event, args) {
        "def_type" : "AnimationResponseDef",
        "def" : {
          "inputs" : [{
            "event" : event,
            "local" : true,
          }],
          "animation" : args
        }
      },
      event: function(event, args) {
        "def_type" : "EventResponseDef",
        "def" : {
          "inputs" : [{
            "event" : event,
            "local" : true,
          }],
          "outputs" : [args]
        }
      },
      audio: function(event, args) {
        "def_type" : "AudioResponseDef",
        "def": {
          "inputs" : [{
            "event" : event,
            "local" : true,
          }],
        } + args
      }
    },

    // Constructor of ScriptOnEventDefs, ScriptEveryFrameDefs,
    // ScriptOnCreateDefs, ScriptOnPostCreateDefs, ScriptOnDestroyDefs. Takes
    // in |args| of ScriptDefs.
    local script_response = {
      on_hover: function(args)
        self.on_event("lull::StartHoverEvent", args),
      on_stop_hover: function(args)
        self.on_event("lull::StopHoverEvent", args),
      on_click: function(args)
        self.on_event("lull::ClickEvent", args),
      on_event: function(event, args) [{
        "def_type": "ScriptOnEventDef",
        "def": {
          "inputs": [{
              "event": event,
              "local": true,
            }],
          "script": $._script_def(args)
        }
      }],
      every_frame: function(args) [{
        "def_type": "ScriptEveryFrameDef",
        "def": {
          "script": $._script_def(args)
        }
      }],
      on_create: function(args) [{
        "def_type": "ScriptOnCreateDef",
        "def": {
          "script": $._script_def(args)
        }
      }],
      on_post_create_init: function(args) [{
        "def_type": "ScriptOnPostCreateInitDef",
        "def": {
          "script": $._script_def(args)
        }
      }],
      on_destroy: function(args) [{
        "def_type": "ScriptOnDestroyDef",
        "def": {
          "script": $._script_def(args)
        }
      }]
    },

     _script_def(args)::
      if std.objectHas(args, "filename") then {
       "filename": args.filename,
       "debug_name": utils.select("inline script", args, "debug_name"),
      } else {
       "code": args.code,
       "debug_name": utils.select("inline script", args, "debug_name"),
       "language": utils.select("LullScript", args, "language")
      },

    // Takes in |event| string and |def_arr| array with AnimationDefs
    // and/or EventDefs. Returns an array with AnimationResponseDefs and/or
    // EventResponseDefs respectively for specified |event|.
    make_response(event, def_arr):: [
        if std.objectHas(response, def._type_) then
          response[def._type_](event, def)
        else error "No ResponseDef for Def %s" % [def._type_]
      for def in def_arr
    ],

    local script_events = ["on_hover", "on_stop_hover", "on_click",
                           "every_frame", "on_create", "on_post_create_init",
                           "on_destroy"],
    // Takes in |args| of ScriptDefs and maps them to ScriptOnEventDefs/
    // ScriptEveryFrameDefs/ScriptOnCreateDefs/ScriptOnPostCreateDefs/
    //ScriptOnDestroyDefs accordingly.
    make_script_response(args)::
     std.flattenArrays([
        if std.objectHas(args, event) then
          script_response[event](args[event])
        else [],
      for event in script_events
    ]),
}



