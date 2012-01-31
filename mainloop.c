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
 * IO Events
 */
struct pa_io_event {
	struct ev_io w;
	pa_io_event_cb_t cb;
	pa_io_event_destroy_cb_t destroy;
};

static struct pa_io_event *
io_new(pa_mainloop_api *a, int fd, pa_io_event_flags_t events,
		pa_io_event_cb_t cb, void *userdata);
static void
io_enable(struct pa_io_event *ev, pa_io_event_flags_t events);
static void
io_free(struct pa_io_event *ev);
static void
io_set_destroy(struct pa_io_event *ev, pa_io_event_destroy_cb_t cb);

/*
 * Time Events
 */
struct pa_time_event {
	struct ev_timer w;
	struct timeval tv;
	pa_time_event_cb_t cb;
	pa_time_event_destroy_cb_t destroy;
};

static struct pa_time_event *
time_new(pa_mainloop_api *a, const struct timeval *tv,
		pa_time_event_cb_t cb, void *userdata);
static void
time_restart(struct pa_time_event *ev, const struct timeval *tv);
static void
time_free(struct pa_time_event *ev);
static void
time_set_destroy(struct pa_time_event *ev, pa_time_event_destroy_cb_t cb);

/*
 * Defer Events
 */
struct pa_defer_event {
	struct ev_idle w;
	pa_defer_event_cb_t cb;
	pa_defer_event_destroy_cb_t destroy;
};

static struct pa_defer_event *
defer_new(pa_mainloop_api *a, pa_defer_event_cb_t cb, void *userdata);
static void
defer_enable(struct pa_defer_event *ev, int b);
static void
defer_free(struct pa_defer_event *ev);
static void
defer_set_destroy(struct pa_defer_event *ev, pa_defer_event_destroy_cb_t cb);

static void
loop_quit(pa_mainloop_api *a, int retval)
{
	(void)a;
	(void)retval;
	lem_debug("Hmm.. pulseaudio wants us to quit with retval %d", retval);
}

static struct pa_mainloop_api loop_api = {
	.io_new            = io_new,
	.io_enable         = io_enable,
	.io_free           = io_free,
	.io_set_destroy    = io_set_destroy,

	.time_new          = time_new,
	.time_restart      = time_restart,
	.time_free         = time_free,
	.time_set_destroy  = time_set_destroy,

	.defer_new         = defer_new,
	.defer_enable      = defer_enable,
	.defer_free        = defer_free,
	.defer_set_destroy = defer_set_destroy,

	.quit              = loop_quit,
};

/*
 * IO Events
 */
static void
io_handler(EV_P_ ev_io *w, int revents)
{
	struct pa_io_event *ev = (struct pa_io_event *)w;
	pa_io_event_flags_t events = PA_IO_EVENT_NULL;

	if (revents & EV_READ)
		events |= PA_IO_EVENT_INPUT;
	if (revents & EV_WRITE)
		events |= PA_IO_EVENT_OUTPUT;
	if (revents & EV_ERROR)
		events |= PA_IO_EVENT_ERROR;

	lem_debug("flags = %d", events);
	ev->cb(&loop_api, ev, ev->w.fd, events, ev->w.data);
}

static struct pa_io_event *
io_new(pa_mainloop_api *a, int fd, pa_io_event_flags_t events,
		pa_io_event_cb_t cb, void *userdata)
{
	struct pa_io_event *ev = lem_xmalloc(sizeof(struct pa_io_event));
	int revents = 0;

	(void)a;
	lem_debug("flags = %d", events);

	if (events & PA_IO_EVENT_INPUT)
		revents |= EV_READ;
	if (events & PA_IO_EVENT_OUTPUT)
		revents |= EV_WRITE;
	if (events & PA_IO_EVENT_HANGUP)
		revents |= EV_READ;

	ev_io_init(&ev->w, io_handler, fd, revents);
	ev->w.data = userdata;
	ev->cb = cb;
	ev->destroy = NULL;

	if (revents)
		ev_io_start(EV_G_ &ev->w);
	return ev;
}

static void
io_enable(pa_io_event *ev, pa_io_event_flags_t events)
{
	int revents = 0;

	lem_debug("flags = %d", events);

	if (events & PA_IO_EVENT_INPUT)
		revents |= EV_READ;
	if (events & PA_IO_EVENT_OUTPUT)
		revents |= EV_WRITE;
	if (events & PA_IO_EVENT_HANGUP)
		revents |= EV_READ;

	if (revents != ev->w.events) {
		ev_io_stop(EV_G_ &ev->w);
		ev->w.events = revents;
		if (revents)
			ev_io_start(EV_G_ &ev->w);
	}
}

