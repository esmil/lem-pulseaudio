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
ctx_sample_spec_push(lua_State *T, const pa_sample_spec *spec)
{
	char buf[PA_SAMPLE_SPEC_SNPRINT_MAX];
	pa_sample_spec_snprint(buf, PA_SAMPLE_SPEC_SNPRINT_MAX, spec);
	lua_pushstring(T, buf);
}

static void
ctx_channel_map_push(lua_State *T, const pa_channel_map *cm)
{
	int channels = cm->channels;
	int i = 0;

	lua_createtable(T, channels, 0);

	while (i < channels) {
		lua_pushnumber(T, cm->map[i++]);
		lua_rawseti(T, -2, i);
	}
}

static void
ctx_cvolume_push(lua_State *T, const pa_cvolume *cvol)
{
	int channels = cvol->channels;
	int i = 0;

	lua_createtable(T, channels, 0);

	while (i < channels) {
		lua_pushnumber(T, cvol->values[i++]);
		lua_rawseti(T, -2, i);
	}
}

static void
ctx_proplist_push(lua_State *T, pa_proplist *p)
{
	void *state = NULL;
	const char *key;

	lua_createtable(T, 0, pa_proplist_size(p));

	while ((key = pa_proplist_iterate(p, &state)) != NULL) {
		const char *value = pa_proplist_gets(p, key);

		if (value) {
			lua_pushstring(T, value);
			lua_setfield(T, -2, key);
		}
	}
}

static void
ctx_format_push(lua_State *T, pa_format_info *f)
{
	lua_createtable(T, 0, 2);

	lua_pushnumber(T, f->encoding);
	lua_setfield(T, -2, "encoding");
	if (f->plist) {
		ctx_proplist_push(T, f->plist);
		lua_setfield(T, -2, "proplist");
	}
}

static void
ctx_formats_push(lua_State *T, int formats, pa_format_info **f)
{
	int i = 0;

	lua_createtable(T, formats, 0);
	while (i < formats) {
		ctx_format_push(T, f[i++]);
		lua_rawseti(T, -2, i);
	}
}

/*
 * Sink Info
 */
static void
ctx_sink_info_push(lua_State *T, const pa_sink_info *info)
{
	lua_createtable(T, 0, 20);
	lua_pushstring(T, info->name);
	lua_setfield(T, -2, "name");
	lua_pushnumber(T, info->index);
	lua_setfield(T, -2, "index");
	lua_pushstring(T, info->description);
	lua_setfield(T, -2, "description");
	ctx_sample_spec_push(T, &info->sample_spec);
	lua_setfield(T, -2, "sample_spec");
	ctx_channel_map_push(T, &info->channel_map);
	lua_setfield(T, -2, "channel_map");
	if (info->owner_module != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->owner_module);
		lua_setfield(T, -2, "owner_module");
	}
	ctx_cvolume_push(T, &info->volume);
	lua_setfield(T, -2, "volume");
	lua_pushboolean(T, info->mute);
	lua_setfield(T, -2, "mute");
	lua_pushnumber(T, info->monitor_source);
	lua_setfield(T, -2, "monitor_source");
	lua_pushstring(T, info->monitor_source_name);
	lua_setfield(T, -2, "monitor_source_name");
	lua_pushnumber(T, info->latency);
	lua_setfield(T, -2, "latency");
	lua_pushstring(T, info->driver);
	lua_setfield(T, -2, "driver");
	lua_pushnumber(T, info->flags);
	lua_setfield(T, -2, "flags");
	if (info->proplist) {
		ctx_proplist_push(T, info->proplist);
		lua_setfield(T, -2, "proplist");
	}
	lua_pushnumber(T, info->configured_latency);
	lua_setfield(T, -2, "configured_latency");
	lua_pushnumber(T, info->base_volume);
	lua_setfield(T, -2, "base_volume");
	lua_pushnumber(T, info->n_volume_steps);
	lua_setfield(T, -2, "n_volume_steps");
	if (info->card != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->card);
		lua_setfield(T, -2, "card");
	}
	if (info->ports) {
		uint32_t i = 0;

		lua_createtable(T, info->n_ports, 0);

		while (i < info->n_ports) {
			lua_createtable(T, 0, 4);

			lua_pushstring(T, info->ports[i]->name);
			lua_setfield(T, -2, "name");
			lua_pushstring(T, info->ports[i]->description);
			lua_setfield(T, -2, "description");
			lua_pushnumber(T, info->ports[i]->priority);
			lua_setfield(T, -2, "priority");
			lua_pushboolean(T, info->active_port == info->ports[i]);
			lua_setfield(T, -2, "active");

			lua_rawseti(T, -2, ++i);
		}
		lua_setfield(T, -2, "ports");
	}
	if (info->formats) {
		ctx_formats_push(T, info->n_formats, info->formats);
		lua_setfield(T, -2, "formats");
	}
}

