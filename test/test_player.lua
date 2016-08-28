local FS = require "cfluidsynth"

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