static void
io_free(pa_io_event *ev)
{
	lem_debug("freeing event %p", ev);
	ev_io_stop(EV_G_ &ev->w);
	if (ev->destroy)
		ev->destroy(&loop_api, ev, ev->w.data);
	free(ev);
}

static void
io_set_destroy(struct pa_io_event *ev, pa_io_event_destroy_cb_t cb)
{
	lem_debug("setting destroy callback");
	ev->destroy = cb;
}

/*
 * Time Events
 */
static inline ev_tstamp
timeval_to_stamp(const struct timeval *tv)
{
	return (((ev_tstamp)tv->tv_usec)/1000000.0) + ((ev_tstamp)tv->tv_sec);
}

static void
time_handler(EV_P_ struct ev_timer *w, int revents)
{
	struct pa_time_event *ev = (struct pa_time_event *)w;

	(void)revents;
	lem_debug("callback..");
	ev->cb(&loop_api, ev, &ev->tv, ev->w.data);
}

static struct pa_time_event *
time_new(pa_mainloop_api *a, const struct timeval *tv,
		pa_time_event_cb_t cb, void *userdata)
{
	struct pa_time_event *ev = lem_xmalloc(sizeof(struct pa_time_event));
	ev_tstamp timeout = timeval_to_stamp(tv);

	(void)a;
	lem_debug("after = %f seconds", timeout - ev_now(EV_G));

	ev->w.data = userdata;
	ev->tv.tv_sec = tv->tv_sec;
	ev->tv.tv_usec = tv->tv_usec;
	ev->cb = cb;
	ev->destroy = NULL;

	ev_timer_init(&ev->w, time_handler, timeout - ev_now(EV_G), 0);
	ev_timer_start(EV_G_ &ev->w);
	return ev;
}

static void
time_restart(pa_time_event *ev, const struct timeval *tv)
{
	ev_tstamp timeout = timeval_to_stamp(tv);

	lem_debug("resetting to %f seconds", timeout - ev_now(EV_G));

	ev->tv.tv_sec = tv->tv_sec;
	ev->tv.tv_usec = tv->tv_usec;

	ev_timer_stop(EV_G_ &ev->w);
	ev_timer_set(&ev->w, timeout - ev_now(EV_G), 0);
	ev_timer_start(EV_G_ &ev->w);
}

static void
time_free(pa_time_event *ev)
{
	lem_debug("freeing event %p", ev);
	ev_timer_stop(EV_G_ &ev->w);
	if (ev->destroy)
		ev->destroy(&loop_api, ev, ev->w.data);
	free(ev);
}

static void
time_set_destroy(struct pa_time_event *ev, pa_time_event_destroy_cb_t cb)
{
	lem_debug("setting destroy callback");
	ev->destroy = cb;
}

/*
 * Defer Events
 */
static void
defer_handler(EV_P_ struct ev_idle *w, int revents)
{
	struct pa_defer_event *ev = (struct pa_defer_event *)w;

	(void)revents;
	lem_debug("callback..");

	ev->cb(&loop_api, ev, ev->w.data);
}

static struct pa_defer_event *
defer_new(pa_mainloop_api *a, pa_defer_event_cb_t cb, void *userdata)
{
	struct pa_defer_event *ev = lem_xmalloc(sizeof(struct pa_defer_event));

	(void)a;
	lem_debug("new defer %p", ev);

	ev_idle_init(&ev->w, defer_handler);
	ev->w.data = userdata;
	ev->cb = cb;
	ev->destroy = NULL;

	ev_idle_start(EV_G_ &ev->w);
	return ev;
}

static void
defer_enable(struct pa_defer_event *ev, int b)
{
	if (b) {
		lem_debug("starting");
		ev_idle_start(EV_G_ &ev->w);
	} else {
		lem_debug("stopping");
		ev_idle_stop(EV_G_ &ev->w);
	}
}

static void
defer_free(struct pa_defer_event *ev)
{
	lem_debug("freeing event %p", ev);
	if (ev->destroy)
		ev->destroy(&loop_api, ev, ev->w.data);
	ev_idle_stop(EV_G_ &ev->w);
	free(ev);
}

static void
defer_set_destroy(struct pa_defer_event *ev, pa_defer_event_destroy_cb_t cb)
{
	lem_debug("setting destroy callback");
	ev->destroy = cb;
}
