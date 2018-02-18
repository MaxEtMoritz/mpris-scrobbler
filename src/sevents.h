/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SEVENTS_H
#define MPRIS_SCROBBLER_SEVENTS_H

bool now_playing_event_free(struct event *event, struct scrobble *payload)
{
    if (NULL == event) { return 0; }

    _trace2("mem::free::event(%p):now_playing", event);
    event_free(event);


    if (NULL != payload) {
        _trace2("mem::free::event_payload(%p):now_playing", payload);
        scrobble_free(payload);
    }

    return true;
}

void events_free(struct events *ev)
{
    if (NULL != ev->dispatch) {
        _trace2("mem::free::event(%p):dispatch", ev->dispatch);
        event_free(ev->dispatch);
    }
    if (NULL != ev->scrobble) {
        _trace2("mem::free::event(%p):scrobble", ev->scrobble);
        event_free(ev->scrobble);
    }
    if (NULL != ev->now_playing) {
        now_playing_event_free(ev->now_playing, ev->now_playing_payload);
    }
    _trace2("mem::free::event(%p):SIGINT", ev->sigint);
    event_free(ev->sigint);
    _trace2("mem::free::event(%p):SIGTERM", ev->sigterm);
    event_free(ev->sigterm);
    _trace2("mem::free::event(%p):SIGHUP", ev->sighup);
    event_free(ev->sighup);
    free(ev);
}

struct events *events_new(void)
{
    struct events *result = calloc(1, sizeof(struct events));

    return result;
}

void events_init(struct events *ev, struct sighandler_payload *p)
{
    ev->base = event_base_new();
    if (NULL == ev->base) {
        _error("mem::init_libevent: failure");
        return;
    } else {
        _trace2("mem::inited_libevent(%p)", ev->base);
    }
    p->event_base = ev->base;
    ev->dispatch = NULL;
    ev->scrobble = NULL;
    ev->now_playing = NULL;
    ev->now_playing_payload = NULL;
    //ev->scrobble_payload = {NULL};

    ev->sigint = evsignal_new(ev->base, SIGINT, sighandler, p);
    if (NULL == ev->sigint || event_add(ev->sigint, NULL) < 0) {
        _error("mem::add_event(SIGINT): failed");
        return;
    }
    ev->sigterm = evsignal_new(ev->base, SIGTERM, sighandler, p);
    if (NULL == ev->sigterm || event_add(ev->sigterm, NULL) < 0) {
        _error("mem::add_event(SIGTERM): failed");
        return;
    }
    ev->sighup = evsignal_new(ev->base, SIGHUP, sighandler, p);
    if (NULL == ev->sighup || event_add(ev->sighup, NULL) < 0) {
        _error("mem::add_event(SIGHUP): failed");
        return;
    }
}

static void remove_event_now_playing(struct events *events)
{
    if (NULL == events) { return; }
    _trace("events::remove_event(%p::%p):now_playing", events->now_playing, events->now_playing_payload);
    now_playing_event_free(events->now_playing, events->now_playing_payload);

    events->now_playing = NULL;
    events->now_playing_payload = NULL;
}

void now_playing_payload_free(struct now_playing_payload *p)
{
    if (NULL == p) { return; }

    if (NULL != p->track) {
        scrobble_free(p->track);
    }

    free(p);
}

struct now_playing_payload *now_playing_payload_new(struct scrobbler *scrobbler, struct events *ev, struct scrobble *track)
{
    struct now_playing_payload *p = calloc(1, sizeof(struct now_playing_payload));

    p->track = track;
    p->now_playing = ev->now_playing;
    p->scrobbler = scrobbler;

    return p;
}

