# Low-level bindings to Fluidsynth for Lua

To get started, import the library.

```lua
local FS = require "cfluidsynth"
```
Then use the fluidsynth api just like you would in C.

```lua
local settings = FS.new_fluid_settings()
local synth = FS.new_fluid_synth(settings)
local audiodriver = FS.new_fluid_audio_driver(settings, synth)
local player = FS.new_fluid_player(synth)

FS.fluid_synth_sfload(synth, "assets/acoustic_piano_imis_1.sf2", 1)
FS.fluid_player_add(player, "assets/ff13-lightnings-theme.mid")

FS.fluid_player_play(player)
FS.fluid_player_join(player)

FS.delete_fluid_audio_driver(audiodriver)
FS.delete_fluid_player(player)
FS.delete_fluid_synth(synth)
FS.delete_fluid_settings(settings)
```

See more examples in the [test](https://github.com/ericnething/fluidsynth/tree/master/test) directory.

A complete midi parser in lua is also provided as [midi_parser.lua](https://github.com/ericnething/fluidsynth/blob/master/test/midi_parser.lua).

## References

+ [Fluidsynth API documentation](http://fluidsynth.sourceforge.net/api/)
+ [Fluidsynth source code](https://sourceforge.net/p/fluidsynth/code-git/ci/master/tree/fluidsynth/)
