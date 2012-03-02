struct stream {
	pa_stream *handle;
	lua_State *T;
};

static int
stream_gc(lua_State *T)
{
	struct stream *s = lua_touserdata(T, 1);

	lem_debug("s->handle = %p", s->handle);
	if (s->handle == NULL)
		return 0;

	pa_stream_unref(s->handle);
	s->handle = NULL;
	return 0;
}

static void
stream_success_cb(pa_stream *h, int success, void *userdata)
{
	struct stream *s = userdata;
	lua_State *T = s->T;

	(void)h;
	lem_debug("success = %d", success);

	lua_pushboolean(T, success);
	s->T = NULL;
	lem_queue(T, 1);
}

static void
stream_state_cb(pa_stream *h, void *userdata)
{
	pa_stream_state_t state = pa_stream_get_state(h);

	(void)userdata;

	switch (state) {
	case PA_STREAM_UNCONNECTED:
		lem_debug("UNCONNECTED");
		break;
	case PA_STREAM_CREATING:
		lem_debug("CREATING");
		break;
	case PA_STREAM_READY:
		lem_debug("READY");
		break;
	case PA_STREAM_FAILED:
		lem_debug("FAILED");
		break;
	case PA_STREAM_TERMINATED:
		lem_debug("TERMINATED");
		break;
	}
}

static int
stream_state(lua_State *T)
{
	struct stream *s;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	s = lua_touserdata(T, 1);

	switch (pa_stream_get_state(s->handle)) {
	case PA_STREAM_UNCONNECTED:
		lua_pushliteral(T, "unconnected");
		break;
	case PA_STREAM_CREATING:
		lua_pushliteral(T, "creating");
		break;
	case PA_STREAM_READY:
		lua_pushliteral(T, "ready");
		break;
	case PA_STREAM_FAILED:
		lua_pushliteral(T, "failed");
		break;
	case PA_STREAM_TERMINATED:
		lua_pushliteral(T, "terminated");
		break;
	}
	return 1;
}

static void
stream_writable_wait_cb(pa_stream *h, size_t nbytes, void *userdata)
{
	struct stream *s = userdata;
	lua_State *T = s->T;

	(void)nbytes;
	lem_debug("requested %lu bytes, T = %p", nbytes, T);
	pa_stream_set_write_callback(h, NULL, NULL);

	s->T = NULL;
	lem_queue(T, 1);
}

static int
stream_writable_wait_k(lua_State *T)
{
	struct stream *s = lua_touserdata(T, 1);
	size_t size = pa_stream_writable_size(s->handle);

	if (size > 0) {
		lua_pushnumber(T, size / sizeof(short));
		return 1;
	}
	s->T = T;
	pa_stream_set_write_callback(s->handle, stream_writable_wait_cb, s);
	return lua_yieldk(T, 1, 0, stream_writable_wait_k);
}

static int
stream_writable_wait(lua_State *T)
{
	struct stream *s;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	lua_settop(T, 1);
	return stream_writable_wait_k(T);
}

static int
stream_write_s16le(lua_State *T, struct stream *s)
{
	void *buf;
	size_t size;
	short *array;
	size_t len;
	int i;

	size = (size_t) -1;
	if (pa_stream_begin_write(s->handle, &buf, &size))
		goto error;

	array = buf;
	len = size / sizeof(short);
	size = 0;

	for (i = 1;; i++) {
		lua_settop(T, 2);
		lua_rawgeti(T, 2, i);
		if (lua_type(T, 3) != LUA_TNUMBER)
			break;

		*array++ = lua_tonumber(T, 3);
		len--;
		size += sizeof(short);

		if (len == 0) {
			lem_debug("writing %lu bytes, %lu samples",
					size, size / sizeof(short));
			if (pa_stream_write(s->handle, buf, size,
					NULL, 0, PA_SEEK_RELATIVE))
				goto error;

			size = (size_t) -1;
			if (pa_stream_begin_write(s->handle, &buf, &size))
				goto error;

			array = buf;
			len = size / sizeof(short);
			size = 0;
		}
	}

	lem_debug("writing %lu bytes, %lu samples",
			size, size / sizeof(short));
	if (pa_stream_write(s->handle, buf, size,
			NULL, 0, PA_SEEK_RELATIVE))
		goto error;

	lua_pushboolean(T, 1);
	return 1;

error:
	lua_pushnil(T);
	lua_pushliteral(T, "error writing to stream");
	return 2;
}

static int
stream_write_float32le(lua_State *T, struct stream *s)
{
	void *buf;
	size_t size;
	float *array;
	size_t len;
	int i;

	size = (size_t) -1;
	if (pa_stream_begin_write(s->handle, &buf, &size))
		goto error;

	array = buf;
	len = size / sizeof(float);
	size = 0;

	for (i = 1;; i++) {
		lua_settop(T, 2);
		lua_rawgeti(T, 2, i);
		if (lua_type(T, 3) != LUA_TNUMBER)
			break;

		*array++ = lua_tonumber(T, 3);
		len--;
		size += sizeof(float);

		if (len == 0) {
			lem_debug("writing %lu bytes, %lu samples",
					size, size / sizeof(float));
			if (pa_stream_write(s->handle, buf, size,
					NULL, 0, PA_SEEK_RELATIVE))
				goto error;

			size = (size_t) -1;
			if (pa_stream_begin_write(s->handle, &buf, &size))
				goto error;

			array = buf;
			len = size / sizeof(float);
			size = 0;
		}
	}

	lem_debug("writing %lu bytes, %lu samples",
			size, size / sizeof(float));
	if (pa_stream_write(s->handle, buf, size,
			NULL, 0, PA_SEEK_RELATIVE))
		goto error;

	lua_pushboolean(T, 1);
	return 1;

error:
	lua_pushnil(T);
	lua_pushliteral(T, "error writing to stream");
	return 2;
}