bool scrobbler_now_playing(struct scrobbler*, const struct scrobble*);
static void send_now_playing(evutil_socket_t fd, short event, void *data)
{
    if (NULL == data) {
        _warn("events::triggered::now_playing: missing data");
        return;
    }

    struct now_playing_payload *state = data;
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    if (NULL == state->track) {
        _warn("events::triggered::now_playing: missing current track");
        return;
    }

    state->track->position += NOW_PLAYING_DELAY;
    _trace("events::triggered(%p:%p):now_playing", state->now_playing, state->track);
    scrobbler_now_playing(state->scrobbler, state->track);

    struct timeval now_playing_tv = {
        .tv_sec = NOW_PLAYING_DELAY,
        .tv_usec = 0
    };
    _trace("events::add_event(%p//%p):now_playing in %2.3f seconds", state->now_playing, state->track, (double)(now_playing_tv.tv_sec + now_playing_tv.tv_usec));
    event_add(state->now_playing, &now_playing_tv);
}

static bool add_event_now_playing(struct state *state, struct scrobble *track)
{

    if (NULL == state) { return false; }
    if (NULL == track) { return false; }
    if (NULL == state->events) { return false; }

    struct timeval now_playing_tv = { .tv_sec = 0, .tv_usec = 0 };
    struct events *ev = state->events;

    if (NULL == state->player) { return false; }

    if (NULL != ev->now_playing) { remove_event_now_playing(state->events); }
    if (NULL == ev->now_playing_payload) {
        ev->now_playing_payload = scrobble_new();
    }
    scrobble_zero(ev->now_playing_payload);
    scrobble_copy(ev->now_playing_payload, track);
    //scrobble_print(ev->now_playing_payload);

    // @TODO: take into consideration the current position
    //unsigned current_position = track->position;
    //unsigned length = track->length;
    //size_t now_playing_count = (length - current_position) / NOW_PLAYING_DELAY;

    ev->now_playing = calloc(1, sizeof(struct event));
    //_trace("events::add_event(%p):now_playing: track_lenth: %zu(s), event_count: %u", ev->now_playing, length, now_playing_count);

    // TODO(marius): we need to add the payloads to the state so we can free it for events destroyed before being triggered
    struct now_playing_payload *payload = now_playing_payload_new(state->scrobbler, state->events, ev->now_playing_payload);
    if (NULL == payload) { return false; }

    // Initalize timed event for now_playing
    if (event_assign(ev->now_playing, ev->base, -1, EV_PERSIST, send_now_playing, payload) == 0) {
        _trace("events::add_event(%p//%p):now_playing in %2.3f seconds", ev->now_playing, payload->track, (double)(now_playing_tv.tv_sec + now_playing_tv.tv_usec));
        event_add(ev->now_playing, &now_playing_tv);
    } else {
        _warn("events::add_event_failed(%p):now_playing", ev->scrobble);
    }

    return true;
}

static void remove_event_scrobble(struct events *ev)
{
    if (NULL == ev->scrobble) { return; }

    _trace("events::remove_event(%p):scrobble", ev->scrobble);

    if (NULL != ev->scrobble) {
        event_free(ev->scrobble);
        ev->scrobble = NULL;
    }
}

struct scrobble_payload *scrobble_payload_new(struct scrobbler *scrobbler, struct mpris_player *player, struct events *ev)
{
    struct scrobble_payload *p = calloc(1, sizeof(struct scrobble_payload));

    p->events = ev;
    p->scrobbler = scrobbler;
    p->player = player;

    return p;
}

void scrobble_payload_free(struct scrobble_payload *p)
{
    if (NULL == p) { return; }

    if (NULL != p->events) {
        remove_event_scrobble(p->events);
    }

    free(p);
}

static void send_scrobble(evutil_socket_t fd, short event, void *data)
{
    if (NULL == data) { return; }
    if (fd) { fd = 0; }
    if (event) { event = 0; }

    struct scrobble_payload *state = data;
    _trace("events::triggered(%p):scrobble", state->events->scrobble);

    state->player->queue_length -= scrobbles_consume_queue(state->scrobbler, state->player->queue, state->player->queue_length);
    _debug("events::new_queue_length: %zu", state->player->queue_length);

    scrobble_payload_free(state);
}

