local utils = import "lullaby/data/jsonnet/utils.jsonnet";
local animations = import "lullaby/data/jsonnet/animations.jsonnet";
local events = import "lullaby/data/jsonnet/events.jsonnet";
local eves = import "lullaby/data/jsonnet/animations.jsonnet";
local responses = import "lullaby/data/jsonnet/responses.jsonnet";

// Set of fade functions returning complete responses consisting of
// AnimationResponseDef and EventResponseDef nessesary for fade animation
// sequence.
//
// In your app send a local event with the name corresponding to the name of
// used function and listen for the event named '{function_name}Complete' to
// know when the animation sequence is over. You can also use function "all"
// which add all fade events responses to your entitry:
// Event "Show" to enable and fade in the entity to become visibile.
// Event "Hide" to fade out the entity to become invisible and then disable it.
// Event "ShowNow" to enable and immidietly make entity visibile.
// Event "HideNow" to enable and immidietly make entity invisibile.
// Event "HideNowThenShow" to immidietly make entity invisibile and then fade
// it in.
//
// To add these functions to the blueprint, concatenate to your components
// array like this:
// "components": [ //some defs// ] + fade.show_now({})
// Each function also accepts (listed with default value)
// {
//    // Amount of time in ms over which to perform the fading animation.
//    time_ms: 150,
//    // Name of the event on which to perform the fading animation.
//    input_event: //function name corresponding event name from |event_names|
//    // Name of the event which to perform on success of fading animation.
//    output_name: //function name corresponding event name from |event_names|
// }
//
// If the fading is not working on your entity, you might need a RenderDef on
// your entity to make render-color-alpha-multiplier-descendants animation
// channel work if your entity does not have one. Here is a simple RenderDef
// example:
// {
//    "def_type" : "RenderDef",
//    "def" : {
//      "shader" : "shaders/color.fplshader",
//      "color" : {"r" : 1.0, "g" : 1.0, "b" : 1.0, "a" : 1.0},
//    }
//  }

{
  event_names :: {
    // Send these to trigger fading animations.
    show: "Show",
    show_now: "ShowNow",
    hide: "Hide",
    hide_now: "HideNow",
    hide_now_then_show: "HideNowThenShow",
    // Listen for these to know when fading animations are successfully
    //completed.
    show_complete: "ShowCompleteEvent",
    show_now_complete: "ShowNowCompleteEvent",
    hide_complete: "HideCompleteEvent",
    hide_now_complete: "HideNowCompleteEvent",
  },

  // Enables and gradually sets entity's alpha to 1 in |time_ms| ms.
  // Fires a local "ShowCompleteEvent" or a given |output_event| on success.
  show(args)::
  responses.make_response(
    utils.select($.event_names.show, args, "input_event"),
    [
      animations.fade_in({
            time_ms: utils.select(150, args, "time_ms"),
            start_delay_ms: utils.select(0, args, "start_delay_ms"),
            on_success:[events.local_event(
                        utils.select($.event_names.show_complete,
                        args, "output_event"))]}),
      events.enable_global()
    ]
  ),

  // Enables and sets entity's alpha to 1. Fires a local "ShowNowCompleteEvent"
  // or a given |output_event| on on success.
  show_now(args)::
  $.show(args + { time_ms: 0.0,
                  start_delay_ms: utils.select(0, args, "start_delay_ms"),
                  input_event: utils.select($.event_names.show_now,
                                             args, "input_event"),
                  output_event: utils.select($.event_names.show_now_complete,
                                             args, "output_event")}),

  // Gradually sets entity's alpha to 0 in |time_ms| ms.
  // Disables entity and fires a local "HideCompleteEvent" or a given
  // |output_event| on success.
  hide(args)::
  responses.make_response(
    utils.select($.event_names.hide, args, "input_event"),
    [
      animations.fade_out({
            time_ms: utils.select(150, args, "time_ms"),
            start_delay_ms: utils.select(0, args, "start_delay_ms"),
            on_success:[events.local_event(
                        utils.select($.event_names.hide_complete,
                        args, "output_event")),
                        events.disable_global()]}),
    ]
  ),

  // Sets entity's alpha to 0. Disables entity and fires a local
  // "HideNowCompleteEvent" or a given |output_event| on success.
  hide_now(args)::
  $.hide(args + { time_ms: 0.0,
                  start_delay_ms: utils.select(0, args, "start_delay_ms"),
                  input_event: utils.select($.event_names.hide_now,
                                             args, "input_event"),
                  output_event: utils.select($.event_names.hide_now_complete,
                                             args, "output_event")}),

  // Sets entity's alpha to 0. Fires a local "Show" event or a given
  // |output_event| on success.
  hide_now_then_show(args)::
  responses.make_response(
    utils.select($.event_names.hide_now_then_show, args, "input_event"),
    [
      animations.fade_out({
            time_ms: 0.0,
            start_delay_ms: utils.select(0, args, "start_delay_ms"),
            on_success:[events.local_event(
                        utils.select($.event_names.show,
                        args, "output_event"))]}),
    ]
  ),

  // Complete fading set of functions for hide and show behaviour.
  all(args)::
  $.show(args) +
  $.show_now(args) +
  $.hide(args) +
  $.hide_now(args) +
  $.hide_now_then_show(args)
}


