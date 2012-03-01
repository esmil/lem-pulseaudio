#define STEP (2 * M_PI / 44100.0)

struct stream {
	pa_stream *handle;
	lua_State *T;
	double t;
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

static void
stream_init_cb(pa_stream *h, void *userdata)
{
	lua_State *T = userdata;
	pa_stream_state_t state = pa_stream_get_state(h);

	switch (state) {
	case PA_STREAM_CREATING:
		lem_debug("CREATING");
		break;
	case PA_STREAM_READY:
		lem_debug("READY");
		pa_stream_set_state_callback(h, stream_state_cb, NULL);
		lem_queue(T, 1);
		break;
	case PA_STREAM_UNCONNECTED:
		lem_debug("UNCONNECTED");
	case PA_STREAM_TERMINATED:
		lem_debug("TERMINATED");
	case PA_STREAM_FAILED:
		lem_debug("FAILED");
		pa_stream_unref(h);
		lua_pushnil(T);
		lua_pushliteral(T, "error initializing stream");
		lem_queue(T, 2);
		break;
	}
}

static void
stream_write_cb(pa_stream *h, size_t nbytes, void *userdata)
{
	struct stream *s = userdata;
	lua_State *T = s->T;

	(void)h;
	lem_debug("requested %lu bytes, T = %p", nbytes, T);

#if 0
	while (1) {
		void *buf;
		size_t size = (size_t) -1;
		short *data;
		size_t len;
		size_t i;
		double t;
		int ret = pa_stream_begin_write(h, &buf, &size);
		lem_debug("received buffer of size %lu, ret = %d",
				size, ret);

		data = buf;
		len = size / sizeof(short);
		t = s->t;
		for (i = 0; i < len;) {
			short v = 10000.0*sin(440.0 * t);

			data[i++] = v;
			data[i++] = v;
			t += STEP;
		}
		s->t = t;

		pa_stream_write(h, buf, size, NULL, 0, PA_SEEK_RELATIVE);
		lem_debug("pa_stream_writable_size() = %lu",
				pa_stream_writable_size(h));

		if (nbytes < size)
			break;

		nbytes -= size;
	}
#else
	if (T == NULL)
		return;

	lua_pushnumber(T, nbytes / sizeof(short));
	s->T = NULL;
	lem_queue(T, 1);
#endif
}

static int
stream_writable_wait(lua_State *T)
{
	struct stream *s;
	size_t size;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

	size = pa_stream_writable_size(s->handle);
	if (size > 0) {
		lua_pushnumber(T, size / sizeof(short));
		return 1;
	}

	s->T = T;
	lua_settop(T, 1);
	return lua_yield(T, 1);
}

static int
stream_write(lua_State *T)
{
	struct stream *s;
	void *buf;
	size_t size;
	short *array;
	size_t len;
	int i;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	luaL_checktype(T, 2, LUA_TTABLE);

	s = lua_touserdata(T, 1);
	if (s->T != NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "busy");
		return 2;
	}

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
			lem_debug("writing %lu bytes, %lu samples", size, size / sizeof(short));
			pa_stream_write(s->handle, buf, size,
					NULL, 0, PA_SEEK_RELATIVE);
			size = (size_t) -1;
			if (pa_stream_begin_write(s->handle, &buf, &size))
				goto error;

			array = buf;
			len = size / sizeof(short);
			size = 0;
		}
	}

	lem_debug("writing %lu bytes, %lu samples", size, size / sizeof(short));
	pa_stream_write(s->handle, buf, size,
			NULL, 0, PA_SEEK_RELATIVE);

	lua_pushboolean(T, 1);
	return 1;

error:
	lem_debug("YIKES!!!");
	return 0;
}

static int
ctx_stream(lua_State *T)
{
	struct ctx *ctx;
	pa_sample_spec ss;
	pa_buffer_attr attr;
	pa_stream *h;
	struct stream *s;

	luaL_checktype(T, 1, LUA_TUSERDATA);
	ctx = lua_touserdata(T, 1);
	if (ctx->handle == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "closed");
		return 2;
	}

	ss.format = PA_SAMPLE_S16LE;
	ss.rate = 44100;
	ss.channels = 2;

	h = pa_stream_new(ctx->handle, "beep", &ss, NULL);
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
	s->t = 0.0;

	pa_stream_set_state_callback(h, stream_init_cb, T);
	pa_stream_set_write_callback(h, stream_write_cb, s);

	attr.maxlength = (uint32_t) -1;
	attr.tlength = (uint32_t) -1;
	attr.prebuf = (uint32_t) -1;
	attr.minreq = (uint32_t) -1;
	attr.fragsize = (uint32_t) -1;
	if (pa_stream_connect_playback(h, NULL, &attr,
			PA_STREAM_NOT_MONOTONIC, NULL, NULL)) {
		pa_stream_unref(h);
		lua_pushnil(T);
		lua_pushliteral(T, "error connecting stream");
		return 2;
	}
	return lua_yield(T, 2);
}