static bool add_event_scrobble(struct state *state, struct scrobble *track)
{
    if (NULL == state) { return false; }
    if (NULL == track) { return false; }
    if (NULL == state->events) { return false; }

    struct timeval scrobble_tv = {.tv_sec = 0, .tv_usec = 0};
    struct events *ev = state->events;

    if (NULL == state->player) { return false; }

#if 0
    scrobble_zero(ev->scrobble);
    scrobble_copy(ev->scrobble, track);
#endif

    if (NULL != ev->scrobble) { remove_event_scrobble(ev); }

    ev->scrobble = calloc(1, sizeof(struct event));
    if (NULL == ev->scrobble) { return false; }

    // Initalize timed event for scrobbling
    // TODO(marius): Split scrobbling into two events:
    //               1. Actually add the current track to the top of the queue in length / 2 or 4 minutes, whichever comes first
    //               2. Process the queue and call APIs with the current queue
    struct scrobble_payload *payload = scrobble_payload_new(state->scrobbler, state->player, state->events);
    if (NULL == payload) { return false; }

    if (event_assign(ev->scrobble, ev->base, -1, EV_PERSIST, send_scrobble, payload) == 0) {
        scrobble_tv.tv_sec = track->length / 2;
        _trace("events::add_event(%p):scrobble in %2.3f seconds", ev->scrobble, (double)scrobble_tv.tv_sec);
        event_add(ev->scrobble, &scrobble_tv);
    } else {
        _warn("events::add_event_failed(%p):scrobble", ev->scrobble);
    }

    return true;
}

static inline void mpris_event_clear(struct mpris_event *ev)
{
    ev->playback_status_changed = false;
    ev->volume_changed = false;
    ev->track_changed = false;
    ev->position_changed = false;
    _trace2("mem::zeroed::mpris_event");
}

static inline bool mpris_event_happened(const struct mpris_event *what_happened)
{
    return (
        what_happened->playback_status_changed ||
        what_happened->volume_changed ||
        what_happened->track_changed ||
        what_happened->position_changed
    );
}

void state_loaded_properties(struct state *state, struct mpris_properties *properties, const struct mpris_event *what_happened)
{
    if (!mpris_event_happened(what_happened)) {
        _debug("events::loaded_properties: nothing found");
        return;
    }
    debug_event(what_happened);

    struct scrobble *scrobble = scrobble_new();
    if (!load_scrobble(scrobble, properties)) {
        _warn("events::unable_to_load_scrobble[%p]");
        scrobble_free(scrobble);
        return;
    }
    if (!now_playing_is_valid(scrobble)) {
        _warn("events::invalid_loaded_scrobble[%p]");
    }

    mpris_properties_copy(state->player->current, properties);

    if (NULL != state->events->now_playing) { remove_event_now_playing(state->events); }
    if (NULL != state->events->scrobble) { remove_event_scrobble(state->events); }

    bool scrobble_added = false;
    bool now_playing_added = false;
    if(what_happened->playback_status_changed && !what_happened->track_changed) {
        if (what_happened->player_state == playing) {
            now_playing_added = add_event_now_playing(state, scrobble);
            scrobble_added = scrobbles_append(state->player, scrobble);
        }
    }
    if(what_happened->track_changed) {
        if (what_happened->player_state == playing) {
            if (!now_playing_added) {
                now_playing_added = add_event_now_playing(state, scrobble);
            }
            if (!scrobble_added) {
                scrobble_added = scrobbles_append(state->player, scrobble);
            }
            add_event_scrobble(state, scrobble);
        }
    }
    if (what_happened->volume_changed) {
        // trigger volume_changed event
    }
    if (what_happened->position_changed) {
        // trigger position event
    }
    scrobble_free(scrobble);

    mpris_event_clear(state->player->changed);
}

#endif // MPRIS_SCROBBLER_SEVENTS_H
