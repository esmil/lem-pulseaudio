#define STEP (2 * M_PI / 44100.0)

struct beeper {
	pa_stream *s;
	double t;
};

static void
beep_state_cb(pa_stream *s, void *userdata)
{
	pa_stream_state_t state = pa_stream_get_state(s);

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
beep_init_cb(pa_stream *s, void *userdata)
{
	lua_State *T = userdata;
	pa_stream_state_t state = pa_stream_get_state(s);

	switch (state) {
	case PA_STREAM_UNCONNECTED:
		lem_debug("UNCONNECTED");
		break;
	case PA_STREAM_CREATING:
		lem_debug("CREATING");
		break;
	case PA_STREAM_READY:
		lem_debug("READY");
		pa_stream_set_state_callback(s, beep_state_cb, NULL);
		lua_pushboolean(T, 1);
		lem_queue(T, 1);
		break;
	case PA_STREAM_FAILED:
		lem_debug("FAILED");
		pa_stream_unref(s);
		lua_pushnil(T);
		lua_pushliteral(T, "error initializing stream");
		lem_queue(T, 2);
		break;
	case PA_STREAM_TERMINATED:
		lem_debug("TERMINATED");
		break;
	}
}

static void
beep_write_cb(pa_stream *s, size_t target, void *userdata)
{
	struct beeper *b = userdata;

	lem_debug("requested %lu bytes", target);

	while (1) {
		void *buf;
		size_t size = (size_t) -1;
		short *data;
		size_t len;
		size_t i;
		double t;
		int ret = pa_stream_begin_write(s, &buf, &size);
		lem_debug("received buffer of size %lu, ret = %d",
				size, ret);

		data = buf;
		len = size / sizeof(short);
		t = b->t;
		for (i = 0; i < len;) {
			short l = 10000.0*(1.0 - cos(4*t)) * sin(220.0 * t);
			short r = 10000.0*(1.0 - cos(t)) * sin(440.0 * t);

			data[i++] = l;
			data[i++] = r;
			t += STEP;
			if (t > 2*M_PI)
				t -= 2*M_PI;
		}
		b->t = t;

		pa_stream_write(s, buf, size, NULL, 0, PA_SEEK_RELATIVE);

		if (target < size)
			break;

		target -= size;
	}
}

static void
beep_overflow_cb(pa_stream *s, void *userdata)
{
	(void)s;
	(void)userdata;

	lem_debug("OVERFLOW");
}

static int
ctx_beep(lua_State *T)
{
	struct ctx *ctx;
	pa_sample_spec ss;
	pa_buffer_attr attr;
	pa_stream *s;
	struct beeper *b;

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

	s = pa_stream_new(ctx->handle, "beep", &ss, NULL);
	if (s == NULL) {
		lua_pushnil(T);
		lua_pushliteral(T, "error initializing stream");
		return 2;
	}

	pa_stream_set_state_callback(s, beep_init_cb, T);
	pa_stream_set_overflow_callback(s, beep_overflow_cb, NULL);

	attr.maxlength = (uint32_t) -1;
	attr.tlength = (uint32_t) -1;
	attr.prebuf = (uint32_t) -1;
	attr.minreq = (uint32_t) -1;
	attr.fragsize = (uint32_t) -1;
	if (pa_stream_connect_playback(s, NULL, &attr,
			PA_STREAM_NOT_MONOTONIC, NULL, NULL)) {
		pa_stream_unref(s);
		lua_pushnil(T);
		lua_pushliteral(T, "error connecting stream");
		return 2;
	}

	lua_settop(T, 1);
	b = lua_newuserdata(T, sizeof(struct beeper));
	b->s = s;
	b->t = 0.0;

	pa_stream_set_write_callback(s, beep_write_cb, b);
	return lua_yield(T, 2);
}
