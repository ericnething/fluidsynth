#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdlib.h>

#include <fluidsynth.h>

/*-------------------------------------------------------------------
  ---=  Sequencer =---
  ------------------------------------------------------------------*/

/*
 * FLUIDSYNTH_API fluid_sequencer_t *
 * new_fluid_sequencer (void)
 * 
 * Create a new sequencer object which uses the system timer.
 *
 */

static int
c_new_fluid_sequencer (lua_State* L)
{
	fluid_sequencer_t* sequencer = new_fluid_sequencer();
	if ((int)sequencer == FLUID_FAILED) { lua_pushnil(L); return 1; }

        fluid_sequencer_t** seq_p = lua_newuserdata(L, sizeof(fluid_sequencer_t**));
        *seq_p = sequencer;
        
        return 1;
}

/*
 *  FLUIDSYNTH_API fluid_sequencer_t *
 *  new_fluid_sequencer2 (int use_system_timer)
 *  
 *  Create a new sequencer object.
 *
 */

static int
c_new_fluid_sequencer2 (lua_State* L)
{
        luaL_checktype(L, 1, LUA_TBOOLEAN);
        int use_system_timer = (int)luaL_checkinteger(L, 1);

        fluid_sequencer_t* sequencer = new_fluid_sequencer2(use_system_timer);
	if ((int)sequencer == FLUID_FAILED) { lua_pushnil(L); return 1; }
        
        fluid_sequencer_t** seq_p = lua_newuserdata(L, sizeof(fluid_sequencer_t**));
        *seq_p = sequencer;
        
        return 1;
}

/*
 *  FLUIDSYNTH_API void
 *  delete_fluid_sequencer (fluid_sequencer_t *seq)
 *  
 *  Free a sequencer object.
 *
 */

static int
c_delete_fluid_sequencer (lua_State* L)
{
        fluid_sequencer_t*  sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        delete_fluid_sequencer(sequencer);
        return 0;
}

/*
 *  FLUIDSYNTH_API int
 *  fluid_sequencer_get_use_system_timer (fluid_sequencer_t *seq)
 *  
 *  Check if a sequencer is using the system timer or not.
 *
 */

