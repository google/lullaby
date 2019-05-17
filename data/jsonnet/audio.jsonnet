local utils = import "lullaby/data/jsonnet/utils.jsonnet";

// Audio functions to create AudioResponseDefs. Cannot be used by themselves,
// must be in call of responses.make_response(event, audio.function).
// Args:
//    sounds: [string] An array of sounds, one will be picked at random
//    if more than one provided.
//    load_type: AudioLoadType How to load the sound(s).
//    playback_type: AudioPlaybackType What to do if playback is requested
//    before the sound is loaded.
//    volume: float The volume level.
//    source_type: AudioSourceType The type of audio source.
//    spatial_directivity_alpha: float The directivity constant "alpha".
//    spatial_directivity_order: float The directivity constant "order".

{
  // Returns a part of AudioResponseDef with specified field values.
  _audio_def(args)::
  {
    "_type_":: "audio",
    "load_type" : utils.select("Preload", args, "load_type"),
    "playback_type" : utils.select("PlayIfReady", args, "playback_type"),
    "volume" : utils.select(1.0, args, "volume"),
    "source_type" : utils.select("SoundObject", args, "source_type"),
    "spatial_directivity_alpha" : utils.select(-1.0, args,
                                               "spatial_directivity_alpha"),
    "spatial_directivity_order" : utils.select(-1.0, args,
                                               "spatial_directivity_order"),
  },

  // Plays a default click sound with specified |args.playback_time|,
  // |args.volume|, |args.source_type|, |args.spatial_directivity_alpha| and
  // |args.spatial_directivity_order| parameters.
  play_btn_click(args)::
  {
    "sound": "sounds/btn_click.ogg"
  } + $._audio_def(args),

  // Plays a default click back sound with specified |args.playback_time|,
  // |args.volume|, |args.source_type|, |args.spatial_directivity_alpha| and
  // |args.spatial_directivity_order| parameters.
  play_btn_click_back(args)::
  {
    "sound": "sounds/btn_click_back.ogg"
  } + $._audio_def(args),

  // Plays a default important click sound with specified |args.playback_time|,
  // |args.volume|, |args.source_type|, |args.spatial_directivity_alpha| and
  // |args.spatial_directivity_order| parameters.
  play_btn_click_important(args)::
  {
    "sound": "sounds/btn_click_important.ogg"
  } + $._audio_def(args),

  // Plays a default hover sound with specified |args.playback_time|,
  // |args.volume|, |args.source_type|, |args.spatial_directivity_alpha| and
  // |args.spatial_directivity_order| parameters.
  play_btn_hover(args)::
  {
    "random_sounds": [
      "sounds/btn_hover1.ogg",
      "sounds/btn_hover2.ogg",
      "sounds/btn_hover3.ogg",
      "sounds/btn_hover4.ogg",
      "sounds/btn_hover5.ogg"
    ]
  } + $._audio_def(args),

  // Plays a default swipe sound with specified |args.playback_time|,
  // |args.volume|, |args.source_type|, |args.spatial_directivity_alpha| and
  // |args.spatial_directivity_order| parameters.
  play_swipe(args)::
  {
    "sound": "sounds/swipe.ogg"
  } + $._audio_def(args),

  // Plays specified |args.sounds| with |args.playback_time|,
  // |args.volume|, |args.source_type|, |args.spatial_directivity_alpha| and
  // |args.spatial_directivity_order| parameters.
  // Note: |args.sounds| must be an array, will play in random order if more
  // than 1 sound provided.
  play(args)::
  if std.length(args.sounds) == 1 then
  {"sound": args.sounds[0]} + $._audio_def(args)
  else
  {"random_sounds": args.sounds} + $._audio_def(args),
}
