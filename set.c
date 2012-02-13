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

/*
 * Helper Functions
 */
static void
ctx_cvolume_check(lua_State *T, int idx, pa_cvolume *cvol)
{
	int len;
	int i;

	luaL_checktype(T, idx, LUA_TTABLE);
	len = lua_rawlen(T, idx);

	for (i = 0; i < len; i++) {
		lua_rawgeti(T, idx, i+1);
		cvol->values[i] = lua_tonumber(T, -1);
		lua_pop(T, 1);
	}

	cvol->channels = len;
}

/*
 * Sinks
 */
static int
ctx__set_sink_mute(lua_State *T, int mute)
{
	struct ctx *ctx;
	int type;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	type = lua_type(T, 2);
	if (lua_gettop(T) < 2 || (type != LUA_TNUMBER && type != LUA_TSTRING))
		return luaL_argerror(T, 2, "expected number or string");
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	switch (type) {
	case LUA_TNUMBER:
		{
			uint32_t idx = lua_tonumber(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_sink_mute_by_index(ctx->handle,
					idx, mute, ctx_success_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_sink_mute_by_name(ctx->handle,
					name, mute, ctx_success_cb, T);
		}
		break;
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

static int
ctx_set_sink_mute(lua_State *T)
{
	return ctx__set_sink_mute(T, 1);
}

static int
ctx_set_sink_unmute(lua_State *T)
{
	return ctx__set_sink_mute(T, 0);
}

static int
ctx_set_sink_volume(lua_State *T)
{
	struct ctx *ctx;
	int type;
	pa_cvolume cvol;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	type = lua_type(T, 2);
	if (lua_gettop(T) < 2 || (type != LUA_TNUMBER && type != LUA_TSTRING))
		return luaL_argerror(T, 2, "expected number or string");
	ctx_cvolume_check(T, 3, &cvol);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	switch (type) {
	case LUA_TNUMBER:
		{
			uint32_t idx = lua_tonumber(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_sink_volume_by_index(ctx->handle,
					idx, &cvol, ctx_success_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_sink_volume_by_name(ctx->handle,
					name, &cvol, ctx_success_cb, T);
		}
		break;
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

static int
ctx_set_sink_port(lua_State *T)
{
	struct ctx *ctx;
	int type;
	const char *port;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	type = lua_type(T, 2);
	if (lua_gettop(T) < 2 || (type != LUA_TNUMBER && type != LUA_TSTRING))
		return luaL_argerror(T, 2, "expected number or string");
	port = luaL_checkstring(T, 3);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	switch (type) {
	case LUA_TNUMBER:
		{
			uint32_t idx = lua_tonumber(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_sink_port_by_index(ctx->handle,
					idx, port, ctx_success_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_sink_port_by_name(ctx->handle,
					name, port, ctx_success_cb, T);
		}
		break;
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Sources
 */
static int
ctx__set_source_mute(lua_State *T, int mute)
{
	struct ctx *ctx;
	int type;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	type = lua_type(T, 2);
	if (lua_gettop(T) < 2 || (type != LUA_TNUMBER && type != LUA_TSTRING))
		return luaL_argerror(T, 2, "expected number or string");
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	switch (type) {
	case LUA_TNUMBER:
		{
			uint32_t idx = lua_tonumber(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_source_mute_by_index(ctx->handle,
					idx, mute, ctx_success_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_source_mute_by_name(ctx->handle,
					name, mute, ctx_success_cb, T);
		}
		break;
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

static int
ctx_set_source_mute(lua_State *T)
{
	return ctx__set_source_mute(T, 1);
}

static int
ctx_set_source_unmute(lua_State *T)
{
	return ctx__set_source_mute(T, 0);
}

static int
ctx_set_source_volume(lua_State *T)
{
	struct ctx *ctx;
	int type;
	pa_cvolume cvol;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	type = lua_type(T, 2);
	if (lua_gettop(T) < 2 || (type != LUA_TNUMBER && type != LUA_TSTRING))
		return luaL_argerror(T, 2, "expected number or string");
	ctx_cvolume_check(T, 3, &cvol);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	switch (type) {
	case LUA_TNUMBER:
		{
			uint32_t idx = lua_tonumber(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_source_volume_by_index(ctx->handle,
					idx, &cvol, ctx_success_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_source_volume_by_name(ctx->handle,
					name, &cvol, ctx_success_cb, T);
		}
		break;
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

static int
ctx_set_source_port(lua_State *T)
{
	struct ctx *ctx;
	int type;
	const char *port;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	type = lua_type(T, 2);
	if (lua_gettop(T) < 2 || (type != LUA_TNUMBER && type != LUA_TSTRING))
		return luaL_argerror(T, 2, "expected number or string");
	port = luaL_checkstring(T, 3);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	switch (type) {
	case LUA_TNUMBER:
		{
			uint32_t idx = lua_tonumber(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_source_port_by_index(ctx->handle,
					idx, port, ctx_success_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_set_source_port_by_name(ctx->handle,
					name, port, ctx_success_cb, T);
		}
		break;
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Sink Inputs
 */

static int
ctx_kill_sink_input(lua_State *T)
{
	struct ctx *ctx;
	uint32_t idx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	idx = luaL_checknumber(T, 2);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	o = pa_context_kill_sink_input(ctx->handle, idx, ctx_success_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Source Outputs
 */

static int
ctx_kill_source_output(lua_State *T)
{
	struct ctx *ctx;
	uint32_t idx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	idx = luaL_checknumber(T, 2);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	o = pa_context_kill_source_output(ctx->handle, idx, ctx_success_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Samples
 */

/*
 * Modules
 */
static void
ctx_load_module_cb(pa_context *c, uint32_t idx, void *userdata)
{
	lua_State *T = userdata;

	(void)c;

	if (idx == PA_INVALID_INDEX)
		lua_pushnil(T);
	else
		lua_pushnumber(T, idx);
	lem_queue(T, 1);
}

static int
ctx_load_module(lua_State *T)
{
	struct ctx *ctx;
	const char *name;
	const char *arg;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	name = luaL_checkstring(T, 2);
	arg = luaL_optstring(T, 3, "");
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	o = pa_context_load_module(ctx->handle, name, arg, ctx_load_module_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

static int
ctx_unload_module(lua_State *T)
{
	struct ctx *ctx;
	uint32_t idx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	idx = luaL_checknumber(T, 2);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	o = pa_context_unload_module(ctx->handle, idx, ctx_success_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Clients
 */

static int
ctx_kill_client(lua_State *T)
{
	struct ctx *ctx;
	uint32_t idx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	idx = luaL_checknumber(T, 2);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	o = pa_context_kill_client(ctx->handle, idx, ctx_success_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}
