/*
 * This file is part of lem-pulseaudio.
 * Copyright 2012 Emil Renner Berthing
 *
 * lem-pulseaudio is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * lem-pulseaudio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with lem-pulseaudio.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <lem.h>
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/sample.h>
#include <pulse/subscribe.h>
#include <pulse/mainloop-api.h>

#include "mainloop.c"

#define CTX_EVENT_QUEUE_SIZE 8
#define CTX_EVENT_QUEUE_MASK (CTX_EVENT_QUEUE_SIZE - 1)

struct event {
	pa_subscription_event_type_t type;
	uint32_t idx;
};

struct ctx {
	pa_context *handle;
	lua_State *T;
	unsigned int start;
	unsigned int end;
	struct event event[CTX_EVENT_QUEUE_SIZE];
};

static int
ctx_gc(lua_State *T)
{
	struct ctx *ctx = lua_touserdata(T, 1);

	if (ctx->handle) {
		lem_debug("collecting");

		pa_context_disconnect(ctx->handle);
		pa_context_unref(ctx->handle);
		ctx->handle = NULL;
	}

	return 0;
}

static int
ctx_disconnect(lua_State *T)
{
	struct ctx *ctx;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	pa_context_disconnect(ctx->handle);
	pa_context_unref(ctx->handle);
	ctx->handle = NULL;
	lua_pushboolean(T, 1);
	return 1;
}

static void
ctx_success_cb(pa_context *c, int success, void *userdata)
{
	lua_State *T = userdata;

	(void)c;
	lem_debug("success = %d", success);

	lua_pushboolean(T, success);
	lem_queue(T, 1);
}

#include "query.c"
#include "set.c"

static int
ctx_subscribe(lua_State *T)
{
	struct ctx *ctx;
	pa_subscription_mask_t mask;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	mask = luaL_checknumber(T, 2);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	o = pa_context_subscribe(ctx->handle, mask, ctx_success_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

static void
ctx_subscribe_event_cb(pa_context *c,
		pa_subscription_event_type_t type, uint32_t idx, void *userdata)
{
	struct ctx *ctx = userdata;
	lua_State *T = ctx->T;

	(void)c;

	if (T) {
		lem_debug("resuming %p", T);
		lua_rawgeti(T, lua_upvalueindex(1),
				(type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) + 1);
		lua_rawgeti(T, lua_upvalueindex(2),
				((type & PA_SUBSCRIPTION_EVENT_TYPE_MASK) >> 4) + 1);
		lua_pushnumber(T, idx);
		lem_queue(T, 3);
		ctx->T = NULL;
	} else {
		unsigned int end = ctx->end;
		struct event *event = &ctx->event[end++];

		ctx->end = end & CTX_EVENT_QUEUE_MASK;
		event->type = type;
		event->idx = idx;

		if (ctx->start == end) {
			lem_debug("dropped event");
			ctx->start = (ctx->start+1) & CTX_EVENT_QUEUE_MASK;
		}
	}
}

static int
ctx_get_event(lua_State *T)
{
	struct ctx *ctx;
	unsigned int start;
	struct event *event;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	if (ctx->T) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	start = ctx->start;
	if (start == ctx->end) {
		ctx->T = T;
		lua_settop(T, 1);
		lem_debug("yielding %p", T);
		return lua_yield(T, 1);
	}

	event = &ctx->event[start++];
	ctx->start = start & CTX_EVENT_QUEUE_MASK;
	lua_rawgeti(T, lua_upvalueindex(1),
			(event->type & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) + 1);
	lua_rawgeti(T, lua_upvalueindex(2),
			((event->type & PA_SUBSCRIPTION_EVENT_TYPE_MASK) >> 4) + 1);
	lua_pushnumber(T, event->idx);
	return 3;
}

static void
ctx_connect_cb(pa_context *c, void *userdata)
{
	lua_State *T = userdata;
	struct ctx *ctx;

	lem_debug("T = %p", T);

	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_UNCONNECTED:
		lem_debug("PA_CONTEXT_UNCONNECTED");
		break;

	case PA_CONTEXT_CONNECTING:
		lem_debug("PA_CONTEXT_CONNECTING");
		break;

	case PA_CONTEXT_AUTHORIZING:
		lem_debug("PA_CONTEXT_AUTHORIZING");
		break;

	case PA_CONTEXT_SETTING_NAME:
		lem_debug("PA_CONTEXT_SETTING_NAME");
		break;

	case PA_CONTEXT_READY:
		lem_debug("PA_CONTEXT_READY");
		pa_context_set_state_callback(c, NULL, NULL);
		ctx = lua_newuserdata(T, sizeof(struct ctx));
		ctx->handle = c;
		ctx->T = NULL;
		ctx->start = ctx->end = 0;
		lua_pushvalue(T, lua_upvalueindex(1));
		lua_setmetatable(T, -2);
		pa_context_set_subscribe_callback(c, ctx_subscribe_event_cb, ctx);
		lem_queue(T, 1);
		break;

	case PA_CONTEXT_FAILED:
		lem_debug("PA_CONTEXT_FAILED");
		pa_context_set_state_callback(c, NULL, NULL);
		lua_pushnil(T);
		lua_pushliteral(T, "failed");
		lem_queue(T, 2);
		break;

	case PA_CONTEXT_TERMINATED:
		lem_debug("PA_CONTEXT_TERMINATED");
		pa_context_set_state_callback(c, NULL, NULL);
		lua_pushnil(T);
		lua_pushliteral(T, "terminated");
		lem_queue(T, 2);
		break;
	}
}

static int
ctx_connect(lua_State *T)
{
	const char *name = luaL_optstring(T, 1, "lem-pulseaudio");
	pa_context *c = pa_context_new(&loop_api, name);

	lua_settop(T, 0);
	pa_context_set_state_callback(c, ctx_connect_cb, T);
	pa_context_connect(c, NULL, 0, NULL);
	return lua_yield(T, 0);
}

static int
ctx_position_name(lua_State *T)
{
	pa_channel_position_t pos;

	if (lua_type(T, 2) != LUA_TNUMBER)
		return 0;

	pos = lua_tonumber(T, 2);
	if (pos > PA_CHANNEL_POSITION_MAX)
		return 0;

	lua_pushstring(T, pa_channel_position_to_string(pos));
	lem_debug("found %s", lua_tostring(T, -1));

	/* insert string so we don't need to be called again */
	lua_pushvalue(T, 2);
	lua_pushvalue(T, -2);
	lua_rawset(T, 1);

	return 1;
}