static void
ctx_sink_info_cb(pa_context *c,
		const pa_sink_info *info, int eol, void *userdata)
{
	lua_State *T = userdata;

	(void)c;
	lem_debug("eol = %d, info = %p", eol, info);

	if (info) {
		ctx_sink_info_push(T, info);
		if (lua_gettop(T) > 2)
			lua_rawseti(T, 2, info->index);
	}

	if (eol) {
		if (lua_gettop(T) < 2)
			lua_pushnil(T);
		lem_queue(T, 1);
	}
}

static int
ctx_sink_info(lua_State *T)
{
	struct ctx *ctx;
	int type = 0;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	if (lua_gettop(T) > 1) {
		type = lua_type(T, 2);
		if (type != LUA_TNUMBER && type != LUA_TSTRING)
			return luaL_argerror(T, 2, "expected number or string");
	}
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
			o = pa_context_get_sink_info_by_index(ctx->handle,
					idx, ctx_sink_info_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_get_sink_info_by_name(ctx->handle,
					name, ctx_sink_info_cb, T);
		}
		break;

	default:
		lua_settop(T, 1);
		lua_createtable(T, 1, 0);
		o = pa_context_get_sink_info_list(ctx->handle,
				ctx_sink_info_cb, T);
		break;
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Source Info
 */
static void
ctx_source_info_push(lua_State *T, const pa_source_info *info)
{
	lua_createtable(T, 0, 21);
	lua_pushstring(T, info->name);
	lua_setfield(T, -2, "name");
	lua_pushnumber(T, info->index);
	lua_setfield(T, -2, "index");
	lua_pushstring(T, info->description);
	lua_setfield(T, -2, "description");
	ctx_sample_spec_push(T, &info->sample_spec);
	lua_setfield(T, -2, "sample_spec");
	ctx_channel_map_push(T, &info->channel_map);
	lua_setfield(T, -2, "channel_map");
	if (info->owner_module != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->owner_module);
		lua_setfield(T, -2, "owner_module");
	}
	ctx_cvolume_push(T, &info->volume);
	lua_setfield(T, -2, "volume");
	lua_pushboolean(T, info->mute);
	lua_setfield(T, -2, "mute");
	if (info->monitor_of_sink != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->monitor_of_sink);
		lua_setfield(T, -2, "monitor_of_sink");
		lua_pushstring(T, info->monitor_of_sink_name);
		lua_setfield(T, -2, "monitor_of_sink_name");
	}
	lua_pushnumber(T, info->latency);
	lua_setfield(T, -2, "latency");
	lua_pushstring(T, info->driver);
	lua_setfield(T, -2, "driver");
	lua_pushnumber(T, info->flags);
	lua_setfield(T, -2, "flags");
	if (info->proplist) {
		ctx_proplist_push(T, info->proplist);
		lua_setfield(T, -2, "proplist");
	}
	lua_pushnumber(T, info->configured_latency);
	lua_setfield(T, -2, "configured_latency");
	lua_pushnumber(T, info->base_volume);
	lua_setfield(T, -2, "base_volume");
	lua_pushnumber(T, info->state);
	lua_setfield(T, -2, "state");
	lua_pushnumber(T, info->n_volume_steps);
	lua_setfield(T, -2, "n_volume_steps");
	if (info->card != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->card);
		lua_setfield(T, -2, "card");
	}
	if (info->ports) {
		uint32_t i = 0;

		lua_createtable(T, info->n_ports, 0);

		while (i < info->n_ports) {
			lua_createtable(T, 0, 4);

			lua_pushstring(T, info->ports[i]->name);
			lua_setfield(T, -2, "name");
			lua_pushstring(T, info->ports[i]->description);
			lua_setfield(T, -2, "description");
			lua_pushnumber(T, info->ports[i]->priority);
			lua_setfield(T, -2, "priority");
			lua_pushboolean(T, info->active_port == info->ports[i]);
			lua_setfield(T, -2, "active");

			lua_rawseti(T, -2, ++i);
		}
		lua_setfield(T, -2, "ports");
	}
	if (info->formats) {
		ctx_formats_push(T, info->n_formats, info->formats);
		lua_setfield(T, -2, "formats");
	}
}

