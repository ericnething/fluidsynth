local inspect = require "inspect"


-- parse_midi_file : FileHandle -> Midi
function parse_midi_file (filename)
   
   local midi = {}
   local f = io.open(filename, "rb")
   
   midi.header = read_header(f)
   midi.tracks = {}

   -- local current_track = read_track(f)
   
   -- while next(current_track) ~= ni do
   --    midi.tracks[#midi.tracks + 1] = current_track
   --    current_track = read_track(f)
   -- end

   for _ = 1, midi.header.tracks do
      midi.tracks[#midi.tracks + 1] = read_track(f)
   end

   return midi
end

-- read_header : FileHandle -> Header
function read_header(f)

   local header = {}
   
   if f:read(4) == "MThd" then
      local length    = hexstring_to_number({string.byte(f:read(4), 1, 4)})
      header.format   = hexstring_to_number({string.byte(f:read(2), 1, 2)})
      header.tracks   = hexstring_to_number({string.byte(f:read(2), 1, 2)})
      header.division = hexstring_to_number({string.byte(f:read(2), 1, 2)})
      
      if length > 6 then
         f:read(length - 6)
      end
      
      print(inspect(header))

      return header
   else
      f:close()
      error("Not a midi file: header not found")
   end
end

-- read_track : FileHandle -> Track
function read_track(f)

   local track = {}

   if f:read(4) == "MTrk" then -- 4d 54 72 6b
      
      local length = hexstring_to_number({string.byte(f:read(4), 1, 4)})
      local end_of_track = false
      local event, delta_time
      local event_code
      repeat
         -- read delta time
         delta_time = read_delta_time(f)

         -- read event
         event, event_code = read_event(event_code, f)

         -- add delta_time to event
         event.delta_time = delta_time

         -- append event to track
         track[#track + 1] = event

         if event.type == "end_of_track" then
            end_of_track = true
         end
         
      until end_of_track
      
   else
      f:close()
      error("Not a midi file: track not found")
   end
   
   return track
end

-- hexstring_to_number : [Number] -> Number
function hexstring_to_number(s)
   local result = 0
   for _, v in ipairs(s) do
      result = (result << 8) + v
   end
   return result
end

-- read_delta_time : FileHandle -> DeltaTime
function read_delta_time(f)
   local time = 0
   local byte
   repeat
      byte = string.byte(f:read(1))
      -- only use the lower 7 bits
      time = (time << 7) + (byte & 0x7F)
   until byte & 0x80 == 0
   return time
end

-- read_event : FileHandle -> Event
function read_event(previous_event_code, f)

   local next_byte = string.byte(f:read(1))

   if previous_event_code and next_byte >= 0x00 and next_byte <= 0x7F then
      --= Handle running status (re-use previous event code) =--
      
      f:seek("cur", -1)
      
      if previous_event_code >= 0xFF00 then
         --= Meta Event =--
         return read_meta_event(previous_event_code, f)
         
      elseif (previous_event_code & 0xF0FF) >= 0xB078
         and (previous_event_code & 0xF0FF) <= 0xB0FF
      then
         --= Midi Mode Event =--
         return read_midi_mode_event(previous_event_code, f)
         
      else
         --= Midi Voice Event =--
         return read_midi_voice_event(previous_event_code, f)
      end

   elseif next_byte >= 0x80 and next_byte <= 0xEF then
      --= New Event =--
      
      if next_byte >= 0xB0 and next_byte <= 0xBF then
         local next_byte_2 = string.byte(f:read(1))
         
         if next_byte_2 > 0x77 then
            --= Midi Mode Event =--
            local status = (next_byte << 8) + next_byte_2
            return read_midi_mode_event(status, f)

         else
            --= Midi Voice Event =--
            f:seek("cur", -1)
            return read_midi_voice_event(next_byte, f)
         end
         
      else
         --= Midi Voice Event =--
         return read_midi_voice_event(next_byte, f)
      end
      
   elseif next_byte == 0xFF then
      --= Meta Event =--
      local new_status = hexstring_to_number({next_byte, string.byte(f:read(1))})
      return read_meta_event(new_status, f)
      
   else
      error("Midi Event not recognized")
      
   end
end

-- Midi Events
MidiEvent = {
   --= Midi Channel Voice Messages =--
   note_off                = 0x80,
   note_on                 = 0x90,
   polyphonic_key_pressure = 0xA0,
   controller_change       = 0xB0, -- next byte must be 0x00-0x77
   program_change          = 0xC0,
   channel_key_pressure    = 0xD0,
   pitch_bend              = 0xE0,
   
   --= Midi Channel Mode Messages =--
   all_sound_off         = 0xB078,
   reset_all_controllers = 0xB079,
   local_control         = 0xB07A,
   all_notes_off         = 0xB07B,
   omni_mode_off         = 0xB07C,
   omni_mode_on          = 0xB07D,
   mono_mode_on          = 0xB07E,
   poly_mode_on          = 0xB07F,

   --= Meta Events =--
   sequence_number     = 0xFF00,
   text_event          = 0xFF01,
   copyright_notice    = 0xFF02,
   track_name          = 0xFF03,
   instrument_name     = 0xFF04,
   lyric               = 0xFF05,
   marker              = 0xFF06,
   cue_point           = 0xFF07,

   reserved_event_08   = 0xFF08,
   reserved_event_09   = 0xFF09,
   reserved_event_0A   = 0xFF0A,
   reserved_event_0B   = 0xFF0B,
   reserved_event_0C   = 0xFF0C,
   reserved_event_0D   = 0xFF0D,
   reserved_event_0E   = 0xFF0E,
   reserved_event_0F   = 0xFF0F,
   
   midi_channel_prefix = 0xFF20,
   end_of_track        = 0xFF2F,
   set_tempo           = 0xFF51,
   smpte_offset        = 0xFF54,
   time_signature      = 0xFF58,
   key_signature       = 0xFF59,
   sequencer_specific  = 0xFF7F
}


-- read_midi_event : FileHandle -> Event
function read_midi_voice_event(status, f)
   
   local event = {}

   --= Midi Channel Voice Messages =--

   local status_high = status & 0xF0
   local channel     = status & 0x0F
   
   if     status_high == MidiEvent.note_off then
      event.type = "note_off"
      event.channel  = channel
      event.key      = string.byte(f:read(1))
      event.velocity = string.byte(f:read(1))
      
   elseif status_high == MidiEvent.note_on then
      event.type = "note_on"
      event.channel  = channel
      event.key      = string.byte(f:read(1))
      event.velocity = string.byte(f:read(1))

   elseif status_high == MidiEvent.polyphonic_key_pressure then
      event.type = "polyphonic_key_pressure"
      event.channel  = channel
      event.key      = string.byte(f:read(1))
      event.pressure = string.byte(f:read(1))

   elseif status_high == MidiEvent.controller_change then
      event.type = "controller_change"
      event.channel           = channel
      event.controller_number = string.byte(f:read(1))
      event.controller_value  = string.byte(f:read(1))

   elseif status_high == MidiEvent.program_change then
      event.type = "program_change"
      event.channel        = channel
      event.program_number = string.byte(f:read(1))

   elseif status_high == MidiEvent.channel_key_pressure then
      event.type = "channel_key_pressure"
      event.channel  = channel
      event.pressure = string.byte(f:read(1))

   elseif status_high == MidiEvent.pitch_bend then
      event.type = "pitch_bend"
      event.channel = channel
      event.lsb     = string.byte(f:read(1))
      event.msb     = string.byte(f:read(1))

   end
   
   return event, status
end

function read_midi_mode_event(status, f)

   local event = {}
   local mm_status = status & 0xF0FF
   local channel = (status & 0x0F00) >> 8
   
      --= Midi Channel Mode Messages =--
      
   if     mm_status == MidiEvent.all_sound_off then
      event.type = "all_sound_off"
      event.channel = channel
      f:read(1) -- next byte is 0x00
      
   elseif mm_status == MidiEvent.reset_all_controllers then
      event.type = "reset_all_controllers"
      event.channel = channel
      f:read(1) -- next byte is 0x00

   elseif mm_status == MidiEvent.local_control then
      event.type = "local_control"
      event.channel = channel
      event.is_connected = string.byte(f:read(1))

   elseif mm_status == MidiEvent.all_notes_off then
      event.type = "all_notes_off"
      event.channel = channel
      f:read(1) -- next byte is 0x00

   elseif mm_status == MidiEvent.omni_mode_off then
      event.type = "omni_mode_off"
      event.channel = channel
      f:read(1) -- next byte is 0x00

   elseif mm_status == MidiEvent.omni_mode_on then
      event.type = "omni_mode_on"
      event.channel = channel
      f:read(1) -- next byte is 0x00

   elseif mm_status == MidiEvent.mono_mode_on then
      event.type = "mono_mode_on"
      event.channel = channel
      event.num_channels = string.byte(f:read(1))

   elseif mm_status == MidiEvent.poly_mode_on then
      event.type = "poly_mode_on"
      event.channel = channel
      f:read(1) -- next byte is 0x00
      
   end

   return event, status
end

-- read_meta_event : FileHandle -> Event
function read_meta_event(status, f)
   
   -- local status = string.byte(f:read(1))
   local event = {}
   
   if     status == MidiEvent.sequence_number then
      event.type = "sequence_number"
      f:read(1) -- length is 2
      event.sequence_number = hexstring_to_number({string.byte(f:read(2), 1, 2)})
      
   elseif status == MidiEvent.text_event then
      event.type = "text_event"
      local length = string.byte(f:read(1))
      event.text = f:read(length)
      
   elseif status == MidiEvent.copyright_notice then
      event.type = "copyright_notice"
      local length = string.byte(f:read(1))
      event.text = f:read(length)

   elseif status == MidiEvent.track_name then
      event.type = "track_name"
      local length = string.byte(f:read(1))
      event.text = f:read(length)
      
   elseif status == MidiEvent.instrument_name then
      event.type = "instrument_name"
      local length = string.byte(f:read(1))
      event.text = f:read(length)
      
   elseif status == MidiEvent.lyric then
      event.type = "lyric"
      local length = string.byte(f:read(1))
      event.text = f:read(length)
      
   elseif status == MidiEvent.marker then
      event.type = "marker"
      local length = string.byte(f:read(1))
      event.text = f:read(length)
      
   elseif status == MidiEvent.cue_point then
      event.type = "cue_point"
      local length = string.byte(f:read(1))
      event.text = f:read(length)

   elseif status >= 0xFF08 and status <= 0xFF0F then
      event.type = "unassigned_event"
      local length = string.byte(f:read(1))
      event.data = f:read(length)
      
   elseif status == MidiEvent.midi_channel_prefix then
      event.type = "midi_channel_prefix"
      f:read(1) -- length is 1
      event.channel = f:read(1)
      
   elseif status == MidiEvent.end_of_track then
      event.type = "end_of_track"
      f:read(1) -- length is 0
      
   elseif status == MidiEvent.set_tempo then
      event.type = "set_tempo"
      f:read(1) -- length is 3
      event.tempo = hexstring_to_number({string.byte(f:read(3), 1, 3)})
      
   elseif status == MidiEvent.smpte_offset then
      event.type = "smpte_offset"
      f:read(1) -- length is 5
      local hh, mm, ss, fr, ff = string.byte(f:read(5), 1, 5)
      event.hours = hh
      event.minutes = mm
      event.seconds = ss
      event.frames = fr
      event.fractional_frames = ff
      
   elseif status == MidiEvent.time_signature then
      event.type = "time_signature"
      f:read(1) -- length is 4
      local nn, dd, cc, bb = string.byte(f:read(4), 1, 4)
      event.nn = nn
      event.dd = dd
      event.cc = cc
      event.bb = bb
      
   elseif status == MidiEvent.key_signature then
      event.type = "key_signature"
      f:read(1) -- length is 2
      local sf, mi = string.byte(f:read(2), 1, 2)
      event.sf = sf
      event.mi = mi
      
   elseif status == MidiEvent.sequencer_specific then
      event.type = "sequencer_specific"
      local length = string.byte(f:read(1))
      event.data = f:read(length)

   else
      event.type = "raw_meta_event"
      local length = string.byte(f:read(1))
      event.data = f:read(length)
   end
   
   return event, status
end

function main(arg)
   local midi = parse_midi_file(arg[1])
   print(inspect(midi))
end

main(arg)