#define set_mask_constant(L, name) \
	lua_pushnumber(L, PA_SUBSCRIPTION_MASK_##name);\
	lua_setfield(L, -2, #name)
#define set_facility_constant(L, macro, name) \
	lua_pushliteral(L, name);\
	lua_rawseti(L, -2, (PA_SUBSCRIPTION_EVENT_##macro) + 1)
#define set_type_constant(L, macro, name) \
	lua_pushliteral(L, name);\
	lua_rawseti(L, -2, ((PA_SUBSCRIPTION_EVENT_##macro) >> 4) + 1)

int
luaopen_lem_pulseaudio_core(lua_State *L)
{
	/* create module table */
	lua_newtable(L);

	/* create metatable for Context objects */
	lua_newtable(L);
	/* mt.__index = mt */
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	/* mt.__gc = <ctx_gc> */
	lua_pushcfunction(L, ctx_gc);
	lua_setfield(L, -2, "__gc");
	/* mt.disconnect = <ctx_disconnect> */
	lua_pushcfunction(L, ctx_disconnect);
	lua_setfield(L, -2, "disconnect");
	/* mt.server_info = <ctx_server_info> */
	lua_pushcfunction(L, ctx_server_info);
	lua_setfield(L, -2, "server_info");
	/* mt.stat = <ctx_stat> */
	lua_pushcfunction(L, ctx_stat);
	lua_setfield(L, -2, "stat");
	/* mt.sink_info = <ctx_sink_info> */
	lua_pushcfunction(L, ctx_sink_info);
	lua_setfield(L, -2, "sink_info");
	/* mt.source_info = <ctx_source_info> */
	lua_pushcfunction(L, ctx_source_info);
	lua_setfield(L, -2, "source_info");
	/* mt.sink_input_info = <ctx_sink_input_info> */
	lua_pushcfunction(L, ctx_sink_input_info);
	lua_setfield(L, -2, "sink_input_info");
	/* mt.source_output_info = <ctx_source_output_info> */
	lua_pushcfunction(L, ctx_source_output_info);
	lua_setfield(L, -2, "source_output_info");
	/* mt.sample_info = <ctx_sample_info> */
	lua_pushcfunction(L, ctx_sample_info);
	lua_setfield(L, -2, "sample_info");
	/* mt.module_info = <ctx_module_info> */
	lua_pushcfunction(L, ctx_module_info);
	lua_setfield(L, -2, "module_info");
	/* mt.client_info = <ctx_client_info> */
	lua_pushcfunction(L, ctx_client_info);
	lua_setfield(L, -2, "client_info");
	/* mt.set_sink_mute = <ctx_set_sink_mute> */
	lua_pushcfunction(L, ctx_set_sink_mute);
	lua_setfield(L, -2, "set_sink_mute");
	/* mt.set_sink_unmute = <ctx_set_sink_unmute> */
	lua_pushcfunction(L, ctx_set_sink_unmute);
	lua_setfield(L, -2, "set_sink_unmute");
	/* mt.set_sink_volume = <ctx_set_sink_volume> */
	lua_pushcfunction(L, ctx_set_sink_volume);
	lua_setfield(L, -2, "set_sink_volume");
	/* mt.set_sink_port = <ctx_set_sink_port> */
	lua_pushcfunction(L, ctx_set_sink_port);
	lua_setfield(L, -2, "set_sink_port");
	/* mt.set_source_mute = <ctx_set_source_mute> */
	lua_pushcfunction(L, ctx_set_source_mute);
	lua_setfield(L, -2, "set_source_mute");
	/* mt.set_source_unmute = <ctx_set_source_unmute> */
	lua_pushcfunction(L, ctx_set_source_unmute);
	lua_setfield(L, -2, "set_source_unmute");
	/* mt.set_source_volume = <ctx_set_source_volume> */
	lua_pushcfunction(L, ctx_set_source_volume);
	lua_setfield(L, -2, "set_source_volume");
	/* mt.set_source_port = <ctx_set_source_port> */
	lua_pushcfunction(L, ctx_set_source_port);
	lua_setfield(L, -2, "set_source_port");
	/* mt.set_sink_input_mute = <ctx_set_sink_input_mute> */
	lua_pushcfunction(L, ctx_set_sink_input_mute);
	lua_setfield(L, -2, "set_sink_input_mute");
	/* mt.set_sink_input_unmute = <ctx_set_sink_input_unmute> */
	lua_pushcfunction(L, ctx_set_sink_input_unmute);
	lua_setfield(L, -2, "set_sink_input_unmute");
	/* mt.set_sink_input_volume = <ctx_set_sink_input_volume> */
	lua_pushcfunction(L, ctx_set_sink_input_volume);
	lua_setfield(L, -2, "set_sink_input_volume");
	/* mt.kill_sink_input = <ctx_kill_sink_input> */
	lua_pushcfunction(L, ctx_kill_sink_input);
	lua_setfield(L, -2, "kill_sink_input");
	/* mt.set_source_output_mute = <ctx_set_source_output_mute> */
	lua_pushcfunction(L, ctx_set_source_output_mute);
	lua_setfield(L, -2, "set_source_output_mute");
	/* mt.set_source_output_unmute = <ctx_set_source_output_unmute> */
	lua_pushcfunction(L, ctx_set_source_output_unmute);
	lua_setfield(L, -2, "set_source_output_unmute");
	/* mt.set_source_output_volume = <ctx_set_source_output_volume> */
	lua_pushcfunction(L, ctx_set_source_output_volume);
	lua_setfield(L, -2, "set_source_output_volume");
	/* mt.kill_source_output = <ctx_kill_source_output> */
	lua_pushcfunction(L, ctx_kill_source_output);
	lua_setfield(L, -2, "kill_source_output");
	/* mt.load_module = <ctx_load_module> */
	lua_pushcfunction(L, ctx_load_module);
	lua_setfield(L, -2, "load_module");
	/* mt.unload_module = <ctx_unload_module> */
	lua_pushcfunction(L, ctx_unload_module);
	lua_setfield(L, -2, "unload_module");
	/* mt.kill_client = <ctx_kill_client> */
	lua_pushcfunction(L, ctx_kill_client);
	lua_setfield(L, -2, "kill_client");
	/* mt.subscribe = <ctx_subscribe> */
	lua_pushcfunction(L, ctx_subscribe);
	lua_setfield(L, -2, "subscribe");
	/* create facility lookup table */
	lua_createtable(L, 9, 0);
	set_facility_constant(L, SINK,          "sink");
	set_facility_constant(L, SOURCE,        "source");
	set_facility_constant(L, SINK_INPUT,    "sink_input");
	set_facility_constant(L, SOURCE_OUTPUT, "source_output");
	set_facility_constant(L, MODULE,        "module");
	set_facility_constant(L, CLIENT,        "client");
	set_facility_constant(L, SAMPLE_CACHE,  "sample_cache");
	set_facility_constant(L, SERVER,        "server");
	set_facility_constant(L, CARD,          "card");
	/* create type lookup table */
	lua_createtable(L, 3, 0);
	set_type_constant(L, NEW,    "new");
	set_type_constant(L, CHANGE, "change");
	set_type_constant(L, REMOVE, "remove");
	/* mt.get_event = <ctx_get_event> */
	lua_pushcclosure(L, ctx_get_event, 2);
	lua_setfield(L, -2, "get_event");
	/* insert table */
	lua_setfield(L, -2, "Context");

	/* insert connect function */
	lua_getfield(L, -1, "Context"); /* upvalue 1 = Context */
	lua_pushcclosure(L, ctx_connect, 1);
	lua_setfield(L, -2, "connect");

	/* create mask table */
	lua_createtable(L, 0, 11);
	set_mask_constant(L, NULL);
	set_mask_constant(L, SINK);
	set_mask_constant(L, SOURCE);
	set_mask_constant(L, SINK_INPUT);
	set_mask_constant(L, SOURCE_OUTPUT);
	set_mask_constant(L, MODULE);
	set_mask_constant(L, CLIENT);
	set_mask_constant(L, SAMPLE_CACHE);
	set_mask_constant(L, SERVER);
	set_mask_constant(L, CARD);
	set_mask_constant(L, ALL);
	lua_setfield(L, -2, "mask");

	/* create volume table */
	lua_createtable(L, 0, 2);
	lua_pushnumber(L, PA_VOLUME_MUTED);
	lua_setfield(L, -2, "MUTED");
	lua_pushnumber(L, PA_VOLUME_NORM);
	lua_setfield(L, -2, "NORM");
	lua_setfield(L, -2, "volume");

	/* create position_name table */
	lua_createtable(L, 0, 0);
	/* setmetatable(position_name, { __index = <ctx_position_name> }) */
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, ctx_position_name);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "position_name");

	return 1;
}
