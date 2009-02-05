#include <ev.h>

#ifndef __EVX_INCLUDE_GAURD__
#define __EVX_INCLUDE_GAURD__

template <typename tWatcher>
struct evx_watcher;
typedef evx_watcher<ev_io> evx_io;
typedef evx_watcher<ev_timer> evx_timer;
typedef evx_watcher<ev_periodic> evx_periodic;
typedef evx_watcher<ev_signal> evx_signal;
typedef evx_watcher<ev_child> evx_child;
typedef evx_watcher<ev_idle> evx_idle;
typedef evx_watcher<ev_prepare> evx_prepare;
typedef evx_watcher<ev_check> evx_check;

template <typename tWatcher>
struct evx_watcher : public tWatcher
{
	typedef evx_watcher<tWatcher> self;
	void (*deleter)(self *w);
};

template <typename tWatcher, typename tCallback>
void evx_callback(struct ev_loop *loop, tWatcher *watcher, int revents)
{
	(*static_cast<tCallback*>(watcher->data))(loop, static_cast<evx_watcher<tWatcher>*>(watcher), revents);
}

template <typename tWatcher, typename tCallback>
void evx_delete_callback(evx_watcher<tWatcher> *ev)
{
	delete static_cast<tCallback*>(ev->data);
}

template <typename tWatcher, typename tCallback>
void evx_init(evx_watcher<tWatcher> *ev, const tCallback &cb)
{
	typedef void (*tFunc)(struct ev_loop *loop, tWatcher *watcher, int revents);
	tFunc func = evx_callback<tWatcher, tCallback>;
	ev_init(ev, func);
	ev->data = new tCallback(cb);
	ev->deleter = &evx_delete_callback<tWatcher, tCallback>;
}

template <typename tWatcher>
void evx_clean(tWatcher *ev)
{
	ev->deleter(ev);
}

#endif