static void
ctx_source_info_cb(pa_context *c,
		const pa_source_info *info, int eol, void *userdata)
{
	lua_State *T = userdata;

	(void)c;
	lem_debug("eol = %d, info = %p", eol, info);

	if (info) {
		ctx_source_info_push(T, info);
		if (lua_gettop(T) > 2)
			lua_rawseti(T, 2, info->index);
	}

	if (eol) {
		if (lua_gettop(T) < 2)
			lua_pushnil(T);
		lem_queue(T, 1);
	}
}

static int
ctx_source_info(lua_State *T)
{
	struct ctx *ctx;
	int type = 0;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	if (lua_gettop(T) > 1) {
		type = lua_type(T, 2);
		if (type != LUA_TNUMBER && type != LUA_TSTRING)
			return luaL_argerror(T, 2, "expected number or string");
	}
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
			o = pa_context_get_source_info_by_index(ctx->handle,
					idx, ctx_source_info_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_get_source_info_by_name(ctx->handle,
					name, ctx_source_info_cb, T);
		}
		break;

	default:
		lua_settop(T, 1);
		lua_createtable(T, 2, 0);
		o = pa_context_get_source_info_list(ctx->handle,
				ctx_source_info_cb, T);
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Server Info
 */
static void
ctx_server_info_cb(pa_context *c, const pa_server_info *info, void *userdata)
{
	lua_State *T = userdata;

	(void)c;

	if (info) {
		lua_createtable(T, 0, 9);
		lua_pushstring(T, info->user_name);
		lua_setfield(T, -2, "user_name");
		lua_pushstring(T, info->host_name);
		lua_setfield(T, -2, "host_name");
		lua_pushstring(T, info->server_version);
		lua_setfield(T, -2, "server_version");
		lua_pushstring(T, info->server_name);
		lua_setfield(T, -2, "server_name");
		ctx_sample_spec_push(T, &info->sample_spec);
		lua_setfield(T, -2, "sample_spec");
		lua_pushstring(T, info->default_sink_name);
		lua_setfield(T, -2, "default_sink_name");
		lua_pushstring(T, info->default_source_name);
		lua_setfield(T, -2, "default_source_name");
		lua_pushnumber(T, info->cookie);
		lua_setfield(T, -2, "cookie");
		ctx_channel_map_push(T, &info->channel_map);
		lua_setfield(T, -2, "channel_map");
	} else
		lua_pushnil(T);

	lem_queue(T, 1);
}

static int
ctx_server_info(lua_State *T)
{
	struct ctx *ctx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	o = pa_context_get_server_info(ctx->handle, ctx_server_info_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Module Info
 */
static void
ctx_module_info_push(lua_State *T, const pa_module_info *info)
{
	lua_createtable(T, 0, 5);
	lua_pushnumber(T, info->index);
	lua_setfield(T, -2, "index");
	lua_pushstring(T, info->name);
	lua_setfield(T, -2, "name");
	lua_pushstring(T, info->argument);
	lua_setfield(T, -2, "argument");
	if (info->n_used != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->n_used);
		lua_setfield(T, -2, "n_used");
	}
	if (info->proplist) {
		ctx_proplist_push(T, info->proplist);
		lua_setfield(T, -2, "proplist");
	}
}

static void
ctx_module_info_cb(pa_context *c,
		const pa_module_info *info, int eol, void *userdata)
{
	lua_State *T = userdata;

	(void)c;
	lem_debug("eol = %d, info = %p", eol, info);

	if (info) {
		ctx_module_info_push(T, info);
		if (lua_gettop(T) > 2)
			lua_rawseti(T, 2, info->index);
	}

	if (eol) {
		if (lua_gettop(T) < 2)
			lua_pushnil(T);
		lem_queue(T, 1);
	}
}

static int
ctx_module_info(lua_State *T)
{
	struct ctx *ctx;
	uint32_t idx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	idx = luaL_optnumber(T, 2, PA_INVALID_INDEX);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	if (idx == PA_INVALID_INDEX) {
		lua_createtable(T, 0, 0);
		o = pa_context_get_module_info_list(ctx->handle,
				ctx_module_info_cb, T);
	} else
		o = pa_context_get_module_info(ctx->handle,
				idx, ctx_module_info_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Client Info
 */
static void
ctx_client_info_push(lua_State *T, const pa_client_info *info)
{
	lua_createtable(T, 0, 5);
	lua_pushnumber(T, info->index);
	lua_setfield(T, -2, "index");
	lua_pushstring(T, info->name);
	lua_setfield(T, -2, "name");
	if (info->owner_module != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->owner_module);
		lua_setfield(T, -2, "owner_module");
	}
	lua_pushstring(T, info->driver);
	lua_setfield(T, -2, "driver");
	if (info->proplist) {
		ctx_proplist_push(T, info->proplist);
		lua_setfield(T, -2, "proplist");
	}
}

static void
ctx_client_info_cb(pa_context *c,
		const pa_client_info *info, int eol, void *userdata)
{
	lua_State *T = userdata;

	(void)c;
	lem_debug("eol = %d, info = %p", eol, info);

	if (info) {
		ctx_client_info_push(T, info);
		if (lua_gettop(T) > 2)
			lua_rawseti(T, 2, info->index);
	}

	if (eol) {
		if (lua_gettop(T) < 2)
			lua_pushnil(T);
		lem_queue(T, 1);
	}
}

static int
ctx_client_info(lua_State *T)
{
	struct ctx *ctx;
	uint32_t idx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	idx = luaL_optnumber(T, 2, PA_INVALID_INDEX);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	if (idx == PA_INVALID_INDEX) {
		lua_createtable(T, 0, 0);
		o = pa_context_get_client_info_list(ctx->handle,
				ctx_client_info_cb, T);
	} else
		o = pa_context_get_client_info(ctx->handle,
				idx, ctx_client_info_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Card Info
 */

/*
 * Sink Input Info
 */
static void
ctx_sink_input_info_push(lua_State *T, const pa_sink_input_info *info)
{
	lua_createtable(T, 0, 18);
	lua_pushnumber(T, info->index);
	lua_setfield(T, -2, "index");
	lua_pushstring(T, info->name);
	lua_setfield(T, -2, "name");
	if (info->owner_module != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->owner_module);
		lua_setfield(T, -2, "owner_module");
	}
	if (info->client != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->client);
		lua_setfield(T, -2, "client");
	}
	lua_pushnumber(T, info->sink);
	lua_setfield(T, -2, "sink");
	ctx_sample_spec_push(T, &info->sample_spec);
	lua_setfield(T, -2, "sample_spec");
	ctx_channel_map_push(T, &info->channel_map);
	lua_setfield(T, -2, "channel_map");
	ctx_cvolume_push(T, &info->volume);
	lua_setfield(T, -2, "volume");
	lua_pushnumber(T, info->buffer_usec);
	lua_setfield(T, -2, "buffer_usec");
	lua_pushnumber(T, info->sink_usec);
	lua_setfield(T, -2, "sink_usec");
	lua_pushstring(T, info->resample_method);
	lua_setfield(T, -2, "resample_method");
	lua_pushstring(T, info->driver);
	lua_setfield(T, -2, "driver");
	lua_pushboolean(T, info->mute);
	lua_setfield(T, -2, "mute");
	if (info->proplist) {
		ctx_proplist_push(T, info->proplist);
		lua_setfield(T, -2, "proplist");
	}
	lua_pushboolean(T, info->corked);
	lua_setfield(T, -2, "corked");
	lua_pushboolean(T, info->has_volume);
	lua_setfield(T, -2, "has_volume");
	lua_pushboolean(T, info->volume_writable);
	lua_setfield(T, -2, "volume_writable");
	if (info->format) {
		ctx_format_push(T, info->format);
		lua_setfield(T, -2, "format");
	}
}

static void
ctx_sink_input_info_cb(pa_context *c,
		const pa_sink_input_info *info, int eol, void *userdata)
{
	lua_State *T = userdata;

	(void)c;
	lem_debug("eol = %d, info = %p", eol, info);

	if (info) {
		ctx_sink_input_info_push(T, info);
		if (lua_gettop(T) > 2)
			lua_rawseti(T, 2, info->index);
	}

	if (eol) {
		if (lua_gettop(T) < 2)
			lua_pushnil(T);
		lem_queue(T, 1);
	}
}

static int
ctx_sink_input_info(lua_State *T)
{
	struct ctx *ctx;
	uint32_t idx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	idx = luaL_optnumber(T, 2, PA_INVALID_INDEX);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	if (idx == PA_INVALID_INDEX) {
		lua_createtable(T, 2, 0);
		o = pa_context_get_sink_input_info_list(ctx->handle,
				ctx_sink_input_info_cb, T);
	} else
		o = pa_context_get_sink_input_info(ctx->handle,
				idx, ctx_sink_input_info_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Source Output Info
 */
static void
ctx_source_output_info_push(lua_State *T, const pa_source_output_info *info)
{
	lua_createtable(T, 0, 18);
	lua_pushnumber(T, info->index);
	lua_setfield(T, -2, "index");
	lua_pushstring(T, info->name);
	lua_setfield(T, -2, "name");
	if (info->owner_module != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->owner_module);
		lua_setfield(T, -2, "owner_module");
	}
	if (info->client != PA_INVALID_INDEX) {
		lua_pushnumber(T, info->client);
		lua_setfield(T, -2, "client");
	}
	lua_pushnumber(T, info->source);
	lua_setfield(T, -2, "source");
	ctx_sample_spec_push(T, &info->sample_spec);
	lua_setfield(T, -2, "sample_spec");
	ctx_channel_map_push(T, &info->channel_map);
	lua_setfield(T, -2, "channel_map");
	lua_pushnumber(T, info->buffer_usec);
	lua_setfield(T, -2, "buffer_usec");
	lua_pushnumber(T, info->source_usec);
	lua_setfield(T, -2, "source_usec");
	lua_pushstring(T, info->resample_method);
	lua_setfield(T, -2, "resample_method");
	lua_pushstring(T, info->driver);
	lua_setfield(T, -2, "driver");
	if (info->proplist) {
		ctx_proplist_push(T, info->proplist);
		lua_setfield(T, -2, "proplist");
	}
	lua_pushboolean(T, info->mute);
	lua_setfield(T, -2, "mute");
	lua_pushboolean(T, info->corked);
	lua_setfield(T, -2, "corked");
	ctx_cvolume_push(T, &info->volume);
	lua_setfield(T, -2, "volume");
	lua_pushboolean(T, info->has_volume);
	lua_setfield(T, -2, "has_volume");
	lua_pushboolean(T, info->volume_writable);
	lua_setfield(T, -2, "volume_writable");
	if (info->format) {
		ctx_format_push(T, info->format);
		lua_setfield(T, -2, "format");
	}
}

static void
ctx_source_output_info_cb(pa_context *c,
		const pa_source_output_info *info, int eol, void *userdata)
{
	lua_State *T = userdata;

	(void)c;
	lem_debug("eol = %d, info = %p", eol, info);

	if (info) {
		ctx_source_output_info_push(T, info);
		if (lua_gettop(T) > 2)
			lua_rawseti(T, 2, info->index);
	}

	if (eol) {
		if (lua_gettop(T) < 2)
			lua_pushnil(T);
		lem_queue(T, 1);
	}
}

static int
ctx_source_output_info(lua_State *T)
{
	struct ctx *ctx;
	uint32_t idx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	idx = luaL_optnumber(T, 2, PA_INVALID_INDEX);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	if (idx == PA_INVALID_INDEX) {
		lua_createtable(T, 2, 0);
		o = pa_context_get_source_output_info_list(ctx->handle,
				ctx_source_output_info_cb, T);
	} else
		o = pa_context_get_source_output_info(ctx->handle,
				idx, ctx_source_output_info_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Stat
 */
static void
ctx_stat_cb(pa_context *c, const pa_stat_info *info, void *userdata)
{
	lua_State *T = userdata;

	(void)c;

	if (info) {
		lua_createtable(T, 0, 5);
		lua_pushnumber(T, info->memblock_total);
		lua_setfield(T, -2, "memblock_total");
		lua_pushnumber(T, info->memblock_total_size);
		lua_setfield(T, -2, "memblock_total_size");
		lua_pushnumber(T, info->memblock_allocated);
		lua_setfield(T, -2, "memblock_allocated");
		lua_pushnumber(T, info->memblock_allocated_size);
		lua_setfield(T, -2, "memblock_allocated_size");
		lua_pushnumber(T, info->scache_size);
		lua_setfield(T, -2, "scache_size");
	} else
		lua_pushnil(T);

	lem_queue(T, 1);
}

static int
ctx_stat(lua_State *T)
{
	struct ctx *ctx;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	lua_settop(T, 1);
	o = pa_context_stat(ctx->handle, ctx_stat_cb, T);
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}

/*
 * Sample Info
 */
static void
ctx_sample_info_push(lua_State *T, const pa_sample_info *info)
{
	lua_createtable(T, 0, 10);
	lua_pushnumber(T, info->index);
	lua_setfield(T, -2, "index");
	lua_pushstring(T, info->name);
	lua_setfield(T, -2, "name");
	ctx_cvolume_push(T, &info->volume);
	lua_setfield(T, -2, "volume");
	ctx_sample_spec_push(T, &info->sample_spec);
	lua_setfield(T, -2, "sample_spec");
	ctx_channel_map_push(T, &info->channel_map);
	lua_setfield(T, -2, "channel_map");
	lua_pushnumber(T, info->duration);
	lua_setfield(T, -2, "duration");
	lua_pushnumber(T, info->bytes);
	lua_setfield(T, -2, "bytes");
	lua_pushboolean(T, info->lazy);
	lua_setfield(T, -2, "lazy");
	lua_pushstring(T, info->filename);
	lua_setfield(T, -2, "filename");
	if (info->proplist) {
		ctx_proplist_push(T, info->proplist);
		lua_setfield(T, -2, "proplist");
	}
}

static void
ctx_sample_info_cb(pa_context *c, const pa_sample_info *info, int eol, void *userdata)
{
	lua_State *T = userdata;

	(void)c;
	lem_debug("eol = %d, info = %p", eol, info);

	if (info) {
		ctx_sample_info_push(T, info);
		if (lua_gettop(T) > 2)
			lua_rawseti(T, 2, info->index);
	}

	if (eol) {
		if (lua_gettop(T) < 2)
			lua_pushnil(T);
		lem_queue(T, 1);
	}
}

static int
ctx_sample_info(lua_State *T)
{
	struct ctx *ctx;
	int type = 0;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	if (lua_gettop(T) > 1) {
		type = lua_type(T, 2);
		if (type != LUA_TNUMBER && type != LUA_TSTRING)
			return luaL_argerror(T, 2, "expected number or string");
	}
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
			o = pa_context_get_sample_info_by_index(ctx->handle,
					idx, ctx_sample_info_cb, T);
		}
		break;

	case LUA_TSTRING:
		{
			const char *name = lua_tostring(T, 2);

			lua_settop(T, 1);
			o = pa_context_get_sample_info_by_name(ctx->handle,
					name, ctx_sample_info_cb, T);
		}
		break;

	default:
		lua_settop(T, 1);
		lua_createtable(T, 2, 0);
		o = pa_context_get_sample_info_list(ctx->handle,
				ctx_sample_info_cb, T);
	}
	pa_operation_unref(o);
	return lua_yield(T, lua_gettop(T));
}
