local FS = require "cfluidsynth"

local settings
local synth
local audiodriver
local sequencer

local synth_destination
local client_destination

function sequencer_callback (time, event, seq, data)
   print (time)
   send_note (time)
   schedule_timer (time)
end

function send_note (time)
   local event = FS.new_fluid_event()
   FS.fluid_event_set_source (event, -1)
   FS.fluid_event_set_dest (event, synth_destination)
   FS.fluid_event_note (event, 0, 60, 127, 400)
   FS.fluid_sequencer_send_at (sequencer, event, 600, false)
   FS.delete_fluid_event (event)
end

function schedule_timer (time)
   local event = FS.new_fluid_event()
   FS.fluid_event_set_source (event, -1)
   FS.fluid_event_set_dest (event, client_destination)
   FS.fluid_event_timer (event)
   FS.fluid_sequencer_send_at (sequencer, event, 600, false)
   FS.delete_fluid_event (event)
end

function main ()
   settings = FS.new_fluid_settings()
   synth = FS.new_fluid_synth(settings)
   audiodriver = FS.new_fluid_audio_driver(settings, synth)
   sequencer = FS.new_fluid_sequencer()


   synth_destination = FS.fluid_sequencer_register_fluidsynth(sequencer, synth)
   client_destination = FS.fluid_sequencer_register_client(
      sequencer, "test", sequencer_callback, {})

   FS.fluid_synth_sfload(synth, "assets/acoustic_piano_imis_1.sf2", 1)

   local n = nil
   local time = FS.fluid_sequencer_get_tick (sequencer)
   while n == nil do
      send_note (time)
      schedule_timer (time)
      print "Press <Enter> to quit"
      n = io.read()
   end

   FS.delete_fluid_sequencer(sequencer)
   FS.delete_fluid_audio_driver(audiodriver)
   FS.delete_fluid_synth(synth)
   FS.delete_fluid_settings(settings)
end

main()