static int
c_fluid_sequencer_get_use_system_timer (lua_State* L)
{
        fluid_sequencer_t*  sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
	int use_system_timer = fluid_sequencer_get_use_system_timer(sequencer);
        lua_pushboolean(L, use_system_timer);
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_sequencer_register_client (fluid_sequencer_t *seq,
 *                                  const char *name,
 *                                  fluid_event_callback_t callback,
 *                                  void *data)
 *                                  
 * Register a sequencer client.
 *
 */

struct cbdata {
        lua_State* L;
        int cb_index;
        int data_index;
};

static void
event_callback_wrapper (unsigned int time,
                        fluid_event_t *event,
                        fluid_sequencer_t *seq,
                        void *data)
{
        struct cbdata* cb = (struct cbdata*)data;

        // push callback function onto stack
        lua_rawgeti(cb->L, LUA_REGISTRYINDEX, cb->cb_index);

        // push `time`
        lua_pushinteger(cb->L, time);
        
        // push `event`
        fluid_event_t** event_p = lua_newuserdata(cb->L, sizeof(fluid_event_t*));
        *event_p = event;
        
        // push `seq`
        fluid_sequencer_t** seq_p = lua_newuserdata(cb->L, sizeof(fluid_sequencer_t*));
        *seq_p = seq;
        
        // push data onto stack
        lua_rawgeti(cb->L, LUA_REGISTRYINDEX, cb->data_index);

        // apply the lua callback function
        lua_call(cb->L, 4, 0);
}

static int
c_fluid_sequencer_register_client (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        const char* name = (const char*)luaL_checkstring(L, 2);
        luaL_checktype(L, 3, LUA_TFUNCTION);
        luaL_checktype(L, 4, LUA_TTABLE);

        /*
         * `data` is the 4th parameter (top of stack). `luaL_ref` pops
         * it from the stack.
         *
         */
        int callback_data_index = luaL_ref(L, LUA_REGISTRYINDEX);

        // `callback` is the 3rd parameter (now at top of stack)
        int callback_index = luaL_ref(L, LUA_REGISTRYINDEX);

        struct cbdata* cb = malloc(sizeof(struct cbdata));
        cb->L = L;
        cb->cb_index = callback_index;
        cb->data_index = callback_data_index;
        
        short seqid = fluid_sequencer_register_client(sequencer,
                                                      name,
                                                      event_callback_wrapper,
                                                      (void*)cb);
        
        if ((int)seqid == FLUID_FAILED) { lua_pushnil(L); return 1; }
        
        lua_pushinteger(L, (int)seqid);
        return 1;
}

/*
 * FLUIDSYNTH_API void
 * fluid_sequencer_unregister_client (fluid_sequencer_t *seq,
 *                                    short id)
 *                                    
 * Unregister a previously registered client.
 *
 */

static int
c_fluid_sequencer_unregister_client (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        short client_id = (short)luaL_checkinteger(L, 2);
        
        fluid_sequencer_unregister_client(sequencer, client_id);
        
        return 0;
}

/*
 * FLUIDSYNTH_API int
 * fluid_sequencer_count_clients (fluid_sequencer_t *seq)
 * 
 * Count a sequencers registered clients.
 *
 */

static int
c_fluid_sequencer_count_clients (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
	int num_clients = fluid_sequencer_count_clients(sequencer);
        
        lua_pushinteger(L, num_clients);

        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_sequencer_get_client_id (fluid_sequencer_t *seq,
 *                                int index)
 * 
 * Get a client ID from its index (order in which it was registered). 
 *
 */

static int
c_fluid_sequencer_get_client_id (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        int index = (int)luaL_checkinteger(L, 2);

        short client_id = fluid_sequencer_get_client_id(sequencer, index);
        if ((int)client_id == FLUID_FAILED) { lua_pushnil(L); return 1; }
        
        lua_pushinteger(L, (int)client_id);

        return 1;
}

/*
 * FLUIDSYNTH_API char *
 * fluid_sequencer_get_client_name (fluid_sequencer_t *seq,
 *                                  int id)
 *                                  
 * Get the name of a registered client.
 *
 */

static int
c_fluid_sequencer_get_client_name (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        int client_id = (int)luaL_checkinteger(L, 2);
        
	char* name = fluid_sequencer_get_client_name(sequencer, client_id);
        if (name == NULL) { lua_pushnil(L); return 1; }

        lua_pushstring(L, name);

        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_sequencer_client_is_dest (fluid_sequencer_t *seq,
 *                                 int id)
 * 
 * Check if a client is a destination client.
 *
 */

static int
c_fluid_sequencer_client_is_dest (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        int client_id = (int)luaL_checkinteger(L, 2);

	int is_dest = fluid_sequencer_client_is_dest(sequencer, client_id);

        lua_pushboolean(L, is_dest);

        return 1;
}

/*
 * FLUIDSYNTH_API void
 * fluid_sequencer_process (fluid_sequencer_t *seq,
 *                          unsigned int msec)
 *                          
 * Advance a sequencer that isn't using the system timer.
 *
 */

static int
c_fluid_sequencer_process (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        unsigned int msec = (unsigned int)luaL_checkinteger(L, 2);

        fluid_sequencer_process(sequencer, msec);

        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_sequencer_send_now (fluid_sequencer_t *seq,
 *                           fluid_event_t *evt)
 *                           
 * Send an event immediately.
 *
 */

static int
c_fluid_sequencer_send_now (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        fluid_event_t*  event = *(fluid_event_t**)lua_touserdata(L, 2);
        
	fluid_sequencer_send_now(sequencer, event);
        
        return 0;
}

/*
 * FLUIDSYNTH_API int
 * fluid_sequencer_send_at (fluid_sequencer_t *seq,
 *                          fluid_event_t *evt,
 *                          unsigned int time,
 *                          int absolute)
 *                          
 * Schedule an event for sending at a later time.
 *
 */

static int
c_fluid_sequencer_send_at (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        fluid_event_t*  event = *(fluid_event_t**)lua_touserdata(L, 2);

        unsigned int time = (unsigned int)luaL_checkinteger(L, 3);
        
        luaL_checktype(L, 4, LUA_TBOOLEAN);
        int is_absolute = (int)lua_tointeger(L, 4);
        
	int status = fluid_sequencer_send_at(sequencer, event, time, is_absolute);
        if (status == FLUID_FAILED) { lua_pushnil(L); return 1; }
        
        lua_pushinteger(L, FLUID_OK);
        return 1;
}

/*
 * FLUIDSYNTH_API void
 * fluid_sequencer_remove_events (fluid_sequencer_t *seq,
 *                                short source,
 *                                short dest,
 *                                int type)
 *                                
 * Remove events from the event queue.
 *
 */

static int
c_fluid_sequencer_remove_events (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        short source = (short)luaL_checkinteger(L, 2);
        short dest = (short)luaL_checkinteger(L, 3);
        int type = (int)luaL_checkinteger(L, 4);
        
        fluid_sequencer_remove_events(sequencer, source, dest, type);
        
        return 0;
}

/*
 * FLUIDSYNTH_API unsigned int
 * fluid_sequencer_get_tick (fluid_sequencer_t *seq)
 * 
 * Get the current tick of a sequencer.
 *
 */

static int
c_fluid_sequencer_get_tick (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        
        unsigned int tick = fluid_sequencer_get_tick(sequencer);

        lua_pushinteger(L, tick);
        return 1;
}

/*
 * FLUIDSYNTH_API void
 * fluid_sequencer_set_time_scale (fluid_sequencer_t *seq,
 *                                 double scale)
 *                                 
 * Set the time scale of a sequencer.
 *
 */

static int
c_fluid_sequencer_set_time_scale (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        double scale = (double)luaL_checknumber(L, 2);

        fluid_sequencer_set_time_scale(sequencer, scale);

        return 0;
}

/*
 * FLUIDSYNTH_API double
 * fluid_sequencer_get_time_scale (fluid_sequencer_t *seq)
 * 
 * Get a sequencer's time scale.
 *
 */

static int
c_fluid_sequencer_get_time_scale (lua_State* L)
{
        fluid_sequencer_t* sequencer = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        
        double time_scale = fluid_sequencer_get_time_scale(sequencer);

        lua_pushnumber(L, time_scale);
        return 1;
}

/*-------------------------------------------------------------------
  ---=  Sequencer Bind =---
  ------------------------------------------------------------------*/

/*
 * FLUIDSYNTH_API short
 * fluid_sequencer_register_fluidsynth (fluid_sequencer_t *seq,
 *                                      fluid_synth_t *synth)
 *
 * Registers a synthesizer as a destination client of the given
 * sequencer.
 *
 */

static int
c_fluid_sequencer_register_fluidsynth (lua_State* L)
{
        fluid_sequencer_t* seq = *(fluid_sequencer_t**)lua_touserdata(L, 1);
        fluid_synth_t* synth = *(fluid_synth_t**)lua_touserdata(L, 2);

        short dest = fluid_sequencer_register_fluidsynth(seq, synth);
        if ((int)dest == FLUID_FAILED) { lua_pushnil(L); return 1; }

        lua_pushinteger(L, dest);
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_sequencer_add_midi_event_to_buffer (void *data,
 *                                           fluid_midi_event_t *event)
 *
 * Transforms an incoming midi event (from a midi driver or midi
 * router) to a sequencer event and adds it to the sequencer queue for
 * sending as soon as possible.
 *
 */

static int
c_fluid_sequencer_add_midi_event_to_buffer (lua_State* L)
{
        void* data = *(void**)lua_touserdata(L, 1);
        fluid_midi_event_t* event = *(fluid_midi_event_t**)lua_touserdata(L, 2);

        int status = fluid_sequencer_add_midi_event_to_buffer(data, event);
        if (status == FLUID_FAILED) { lua_pushnil(L); return 1; }

        lua_pushinteger(L, FLUID_OK);
        return 1;
}

/*-------------------------------------------------------------------
  ---=  Events =---
  ------------------------------------------------------------------*/

static int
gc_delete_fluid_event (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        delete_fluid_event(event);
        return 0;
}

/*
 * FLUIDSYNTH_API fluid_event_t *
 * new_fluid_event (void)
 *
 * Create a new sequencer event structure.
 *
 */

static int
c_new_fluid_event (lua_State* L)
{
        fluid_event_t* event = new_fluid_event();
        if (event == NULL) { lua_pushnil(L); return 1; }
        
        fluid_event_t** event_p = lua_newuserdata(L, sizeof(fluid_event_t*));
        *event_p = event;

        luaL_newmetatable(L, "fluid.event");
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, gc_delete_fluid_event);
        lua_settable(L, -3);

        lua_pop(L, 1); // remove the metatable from the stack
        
        return 1;
}

/*
 * FLUIDSYNTH_API void
 * delete_fluid_event (fluid_event_t *evt)
 *
 * Delete a sequencer event structure.
 *
 */

static int
c_delete_fluid_event (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        delete_fluid_event(event);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_set_source (fluid_event_t *evt,
 *                         short src)
 *
 * Set source of a sequencer event (DOCME).
 *
 */

static int
c_fluid_event_set_source (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        short src = (short)luaL_checkinteger(L, 2);

        fluid_event_set_source(event, src);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_set_dest (fluid_event_t *evt,
 *                       short dest)
 *
 * Set destination of a sequencer event (DOCME).
 *
 */

static int
c_fluid_event_set_dest (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        short dest = (short)luaL_checkinteger(L, 2);

        fluid_event_set_dest(event, dest);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_timer (fluid_event_t *evt,
 *                    void *data)
 *
 * Set a sequencer event to be a timer event.
 *
 */

static int
c_fluid_event_timer (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }
        
        // Just to satisfy the type system, we need some pointer to
        // take the place of void* data
        void* data;
        
        fluid_event_timer(event, (void*)data);
        
        return 0;
}


/*
 * FLUIDSYNTH_API void
 * fluid_event_note (fluid_event_t *evt,
 *                   int channel,
 *                   short key,
 *                   short vel,
 *                   unsigned int duration)
 *
 * Set a sequencer event to be a note duration event.
 *
 */

static int
c_fluid_event_note (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int channel = (int)luaL_checkinteger(L, 2);
        short key = (short)luaL_checkinteger(L, 3);
        short vel = (short)luaL_checkinteger(L, 4);
        unsigned int duration = (unsigned int)luaL_checkinteger(L, 5);

        fluid_event_note(event, channel, key, vel, duration);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_noteon (fluid_event_t *evt,
 *                     int channel,
 *                     short key,
 *                     short vel)
 *
 * Set a sequencer event to be a note on event.
 *
 */

static int
c_fluid_event_noteon (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int channel = (int)luaL_checkinteger(L, 2);
        short key = (short)luaL_checkinteger(L, 3);
        short vel = (short)luaL_checkinteger(L, 4);

        fluid_event_noteon(event, channel, key, vel);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_noteoff (fluid_event_t *evt,
 *                      int channel,
 *                      short key)
 * 
 * Set a sequencer event to be a note off event.
 *
 */

static int
c_fluid_event_noteoff (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int channel = (int)luaL_checkinteger(L, 2);
        short key = (short)luaL_checkinteger(L, 3);

        fluid_event_noteoff(event, channel, key);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_all_sounds_off (fluid_event_t *evt,
 *                             int channel)
 *
 * Set a sequencer event to be an all sounds off event.
 *
 */

static int
c_fluid_event_all_sounds_off (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int channel = (int)luaL_checkinteger(L, 2);

        fluid_event_all_sounds_off(event, channel);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_all_notes_off (fluid_event_t *evt,
 *                            int channel)
 *
 * Set a sequencer event to be a all notes off event.
 *
 */

static int
c_fluid_event_all_notes_off (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int channel = (int)luaL_checkinteger(L, 2);

        fluid_event_all_notes_off(event, channel);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_bank_select (fluid_event_t *evt,
 *                          int channel,
 *                          short bank_num)
 *
 * Set a sequencer event to be a bank select event.
 *
 */

static int
c_fluid_event_bank_select (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int channel = (int)luaL_checkinteger(L, 2);
        short bank_num = (short)luaL_checkinteger(L, 3);

        fluid_event_bank_select(event, channel, bank_num);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_program_change (fluid_event_t *evt,
 *                             int channel,
 *                             short preset_num)
 *
 * Set a sequencer event to be a program change event.
 *
 */

static int
c_fluid_event_program_change (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int channel = (int)luaL_checkinteger(L, 2);
        short preset_num = (short)luaL_checkinteger(L, 3);

        fluid_event_program_change(event, channel, preset_num);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_program_select (fluid_event_t *evt,
 *                             int channel,
 *                             unsigned int sfont_id,
 *                             short bank_num,
 *                             short preset_num)
 *
 * Set a sequencer event to be a program select event.
 *
 */

static int
c_fluid_event_program_select (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int           channel = (int)luaL_checkinteger(L, 2);
        unsigned int sfont_id = (unsigned int)luaL_checkinteger(L, 3);
        short        bank_num = (short)luaL_checkinteger(L, 4);
        short      preset_num = (short)luaL_checkinteger(L, 5);

        fluid_event_program_select(event, channel, sfont_id, bank_num, preset_num);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_control_change (fluid_event_t *evt,
 *                             int channel,
 *                             short control,
 *                             short val)
 *
 * Set a sequencer event to be a MIDI control change event.
 *
 */

static int
c_fluid_event_control_change (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short  control = (short)luaL_checkinteger(L, 3);
        short      val = (short)luaL_checkinteger(L, 4);

        fluid_event_control_change(event, channel, control, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_pitch_bend (fluid_event_t *evt,
 *                         int channel,
 *                         int val)
 *
 * Set a sequencer event to be a pitch bend event.
 *
 */

static int
c_fluid_event_pitch_bend (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_pitch_bend(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_pitch_wheelsens (fluid_event_t *evt,
 *                              int channel,
 *                              short val)
 *
 * Set a sequencer event to be a pitch wheel sensitivity event.
 *
 */

static int
c_fluid_event_pitch_wheelsens (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_pitch_wheelsens(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_modulation (fluid_event_t *evt,
 *                         int channel,
 *                         short val)
 *
 * Set a sequencer event to be a modulation event.
 *
 */

static int
c_fluid_event_modulation (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_modulation(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_sustain (fluid_event_t *evt,
 *                      int channel,
 *                      short val)
 *
 * Set a sequencer event to be a MIDI sustain event.
 *
 */

static int
c_fluid_event_sustain (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_sustain(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_pan (fluid_event_t *evt,
 *                  int channel,
 *                  short val)
 *
 * Set a sequencer event to be a stereo pan event.
 *
 */

static int
c_fluid_event_pan (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_pan(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_volume (fluid_event_t *evt,
 *                     int channel,
 *                     short val)
 *
 * Set a sequencer event to be a volume event.
 *
 */

static int
c_fluid_event_volume (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_volume(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_reverb_send (fluid_event_t *evt,
 *                          int channel,
 *                          short val)
 *
 * Set a sequencer event to be a reverb send event.
 *
 */

static int
c_fluid_event_reverb_send (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_reverb_send(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_chorus_send (fluid_event_t *evt,
 *                          int channel,
 *                          short val)
 *
 * Set a sequencer event to be a chorus send event.
 *
 */

static int
c_fluid_event_chorus_send (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_chorus_send(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_channel_pressure (fluid_event_t *evt,
 *                               int channel,
 *                               short val)
 *
 * Set a sequencer event to be a channel-wide aftertouch event.
 *
 */

static int
c_fluid_event_channel_pressure (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);
        short      val = (short)luaL_checkinteger(L, 3);

        fluid_event_channel_pressure(event, channel, val);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_system_reset (fluid_event_t *evt)
 *
 * Set a sequencer event to be a midi system reset event.
 *
 */

static int
c_fluid_event_system_reset (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        fluid_event_system_reset(event);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_any_control_change (fluid_event_t *evt,
 *                                 int channel)
 *
 * Set a sequencer event to be an any control change event.
 *
 */

static int
c_fluid_event_any_control_change (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int    channel = (int)luaL_checkinteger(L, 2);

        fluid_event_any_control_change(event, channel);
        
        return 0;
}

/*
 * FLUIDSYNTH_API void
 * fluid_event_unregistering (fluid_event_t *evt)
 *
 * Set a sequencer event to be an unregistering event.
 *
 */

static int
c_fluid_event_unregistering (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        fluid_event_unregistering(event);
        
        return 0;
}

/*
 * FLUIDSYNTH_API int
 * fluid_event_get_type (fluid_event_t *evt)
 *
 * Get the event type (fluid_seq_event_type) field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_type (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int type = fluid_event_get_type(event);

        lua_pushinteger(L, type);
        
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_event_get_source (fluid_event_t *evt)
 *
 * Get the source field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_source (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        short src = fluid_event_get_source(event);

        lua_pushinteger(L, src);
        
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_event_get_dest (fluid_event_t *evt)
 *
 * Get the dest field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_dest (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        short dest = fluid_event_get_dest(event);

        lua_pushinteger(L, dest);
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_event_get_channel (fluid_event_t *evt)
 *
 * Get the MIDI channel field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_channel (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int channel = fluid_event_get_channel(event);

        lua_pushinteger(L, channel);
        
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_event_get_key (fluid_event_t *evt)
 *
 * Get the MIDI note field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_key (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        short key = fluid_event_get_key(event);

        lua_pushinteger(L, key);
        
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_event_get_velocity (fluid_event_t *evt)
 *
 * Get the MIDI velocity field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_velocity (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        short vel = fluid_event_get_velocity(event);

        lua_pushinteger(L, vel);
        
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_event_get_control (fluid_event_t *evt)
 *
 * Get the MIDI control number field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_control (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        short control = fluid_event_get_control(event);

        lua_pushinteger(L, control);
        
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_event_get_value (fluid_event_t *evt)
 *
 * Get the value field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_value (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        short val = fluid_event_get_value(event);

        lua_pushinteger(L, val);
        
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_event_get_program (fluid_event_t *evt)
 *
 * Get the MIDI program field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_program (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        short program = fluid_event_get_program(event);

        lua_pushinteger(L, program);
        
        return 1;
}

/*
 * FLUIDSYNTH_API void *
 * fluid_event_get_data (fluid_event_t *evt)
 *
 * Get the data field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_data (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        void* data = fluid_event_get_data(event);

        void** data_p = lua_newuserdata(L, sizeof(void*));
        *data_p = data;
                
        return 1;
}

/*
 * FLUIDSYNTH_API unsigned int
 * fluid_event_get_duration (fluid_event_t *evt)
 *
 * Get the duration field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_duration (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        unsigned int duration = fluid_event_get_duration(event);

        lua_pushinteger(L, duration);
        
        return 1;
}

/*
 * FLUIDSYNTH_API short
 * fluid_event_get_bank (fluid_event_t *evt)
 *
 * Get the MIDI bank field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_bank (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        short bank = fluid_event_get_bank(event);

        lua_pushinteger(L, bank);
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_event_get_pitch (fluid_event_t *evt)
 *
 * Get the pitch field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_pitch (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        int pitch = fluid_event_get_pitch(event);

        lua_pushinteger(L, pitch);
        
        return 1;
}

/*
 * FLUIDSYNTH_API unsigned int
 * fluid_event_get_sfont_id (fluid_event_t *evt)
 *
 * Get the SoundFont ID field from a sequencer event structure.
 *
 */

static int
c_fluid_event_get_sfont_id (lua_State* L)
{
        fluid_event_t* event = *(fluid_event_t**)lua_touserdata(L, 1);
        if (event == NULL) { lua_pushnil(L); return 1; }

        unsigned int sfont_id = fluid_event_get_sfont_id(event);

        lua_pushinteger(L, sfont_id);
        
        return 1;
}


/*-------------------------------------------------------------------
  ---=  Synth =---
  ------------------------------------------------------------------*/

/*
 * FLUIDSYNTH_API fluid_synth_t *
 * new_fluid_synth (fluid_settings_t *settings)
 *
 * Create new FluidSynth instance.
 *
 */

static int
c_new_fluid_synth (lua_State* L)
{
        fluid_settings_t* settings = *(fluid_settings_t**)lua_touserdata(L, 1);
        if (settings == NULL) { lua_pushnil(L); return 1; }

        fluid_synth_t* synth = new_fluid_synth(settings);
        if (synth == NULL) { lua_pushnil(L); return 1; }

        fluid_synth_t** synth_p = lua_newuserdata(L, sizeof(fluid_synth_t*));
        *synth_p = synth;
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * delete_fluid_synth (fluid_synth_t *synth)
 *
 * Delete a FluidSynth instance.
 *
 */

static int
c_delete_fluid_synth (lua_State* L)
{
        fluid_synth_t* synth = *(fluid_synth_t**)lua_touserdata(L, 1);
        if (synth == NULL) { lua_pushnil(L); return 1; }

        int status = delete_fluid_synth(synth);

        lua_pushinteger(L, status);
        return 1;
}

/*
 * FLUIDSYNTH_API fluid_settings_t * 	fluid_synth_get_settings (fluid_synth_t *synth)
 *
 * Get settings assigned to a synth.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_noteon (fluid_synth_t *synth,
 *                     int chan,
 *                     int key,
 *                     int vel)
 *
 * Send a note-on event to a FluidSynth object.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_noteoff (fluid_synth_t *synth,
 *                      int chan,
 *                      int key)
 *
 * Send a note-off event to a FluidSynth object.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_cc (fluid_synth_t *synth, int chan, int ctrl, int val)
 *
 * Send a MIDI controller event on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_cc (fluid_synth_t *synth,
 *                     int chan,
 *                     int ctrl,
 *                     int *pval)
 *
 * Get current MIDI controller value on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_sysex (fluid_synth_t *synth,
 *                    const char *data,
 *                    int len,
 *                    char *response,
 *                    int *response_len,
 *                    int *handled,
 *                    int dryrun)
 *
 * Process a MIDI SYSEX (system exclusive) message.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_pitch_bend (fluid_synth_t *synth,
 *                         int chan,
 *                         int val)
 *
 * Set the MIDI pitch bend controller value on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_pitch_bend (fluid_synth_t *synth,
 *                             int chan,
 *                             int *ppitch_bend)
 *
 * Get the MIDI pitch bend controller value on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_pitch_wheel_sens (fluid_synth_t *synth,
 *                               int chan,
 *                               int val)
 *
 * Set MIDI pitch wheel sensitivity on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_pitch_wheel_sens (fluid_synth_t *synth,
 *                                   int chan,
 *                                   int *pval)
 *
 * Get MIDI pitch wheel sensitivity on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_program_change (fluid_synth_t *synth,
 *                             int chan,
 *                             int program)
 *
 * Send a program change event on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_channel_pressure (fluid_synth_t *synth,
 *                               int chan,
 *                               int val)
 *
 * Set the MIDI channel pressure controller value.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_bank_select (fluid_synth_t *synth,
 *                          int chan,
 *                          unsigned int bank)
 *
 * Set instrument bank number on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_sfont_select (fluid_synth_t *synth,
 *                           int chan,
 *                           unsigned int sfont_id)
 *
 * Set SoundFont ID on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_program_select (fluid_synth_t *synth,
 *                             int chan,
 *                             unsigned int sfont_id,
 *                             unsigned int bank_num,
 *                             unsigned int preset_num)
 *
 * Select an instrument on a MIDI channel by SoundFont ID, bank and
 * program numbers.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_program_select_by_sfont_name (fluid_synth_t *synth,
 *                                           int chan,
 *                                           const char *sfont_name,
 *                                           unsigned int bank_num,
 *                                           unsigned int preset_num)
 *
 * Select an instrument on a MIDI channel by SoundFont name, bank and
 * program numbers.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_program (fluid_synth_t *synth,
 *                          int chan,
 *                          unsigned int *sfont_id,
 *                          unsigned int *bank_num,
 *                          unsigned int *preset_num)
 *
 * Get current SoundFont ID, bank number and program number for a MIDI
 * channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_unset_program (fluid_synth_t *synth,
 *                            int chan)
 *
 * Set the preset of a MIDI channel to an unassigned state.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_channel_info (fluid_synth_t *synth,
 *                               int chan,
 *                               fluid_synth_channel_info_t *info)
 *
 * Get information on the currently selected preset on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_program_reset (fluid_synth_t *synth)
 *
 * Resend a bank select and a program change for every channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_system_reset (fluid_synth_t *synth)
 *
 * Send MIDI system reset command (big red 'panic' button), turns off
 * notes and resets controllers.
 *
 */

/*
 * FLUIDSYNTH_API fluid_preset_t *
 * fluid_synth_get_channel_preset (fluid_synth_t *synth,
 *                                 int chan)
 *
 * Get active preset on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_start (fluid_synth_t *synth,
 *                    unsigned int id,
 *                    fluid_preset_t *preset,
 *                    int audio_chan,
 *                    int midi_chan,
 *                    int key,
 *                    int vel)
 *
 * Create and start voices using a preset and a MIDI note on event.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_stop (fluid_synth_t *synth,
 *                   unsigned int id)
 *
 * Stop notes for a given note event voice ID.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_sfload (fluid_synth_t *synth,
 *                     const char *filename,
 *                     int reset_presets)
 *
 * Load a SoundFont file (filename is interpreted by SoundFont loaders).
 *
 */

static int
c_fluid_synth_sfload (lua_State* L)
{
        fluid_synth_t* synth = *(fluid_synth_t**)lua_touserdata(L, 1);
        const char* filename = (const char*)luaL_checkstring(L, 2);
        int reset_presets = (int)luaL_checkinteger(L, 3);

        int sfid = fluid_synth_sfload(synth, filename, reset_presets);
        if (sfid == FLUID_FAILED) { lua_pushnil(L); return 1; }

        lua_pushinteger(L, sfid);
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_synth_sfreload (fluid_synth_t *synth,
 *                       unsigned int id)
 *
 * Reload a SoundFont.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_sfunload (fluid_synth_t *synth,
 *                       unsigned int id,
 *                       int reset_presets)
 *
 * Unload a SoundFont.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_add_sfont (fluid_synth_t *synth,
 *                        fluid_sfont_t *sfont)
 *
 * Add a SoundFont.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_remove_sfont (fluid_synth_t *synth,
 *                           fluid_sfont_t *sfont)
 *
 * Remove a SoundFont from the SoundFont stack without deleting it.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_sfcount (fluid_synth_t *synth)
 *
 * Count number of loaded SoundFont files.
 *
 */

/*
 * FLUIDSYNTH_API fluid_sfont_t *
 * fluid_synth_get_sfont (fluid_synth_t *synth,
 *                        unsigned int num)
 *
 * Get SoundFont by index.
 *
 */

/*
 * FLUIDSYNTH_API fluid_sfont_t *
 * fluid_synth_get_sfont_by_id (fluid_synth_t *synth,
 *                              unsigned int id)
 *
 * Get SoundFont by ID.
 *
 */

/*
 * FLUIDSYNTH_API fluid_sfont_t *
 * fluid_synth_get_sfont_by_name (fluid_synth_t *synth,
 *                                const char *name)
 *
 * Get SoundFont by name.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_set_bank_offset (fluid_synth_t *synth,
 *                              int sfont_id,
 *                              int offset)
 *
 * Offset the bank numbers of a loaded SoundFont.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_bank_offset (fluid_synth_t *synth,
 *                              int sfont_id)
 *
 * Get bank offset of a loaded SoundFont.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_set_reverb (fluid_synth_t *synth,
 *                         double roomsize,
 *                         double damping,
 *                         double width,
 *                         double level)
 *
 * Set reverb parameters.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_set_reverb_on (fluid_synth_t *synth,
 *                            int on)
 *
 * Enable or disable reverb effect.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_synth_get_reverb_roomsize (fluid_synth_t *synth)
 *
 * Get reverb room size.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_synth_get_reverb_damp (fluid_synth_t *synth)
 *
 * Get reverb damping.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_synth_get_reverb_level (fluid_synth_t *synth)
 *
 * Get reverb level.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_synth_get_reverb_width (fluid_synth_t *synth)
 *
 * Get reverb width.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_set_chorus (fluid_synth_t *synth,
 *                         int nr,
 *                         double level,
 *                         double speed,
 *                         double depth_ms,
 *                         int type)
 *
 * Set chorus parameters.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_set_chorus_on (fluid_synth_t *synth,
 *                            int on)
 *
 * Enable or disable chorus effect.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_chorus_nr (fluid_synth_t *synth)
 *
 * Get chorus voice number (delay line count) value.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_synth_get_chorus_level (fluid_synth_t *synth)
 *
 * Get chorus level.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_synth_get_chorus_speed_Hz (fluid_synth_t *synth)
 *
 * Get chorus speed in Hz.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_synth_get_chorus_depth_ms (fluid_synth_t *synth)
 *
 * Get chorus depth.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_chorus_type (fluid_synth_t *synth)
 *
 * Get chorus waveform type.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_count_midi_channels (fluid_synth_t *synth)
 *
 * Get the total count of MIDI channels.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_count_audio_channels (fluid_synth_t *synth)
 *
 * Get the total count of audio channels.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_count_audio_groups (fluid_synth_t *synth)
 *
 * Get the total number of allocated audio channels.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_count_effects_channels (fluid_synth_t *synth)
 *
 * Get the total number of allocated effects channels.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_set_sample_rate (fluid_synth_t *synth,
 *                              float sample_rate)
 *
 * Set sample rate of the synth.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_set_gain (fluid_synth_t *synth,
 *                       float gain)
 *
 * Set synth output gain value.
 *
 */

/*
 * FLUIDSYNTH_API float
 * fluid_synth_get_gain (fluid_synth_t *synth)
 *
 * Get synth output gain value.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_set_polyphony (fluid_synth_t *synth,
 *                            int polyphony)
 *
 * Set synthesizer polyphony (max number of voices).
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_polyphony (fluid_synth_t *synth)
 *
 * Get current synthesizer polyphony (max number of voices).
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_active_voice_count (fluid_synth_t *synth)
 *
 * Get current number of active voices.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_get_internal_bufsize (fluid_synth_t *synth)
 *
 * Get the internal synthesis buffer size value.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_set_interp_method (fluid_synth_t *synth,
 *                                int chan,
 *                                int interp_method)
 *
 * Set synthesis interpolation method on one or all MIDI channels.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_set_gen (fluid_synth_t *synth,
 *                      int chan,
 *                      int param,
 *                      float value)
 *
 * Set a SoundFont generator (effect) value on a MIDI channel in
 * real-time.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_set_gen2 (fluid_synth_t *synth,
 *                       int chan,
 *                       int param,
 *                       float value,
 *                       int absolute,
 *                       int normalized)
 *
 * Set a SoundFont generator (effect) value on a MIDI channel in
 * real-time.
 *
 */

/*
 * FLUIDSYNTH_API float
 * fluid_synth_get_gen (fluid_synth_t *synth,
 *                      int chan,
 *                      int param)
 *
 * Get generator value assigned to a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_create_key_tuning (fluid_synth_t *synth,
 *                                int bank,
 *                                int prog,
 *                                const char *name,
 *                                const double *pitch)
 *
 * Set the tuning of the entire MIDI note scale.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_activate_key_tuning (fluid_synth_t *synth,
 *                                  int bank,
 *                                  int prog,
 *                                  const char *name,
 *                                  const double *pitch,
 *                                  int apply)
 *
 * Set the tuning of the entire MIDI note scale.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_create_octave_tuning (fluid_synth_t *synth,
 *                                   int bank,
 *                                   int prog,
 *                                   const char *name,
 *                                   const double *pitch)
 *
 * Apply an octave tuning to every octave in the MIDI note scale.
 *
 */


/*
 * FLUIDSYNTH_API int
 * fluid_synth_activate_octave_tuning (fluid_synth_t *synth,
 *                                     int bank,
 *                                     int prog,
 *                                     const char *name,
 *                                     const double *pitch,
 *                                     int apply)
 *
 * Activate an octave tuning on every octave in the MIDI note scale.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_tune_notes (fluid_synth_t *synth,
 *                         int bank,
 *                         int prog,
 *                         int len,
 *                         const int *keys,
 *                         const double *pitch,
 *                         int apply)
 *
 * Set tuning values for one or more MIDI notes for an existing
 * tuning.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_select_tuning (fluid_synth_t *synth,
 *                            int chan,
 *                            int bank,
 *                            int prog)
 *
 * Select a tuning scale on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_activate_tuning (fluid_synth_t *synth,
 *                              int chan,
 *                              int bank,
 *                              int prog,
 *                              int apply)
 *
 * Activate a tuning scale on a MIDI channel.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_reset_tuning (fluid_synth_t *synth,
 *                           int chan)
 *
 * Clear tuning scale on a MIDI channel (set it to the default
 * well-tempered scale).
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_deactivate_tuning (fluid_synth_t *synth,
 *                                int chan,
 *                                int apply)
 *
 * Clear tuning scale on a MIDI channel (use default equal tempered
 * scale).
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_tuning_iteration_start (fluid_synth_t *synth)
 *
 * Start tuning iteration.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_tuning_iteration_next (fluid_synth_t *synth,
 *                                    int *bank,
 *                                    int *prog)
 *
 * Advance to next tuning.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_tuning_dump (fluid_synth_t *synth,
 *                          int bank,
 *                          int prog,
 *                          char *name,
 *                          int len,
 *                          double *pitch)
 *
 * Get the entire note tuning for a given MIDI bank and program.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_synth_get_cpu_load (fluid_synth_t *synth)
 *
 * Get the synth CPU load value.
 *
 */

/*
 * FLUIDSYNTH_API char *
 * fluid_synth_error (fluid_synth_t *synth)
 *
 * Get a textual representation of the last error.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_write_s16 (fluid_synth_t *synth,
 *                        int len,
 *                        void *lout,
 *                        int loff,
 *                        int lincr,
 *                        void *rout,
 *                        int roff,
 *                        int rincr)
 *
 * Synthesize a block of 16 bit audio samples to audio buffers.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_write_float (fluid_synth_t *synth,
 *                          int len,
 *                          void *lout,
 *                          int loff,
 *                          int lincr,
 *                          void *rout,
 *                          int roff,
 *                          int rincr)
 *
 * Synthesize a block of floating point audio samples to audio
 * buffers.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_nwrite_float (fluid_synth_t *synth,
 *                           int len,
 *                           float **left,
 *                           float **right,
 *                           float **fx_left,
 *                           float **fx_right)
 *
 * Synthesize a block of floating point audio to audio buffers.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_process (fluid_synth_t *synth,
 *                      int len,
 *                      int nin,
 *                      float **in,
 *                      int nout,
 *                      float **out)
 *
 * Synthesize floating point audio to audio buffers.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_add_sfloader (fluid_synth_t *synth,
 *                           fluid_sfloader_t *loader)
 *
 * Add a SoundFont loader interface.
 *
 */

/*
 * FLUIDSYNTH_API fluid_voice_t *
 * fluid_synth_alloc_voice (fluid_synth_t *synth,
 *                          fluid_sample_t *sample,
 *                          int channum,
 *                          int key,
 *                          int vel)
 *
 * Allocate a synthesis voice.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_start_voice (fluid_synth_t *synth,
 *                          fluid_voice_t *voice)
 *
 * Activate a voice previously allocated with
 * fluid_synth_alloc_voice().
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_get_voicelist (fluid_synth_t *synth,
 *                            fluid_voice_t *buf[],
 *                            int bufsize,
 *                            int ID)
 *
 * Get list of voices.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_synth_handle_midi_event (void *data,
 *                                fluid_midi_event_t *event)
 *
 * Handle MIDI event from MIDI router, used as a callback function.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_synth_set_midi_router (fluid_synth_t *synth,
 *                              fluid_midi_router_t *router)
 *
 * Assign a MIDI router to a synth.
 *
 */

/*-------------------------------------------------------------------
  ---=  MIDI =---
  ------------------------------------------------------------------*/
/*
 * FLUIDSYNTH_API fluid_midi_event_t *
 * new_fluid_midi_event (void)
 *
 * Create a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * delete_fluid_midi_event (fluid_midi_event_t *event)
 *
 * Delete MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_type (fluid_midi_event_t *evt,
 *                            int type)
 *
 * Set the event type field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_get_type (fluid_midi_event_t *evt)
 *
 * Get the event type field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_channel (fluid_midi_event_t *evt,
 *                               int chan)
 *
 * Set the channel field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_get_channel (fluid_midi_event_t *evt)
 *
 * Get the channel field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_get_key (fluid_midi_event_t *evt)
 *
 * Get the key field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_key (fluid_midi_event_t *evt,
 *                           int key)
 *
 * Set the key field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_get_velocity (fluid_midi_event_t *evt)
 *
 * Get the velocity field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_velocity (fluid_midi_event_t *evt,
 *                                int vel)
 *
 * Set the velocity field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_get_control (fluid_midi_event_t *evt)
 *
 * Get the control number of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_control (fluid_midi_event_t *evt,
 *                               int ctrl)
 *
 * Set the control field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_get_value (fluid_midi_event_t *evt)
 *
 * Get the value field from a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_value (fluid_midi_event_t *evt,
 *                             int val)
 *
 * Set the value field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_get_program (fluid_midi_event_t *evt)
 *
 * Get the program field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_program (fluid_midi_event_t *evt,
 *                               int val)
 *
 * Set the program field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_get_pitch (fluid_midi_event_t *evt)
 *
 * Get the pitch field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_pitch (fluid_midi_event_t *evt,
 *                             int val)
 *
 * Set the pitch field of a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_event_set_sysex (fluid_midi_event_t *evt,
 *                             void *data,
 *                             int size,
 *                             int dynamic)
 *
 * Assign sysex data to a MIDI event structure.
 *
 */

/*
 * FLUIDSYNTH_API fluid_midi_router_t *
 * new_fluid_midi_router (fluid_settings_t *settings,
 *                        handle_midi_event_func_t handler,
 *                        void *event_handler_data)
 *
 * Create a new midi router.
 *
 */

/*
 * FLUIDSYNTH_API int
 * delete_fluid_midi_router (fluid_midi_router_t *handler)
 *
 * Delete a MIDI router instance.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_router_set_default_rules (fluid_midi_router_t *router)
 *
 * Set a MIDI router to use default "unity" rules.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_router_clear_rules (fluid_midi_router_t *router)
 *
 * Clear all rules in a MIDI router.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_router_add_rule (fluid_midi_router_t *router,
 *                             fluid_midi_router_rule_t *rule,
 *                             int type)
 *
 * Add a rule to a MIDI router.
 *
 */

/*
 * FLUIDSYNTH_API fluid_midi_router_rule_t *
 * new_fluid_midi_router_rule (void)
 *
 * Create a new MIDI router rule.
 *
 */

/*
 * FLUIDSYNTH_API void
 * delete_fluid_midi_router_rule (fluid_midi_router_rule_t *rule)
 *
 * Free a MIDI router rule.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_midi_router_rule_set_chan (fluid_midi_router_rule_t *rule,
 *                                  int min,
 *                                  int max,
 *                                  float mul,
 *                                  int add)
 *
 * Set the channel portion of a rule.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_midi_router_rule_set_param1 (fluid_midi_router_rule_t *rule,
 *                                    int min,
 *                                    int max,
 *                                    float mul,
 *                                    int add)
 *
 * Set the first parameter portion of a rule.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_midi_router_rule_set_param2 (fluid_midi_router_rule_t *rule,
 *                                    int min,
 *                                    int max,
 *                                    float mul,
 *                                    int add)
 *
 * Set the second parameter portion of a rule.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_router_handle_midi_event (void *data,
 *                                      fluid_midi_event_t *event)
 *
 * Handle a MIDI event through a MIDI router instance.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_dump_prerouter (void *data,
 *                            fluid_midi_event_t *event)
 *
 * MIDI event callback function to display event information to
 * stdout.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_midi_dump_postrouter (void *data,
 *                             fluid_midi_event_t *event)
 *
 * MIDI event callback function to display event information to
 * stdout.
 *
 */

/*
 * FLUIDSYNTH_API fluid_midi_driver_t *
 * new_fluid_midi_driver (fluid_settings_t *settings,
 *                        handle_midi_event_func_t handler,
 *                        void *event_handler_data)
 *
 * Create a new MIDI driver instance.
 *
 */

/*
 * FLUIDSYNTH_API void
 * delete_fluid_midi_driver (fluid_midi_driver_t *driver)
 *
 * Delete a MIDI driver instance.
 *
 */

/*
 * FLUIDSYNTH_API fluid_player_t *
 * new_fluid_player (fluid_synth_t *synth)
 *
 * Create a new MIDI player.
 *
 */

static int
c_new_fluid_player (lua_State* L)
{
        fluid_synth_t* synth = *(fluid_synth_t**)lua_touserdata(L, 1);

        fluid_player_t* player = new_fluid_player(synth);
        if (player == NULL) { lua_pushnil(L); return 1; }

        fluid_player_t** player_p = lua_newuserdata(L, sizeof(fluid_player_t*));
        *player_p = player;
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * delete_fluid_player (fluid_player_t *player)
 *
 * Delete a MIDI player instance.
 *
 */

static int
c_delete_fluid_player (lua_State* L)
{
        fluid_player_t* player = *(fluid_player_t**)lua_touserdata(L, 1);

        int status = delete_fluid_player(player);

        lua_pushinteger(L, status);
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_player_add (fluid_player_t *player,
 *                   const char *midifile)
 *
 * Add a MIDI file to a player queue.
 *
 */

static int
c_fluid_player_add (lua_State* L)
{
        fluid_player_t* player = *(fluid_player_t**)lua_touserdata(L, 1);
        const char* midifile = (const char*)luaL_checkstring(L, 2);

        int status = fluid_player_add(player, midifile);

        lua_pushinteger(L, status);
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_player_play (fluid_player_t *player)
 *
 * Activates play mode for a MIDI player if not already playing.
 *
 */

static int
c_fluid_player_play (lua_State* L)
{
        fluid_player_t* player = *(fluid_player_t**)lua_touserdata(L, 1);

        int status = fluid_player_play(player);
        if (status == FLUID_FAILED) { lua_pushnil(L); return 1; }

        lua_pushinteger(L, status);
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_player_stop (fluid_player_t *player)
 *
 * Stops a MIDI player.
 *
 */

static int
c_fluid_player_stop (lua_State* L)
{
        fluid_player_t* player = *(fluid_player_t**)lua_touserdata(L, 1);

        int status = fluid_player_stop(player);

        lua_pushinteger(L, status);
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_player_join (fluid_player_t *player)
 *
 * Wait for a MIDI player to terminate (when done playing).
 *
 */

static int
c_fluid_player_join (lua_State* L)
{
        fluid_player_t* player = *(fluid_player_t**)lua_touserdata(L, 1);

        int status = fluid_player_join(player);
        if (status == FLUID_FAILED) { lua_pushnil(L); return 1; }

        lua_pushinteger(L, status);
        
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_player_set_loop (fluid_player_t *player,
 *                        int loop)
 *
 * Enable looping of a MIDI player.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_player_set_midi_tempo (fluid_player_t *player,
 *                              int tempo)
 *
 * Set the tempo of a MIDI player.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_player_set_bpm (fluid_player_t *player,
 *                       int bpm)
 *
 * Set the tempo of a MIDI player in beats per minute.
 *
 */

 /*
 * FLUIDSYNTH_API int
 * fluid_player_get_status (fluid_player_t *player)
 *
 * Get MIDI player status.
 *
 */

static int
c_fluid_player_get_status (lua_State* L)
{
        fluid_player_t* player = *(fluid_player_t**)lua_touserdata(L, 1);

        enum fluid_player_status status = fluid_player_get_status(player);

        lua_pushinteger(L, (int)status);
        
        return 1;
}

/*-------------------------------------------------------------------
  ---=  Settings =---
  ------------------------------------------------------------------*/

/*
 * FLUIDSYNTH_API fluid_settings_t *
 * new_fluid_settings (void)
 * 
 * Create a new settings object.
 * 
 */

static int
c_new_fluid_settings (lua_State* L)
{
        fluid_settings_t* settings = new_fluid_settings();
        if (settings == NULL) { lua_pushnil(L); return 1; }
        
        fluid_settings_t** settings_p = lua_newuserdata(L, sizeof(fluid_settings_t*));
        *settings_p = settings;

        return 1;
}

/*
 * FLUIDSYNTH_API void
 * delete_fluid_settings (fluid_settings_t *settings)
 *
 * Delete the provided settings object.
 *
 */

static int
c_delete_fluid_settings (lua_State* L)
{
        fluid_settings_t* settings = *(fluid_settings_t**)lua_touserdata(L, 1);
        delete_fluid_settings(settings);
        return 0;
}

/*
 * FLUIDSYNTH_API int
 * fluid_settings_get_type (fluid_settings_t *settings,
 *                          const char *name)
 *
 * Get the type of the setting with the given name.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_get_hints (fluid_settings_t *settings,
 *                           const char *name)
 *
 * Get the hints for the named setting as an integer bitmap.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_is_realtime (fluid_settings_t *settings,
 *                             const char *name)
 *
 * Ask whether the setting is changeable in real-time.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_setstr (fluid_settings_t *settings,
 *                        const char *name,
 *                        const char *str)
 *
 * Set a string value for a named setting.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_copystr (fluid_settings_t *settings,
 *                         const char *name,
 *                         char *str,
 *                         int len)
 *
 * Copy the value of a string setting.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_dupstr (fluid_settings_t *settings,
 *                        const char *name,
 *                        char **str)
 *
 * Duplicate the value of a string setting.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_getstr (fluid_settings_t *settings,
 *                        const char *name,
 *                        char **str)
 *
 * Get the value of a string setting.
 *
 */

/*
 * FLUIDSYNTH_API char *
 * fluid_settings_getstr_default (fluid_settings_t *settings,
 *                                const char *name)
 *
 * Get the default value of a string setting.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_str_equal (fluid_settings_t *settings,
 *                           const char *name,
 *                           const char *value)
 *
 * Test a string setting for some value.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_setnum (fluid_settings_t *settings,
 *                        const char *name,
 *                        double val)
 *
 * Set a numeric value for a named setting.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_getnum (fluid_settings_t *settings,
 *                        const char *name,
 *                        double *val)
 *
 * Get the numeric value of a named setting.
 *
 */

/*
 * FLUIDSYNTH_API double
 * fluid_settings_getnum_default (fluid_settings_t *settings,
 *                                const char *name)
 *
 * Get the default value of a named numeric (double) setting.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_settings_getnum_range (fluid_settings_t *settings,
 *                              const char *name,
 *                              double *min,
 *                              double *max)
 *
 * Get the range of values of a numeric setting.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_setint (fluid_settings_t *settings,
 *                        const char *name,
 *                        int val)
 *
 * Set an integer value for a setting.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_getint (fluid_settings_t *settings,
 *                        const char *name,
 *                        int *val)
 *
 * Get an integer value setting.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_getint_default (fluid_settings_t *settings,
 *                                const char *name)
 *
 * Get the default value of an integer setting.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_settings_getint_range (fluid_settings_t *settings,
 *                              const char *name,
 *                              int *min,
 *                              int *max)
 *
 * Get the range of values of an integer setting.
 *
 */

/*
 * FLUIDSYNTH_API void
 * fluid_settings_foreach_option (fluid_settings_t *settings,
 *                                const char *name,
 *                                void *data,
 *                                fluid_settings_foreach_option_t func)
 *
 * Iterate the available options for a named string setting, calling
 * the provided callback function for each existing option.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_settings_option_count (fluid_settings_t *settings,
 *                              const char *name)
 * 
 * Count option string values for a string setting.
 * 
 */

/*
 * FLUIDSYNTH_API char *
 * fluid_settings_option_concat (fluid_settings_t *settings,
 *                               const char *name,
 *                               const char *separator)
 *
 * Concatenate options for a string setting together with a separator
 * between.
 * 
 */

/*
 * FLUIDSYNTH_API void
 * fluid_settings_foreach (fluid_settings_t *settings,
 *                         void *data,
 *                         fluid_settings_foreach_t func)
 *
 * Iterate the existing settings defined in a settings object, calling
 * the provided callback function for each setting.
 *
 */


/*-------------------------------------------------------------------
  ---=  Audio =---
  ------------------------------------------------------------------*/

/*
 * FLUIDSYNTH_API fluid_audio_driver_t *
 * new_fluid_audio_driver (fluid_settings_t *settings,
 *                         fluid_synth_t *synth)
 *
 * Create a new audio driver.
 *
 */

static int
c_new_fluid_audio_driver (lua_State* L)
{
        fluid_settings_t* settings = *(fluid_settings_t**)lua_touserdata(L, 1);
        if (settings == NULL) { lua_pushnil(L); return 1; }

        fluid_synth_t* synth = *(fluid_synth_t**)lua_touserdata(L, 2);
        if (synth == NULL) { lua_pushnil(L); return 1; }

        fluid_audio_driver_t** driver_p =
                lua_newuserdata(L, sizeof(fluid_audio_driver_t*));
        *driver_p = new_fluid_audio_driver(settings, synth);
        
        return 1;
}

/*
 * FLUIDSYNTH_API fluid_audio_driver_t *
 * new_fluid_audio_driver2 (fluid_settings_t *settings,
 *                          fluid_audio_func_t func,
 *                          void *data)
 *
 * Create a new audio driver.
 *
 */

/*
 * FLUIDSYNTH_API void
 * delete_fluid_audio_driver (fluid_audio_driver_t *driver)
 *
 * Deletes an audio driver instance.
 *
 */

static int
c_delete_fluid_audio_driver (lua_State* L)
{
        fluid_audio_driver_t* driver = *(fluid_audio_driver_t**)lua_touserdata(L, 1);
        if (driver == NULL) { lua_pushnil(L); return 1; }

        delete_fluid_audio_driver(driver);
        
        return 0;
}

/*
 * FLUIDSYNTH_API fluid_file_renderer_t *
 * new_fluid_file_renderer (fluid_synth_t *synth)
 *
 * Create a new file renderer and open the file.
 *
 */

/*
 * FLUIDSYNTH_API int
 * fluid_file_renderer_process_block (fluid_file_renderer_t *dev)
 *
 * Write period_size samples to file.
 *
 */

/*
 * FLUIDSYNTH_API void
 * delete_fluid_file_renderer (fluid_file_renderer_t *dev)
 *
 * Close file and destroy a file renderer object.
 *
 */

/*-------------------------------------------------------------------
  ---=  Misc Utilities =---
  ------------------------------------------------------------------*/

/*
 * FLUIDSYNTH_API int
 * fluid_is_soundfont (const char *filename)
 *
 * Check if a file is a SoundFont file.
 *
 */

static int
c_fluid_is_soundfont(lua_State* L)
{
        const char* filename = lua_tostring(L, 1);
        lua_pushboolean(L, fluid_is_soundfont(filename));
        return 1;
}

/*
 * FLUIDSYNTH_API int
 * fluid_is_midifile (const char *filename)
 *
 * Check if a file is a MIDI file.
 *
 */

static int
c_fluid_is_midifile(lua_State* L)
{
        const char* filename = lua_tostring(L, 1);
        lua_pushboolean(L, fluid_is_midifile(filename));
        return 1;
}

/*-------------------------------------------------------------------
  ---=  Export =---
  ------------------------------------------------------------------*/

static const struct luaL_Reg mlib[] = {
        /* Settings */
        {"new_fluid_settings",            c_new_fluid_settings },
        {"delete_fluid_settings",         c_delete_fluid_settings },

        /* Synth */
        {"new_fluid_synth",    c_new_fluid_synth },
        {"delete_fluid_synth", c_delete_fluid_synth },
        {"fluid_synth_sfload", c_fluid_synth_sfload },
        
        /* Audio */
        {"new_fluid_audio_driver",    c_new_fluid_audio_driver },
        {"delete_fluid_audio_driver", c_delete_fluid_audio_driver },

        /* Midi */
        {"new_fluid_player",          c_new_fluid_player },
        {"delete_fluid_player",       c_delete_fluid_player },
        {"fluid_player_add",          c_fluid_player_add },
        {"fluid_player_play",         c_fluid_player_play },
        {"fluid_player_stop",         c_fluid_player_stop },
        {"fluid_player_join",         c_fluid_player_join },
        {"fluid_player_get_status",   c_fluid_player_get_status },
        
        /* Sequencer */
        {"new_fluid_sequencer",                  c_new_fluid_sequencer },
        {"new_fluid_sequencer2",                 c_new_fluid_sequencer2 },
        {"delete_fluid_sequencer",               c_delete_fluid_sequencer },
        {"fluid_sequencer_get_use_system_timer", c_fluid_sequencer_get_use_system_timer },
        {"fluid_sequencer_register_client",      c_fluid_sequencer_register_client },
        {"fluid_sequencer_unregister_client",    c_fluid_sequencer_unregister_client },
        {"fluid_sequencer_count_clients",        c_fluid_sequencer_count_clients },
        {"fluid_sequencer_get_client_id",        c_fluid_sequencer_get_client_id },
        {"fluid_sequencer_get_client_name",      c_fluid_sequencer_get_client_name },
        {"fluid_sequencer_client_is_dest",       c_fluid_sequencer_client_is_dest },
        {"fluid_sequencer_process",              c_fluid_sequencer_process },
        {"fluid_sequencer_send_now",             c_fluid_sequencer_send_now },
        {"fluid_sequencer_send_at",              c_fluid_sequencer_send_at },
        {"fluid_sequencer_remove_events",        c_fluid_sequencer_remove_events },
        {"fluid_sequencer_get_tick",             c_fluid_sequencer_get_tick },
        {"fluid_sequencer_set_time_scale",       c_fluid_sequencer_set_time_scale },
        {"fluid_sequencer_get_time_scale",       c_fluid_sequencer_get_time_scale },

        /* Sequencer Bind */
        {"fluid_sequencer_add_midi_event_to_buffer", c_fluid_sequencer_add_midi_event_to_buffer},
        {"fluid_sequencer_register_fluidsynth", c_fluid_sequencer_register_fluidsynth},
        
        /* Events */
        {"new_fluid_event",                c_new_fluid_event },
        {"delete_fluid_event",             c_delete_fluid_event },
        {"fluid_event_set_source",         c_fluid_event_set_source },
        {"fluid_event_set_dest",           c_fluid_event_set_dest },
        {"fluid_event_timer",              c_fluid_event_timer },
        {"fluid_event_note",               c_fluid_event_note },
        {"fluid_event_noteon",             c_fluid_event_noteon },
        {"fluid_event_noteoff",            c_fluid_event_noteoff },
        {"fluid_event_all_sounds_off",     c_fluid_event_all_sounds_off },
        {"fluid_event_all_notes_off",      c_fluid_event_all_notes_off },
        {"fluid_event_bank_select",        c_fluid_event_bank_select },
        {"fluid_event_program_change",     c_fluid_event_program_change },
        {"fluid_event_program_select",     c_fluid_event_program_select },
        {"fluid_event_control_change",     c_fluid_event_control_change },
        {"fluid_event_pitch_bend",         c_fluid_event_pitch_bend },
        {"fluid_event_pitch_wheelsens",    c_fluid_event_pitch_wheelsens },
        {"fluid_event_modulation",         c_fluid_event_modulation },
        {"fluid_event_sustain",            c_fluid_event_sustain },
        {"fluid_event_pan",                c_fluid_event_pan },
        {"fluid_event_volume",             c_fluid_event_volume },
        {"fluid_event_reverb_send",        c_fluid_event_reverb_send },
        {"fluid_event_chorus_send",        c_fluid_event_chorus_send },
        {"fluid_event_channel_pressure",   c_fluid_event_channel_pressure },
        {"fluid_event_system_reset",       c_fluid_event_system_reset },
        {"fluid_event_any_control_change", c_fluid_event_any_control_change },
        {"fluid_event_unregistering",      c_fluid_event_unregistering },
        {"fluid_event_get_type",           c_fluid_event_get_type },
        {"fluid_event_get_source",         c_fluid_event_get_source },
        {"fluid_event_get_dest",           c_fluid_event_get_dest },
        {"fluid_event_get_channel",        c_fluid_event_get_channel },
        {"fluid_event_get_key",            c_fluid_event_get_key },
        {"fluid_event_get_velocity",       c_fluid_event_get_velocity },
        {"fluid_event_get_control",        c_fluid_event_get_control },
        {"fluid_event_get_value",          c_fluid_event_get_value },
        {"fluid_event_get_program",        c_fluid_event_get_program },
        {"fluid_event_get_data",           c_fluid_event_get_data },
        {"fluid_event_get_duration",       c_fluid_event_get_duration },
        {"fluid_event_get_bank",           c_fluid_event_get_bank },
        {"fluid_event_get_pitch",          c_fluid_event_get_pitch },
        {"fluid_event_get_sfont_id",       c_fluid_event_get_sfont_id },

        /* Misc */
        {"fluid_is_soundfont",         c_fluid_is_soundfont },
        {"fluid_is_midifile",          c_fluid_is_midifile },
    
        {NULL, NULL}
};

int
luaopen_cfluidsynth(lua_State* L)
{
        luaL_newlib(L, mlib);
        /* lua_setglobal(L, "mlib"); */
        return 1;
}

