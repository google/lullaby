local utils = import "lullaby/data/jsonnet/utils.jsonnet";

// Animation functions to create AnimationDefs.
// Args:
//    to : [float] The target values for the channel.
//    time_ms : int The amount of time (in milliseconds) to take to reach the
//              target values.
// Optional args:
//    start_delay_ms : int The amount of time (in milliseconds) to delay
//                     starting the animation.
//    on_complete_events: [EventDef] Event to be send upon animation completion
//                        (animation might end interrupted).
//    on_success_events: [EventDef] Event to be send only upon animation succesful
//                       completion (ie reaches the target value).
//    on_cancelled_events: [EventDef] Event to be send upon animation interruption.

{
    // Returns an AnimationDef with specified field values.
    _anim_def(args, targets)::
    {
        "_type_":: "animation",
        "targets" : targets,
        "on_complete_events": utils.select([], args, "on_complete"),
        "on_success_events": utils.select([], args, "on_success"),
        "on_cancelled_events": utils.select([], args, "on_cancelled")
    },

    // Moves entity on z axis by specified value |args.to| in |args.time_ms|.
    // Optionally fires specified events.
    move_z(args)::
    $._anim_def(args, [{
      "channel" : "transform-position-z",
      "values" : [args.to],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    // Moves entity on x axis by specified value |args.to| in |args.time_ms|.
    // Optionally fires specified events.
    move_x(args)::
    $._anim_def(args, [{
      "channel" : "transform-position-x",
      "values" : [args.to],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    // Moves entity on specified axis |position| by specified value |args.to| in
    // |args.time_ms|. Optionally fires specified events.
    move_position(args)::
    $._anim_def(args, [{
      "channel" : "transform-position-%s" % [args.position],
      "values" : [args.to],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    // Gradually sets entity's alpha channel to 1.0 in |args.time_ms|.
    // Optionally fires specified events.
    fade_in(args)::
    $._anim_def(args, [{
      "channel" : "render-color-alpha-multiplier-descendants",
      "values" : [1.0],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    // Gradually sets entity's alpha channel to 0.0 in |args.time_ms|.
    // Optionally fires specified events.
    fade_out(args)::
    $._anim_def(args, [{
      "channel" : "render-color-alpha-multiplier-descendants",
      "values" : [0.0],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    // Gradually sets entity's alpha channel to |alpha| in |args.time_ms|.
    // Optionally fires specified events.
    fade_to(args)::
    $._anim_def(args, [{
      "channel" : "render-color-alpha-multiplier-descendants",
      "values" : [args.alpha],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    fade_in_no_descendants(args)::
    $._anim_def(args, [{
      "channel" : "render-color-alpha",
      "values" : [1.0],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    fade_out_no_descendants(args)::
    $._anim_def(args, [{
      "channel" : "render-color-alpha",
      "values" : [0.0],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    fade_to_no_descendants(args)::
    $._anim_def(args, [{
      "channel" : "render-color-alpha",
      "values" : [args.alpha],
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    // Gradually sets entity's color to |color| in |args.time_ms|.
    // Optionally fires specified events.
    fill_with_color(args)::
    $._anim_def(args, [{
      "channel" : "render-color-rgb-multiplier-descendants",
      "values" : args.color,
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),

    fill_with_color_no_descendants(args)::
    $._anim_def(args, [{
      "channel" : "render-color-rgb",
      "values" : args.color,
      "time_ms" : args.time_ms,
      "start_delay_ms" : utils.select(0, args, "start_delay_ms"),
    }]),
}