static int
stream_write(lua_State *T)
{
	struct stream *s;
	const pa_sample_spec *ss;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	luaL_checktype(T, 2, LUA_TTABLE);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	ss = pa_stream_get_sample_spec(s->handle);
	switch (ss->format) {
	case PA_SAMPLE_S16LE:
		return stream_write_s16le(T, s);
	case PA_SAMPLE_FLOAT32LE:
		return stream_write_float32le(T, s);
	default:
		lua_pushnil(T);
		lua_pushliteral(T, "unsupported sample format");
		return 2;
	}
}

static void
stream_connect_cb(pa_stream *h, void *userdata)
{
	lua_State *T = userdata;
	pa_stream_state_t state = pa_stream_get_state(h);

	switch (state) {
	case PA_STREAM_CREATING:
		lem_debug("CREATING");
		return;
	case PA_STREAM_READY:
		lem_debug("READY");
		pa_stream_set_state_callback(h, stream_state_cb, NULL);
		lem_queue(T, 1);
		return;
	case PA_STREAM_FAILED:
		lem_debug("FAILED");
		lua_pushnil(T);
		lua_pushliteral(T, "failed");
		break;

	case PA_STREAM_UNCONNECTED:
		lem_debug("UNCONNECTED");
	case PA_STREAM_TERMINATED:
		lem_debug("TERMINATED");
		lua_pushnil(T);
		lua_pushliteral(T, "terminated");
		break;
	}
	pa_stream_set_state_callback(h, stream_state_cb, NULL);
	lem_queue(T, 2);
}

static int
stream_connect_playback(lua_State *T)
{
	struct stream *s;
	pa_buffer_attr attr;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	pa_stream_set_state_callback(s->handle, stream_connect_cb, T);

	attr.maxlength = (uint32_t) -1;
	attr.tlength = (uint32_t) -1;
	attr.prebuf = (uint32_t) -1;
	attr.minreq = (uint32_t) -1;
	attr.fragsize = (uint32_t) -1;
	if (pa_stream_connect_playback(s->handle, NULL, &attr,
			PA_STREAM_NOT_MONOTONIC, NULL, NULL)) {
		lua_pushnil(T);
		lua_pushliteral(T, "error connecting stream");
		return 2;
	}

	lua_settop(T, 1);
	return lua_yield(T, 1);
}

static int
stream_set_name(lua_State *T)
{
	struct stream *s;
	const char *name;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	name = luaL_checkstring(T, 2);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	lua_settop(T, 1);
	s->T = T;
	o = pa_stream_set_name(s->handle, name, stream_success_cb, s);
	pa_operation_unref(o);
	return lua_yield(T, 1);
}

static int
stream_drain(lua_State *T)
{
	struct stream *s;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	lua_settop(T, 1);
	s->T = T;
	o = pa_stream_drain(s->handle, stream_success_cb, s);
	pa_operation_unref(o);
	return lua_yield(T, 1);
}

static int
stream_flush(lua_State *T)
{
	struct stream *s;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	lua_settop(T, 1);
	s->T = T;
	o = pa_stream_flush(s->handle, stream_success_cb, s);
	pa_operation_unref(o);
	return lua_yield(T, 1);
}

static int
stream_prebuf(lua_State *T)
{
	struct stream *s;
	pa_operation *o;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	lua_settop(T, 1);
	s->T = T;
	o = pa_stream_prebuf(s->handle, stream_success_cb, s);
	pa_operation_unref(o);
	return lua_yield(T, 1);
}

static int
stream_disconnect(lua_State *T)
{
	struct stream *s;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(s->T);
		lua_pushliteral(s->T, "disconnected");
		lem_queue(s->T, 2);
	}

	if (pa_stream_disconnect(s->handle)) {
		lua_pushnil(T);
		lua_pushliteral(T, "failed");
		return 2;
	}

	lua_pushboolean(T, 1);
	return 1;
}

static int
ctx_stream(lua_State *T)
{
	struct ctx *ctx;
	const char *name;
	pa_sample_spec ss;
	pa_stream *h;
	struct stream *s;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	name = luaL_checkstring(T, 2);
	ss.format = pa_parse_sample_format(luaL_checkstring(T, 3));
	if (ss.format == PA_SAMPLE_INVALID)
		return luaL_argerror(T, 3, "invalid sample format");
	ss.rate = luaL_checknumber(T, 4);
	ss.channels = luaL_checknumber(T, 5);

	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	h = pa_stream_new(ctx->handle, name, &ss, NULL);
	if (h == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "error initializing stream");
		return 2;
	}

	lua_settop(T, 1);
	s = lua_newuserdata(T, sizeof(struct stream));
	lua_pushvalue(T, lua_upvalueindex(1));
	lua_setmetatable(T, -2);
	s->handle = h;
	s->T = NULL;
	return 1;
}
