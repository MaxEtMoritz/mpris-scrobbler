/**
#include "sstrings.h"
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_SDBUS_H
#define MPRIS_SCROBBLER_SDBUS_H

#include <dbus/dbus.h>
#include <event.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#if DBUS_MAJOR_VERSION <= 1 && DBUS_MINOR_VERSION <= 8
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#define dbus_message_iter_get_element_count(A) dbus_message_iter_get_array_len(A)
#endif

#if DEBUG
#define LOCAL_NAME                 "org.mpris.scrobbler-debug"
#else
#define LOCAL_NAME                 "org.mpris.scrobbler"
#endif
#define MPRIS_PLAYER_NAMESPACE     "org.mpris.MediaPlayer2"
#define MPRIS_PLAYER_PATH          "/org/mpris/MediaPlayer2"
#define MPRIS_PLAYER_INTERFACE     "org.mpris.MediaPlayer2.Player"
#define MPRIS_METHOD_NEXT          "Next"
#define MPRIS_METHOD_PREVIOUS      "Previous"
#define MPRIS_METHOD_PLAY          "Play"
#define MPRIS_METHOD_PAUSE         "Pause"
#define MPRIS_METHOD_STOP          "Stop"
#define MPRIS_METHOD_PLAY_PAUSE    "PlayPause"

#define MPRIS_PNAME_PLAYBACKSTATUS "PlaybackStatus"
#define MPRIS_PNAME_CANCONTROL     "CanControl"
#define MPRIS_PNAME_CANGONEXT      "CanGoNext"
#define MPRIS_PNAME_CANGOPREVIOUS  "CanGoPrevious"
#define MPRIS_PNAME_CANPLAY        "CanPlay"
#define MPRIS_PNAME_CANPAUSE       "CanPause"
#define MPRIS_PNAME_CANSEEK        "CanSeek"
#define MPRIS_PNAME_SHUFFLE        "Shuffle"
#define MPRIS_PNAME_POSITION       "Position"
#define MPRIS_PNAME_VOLUME         "Volume"
#define MPRIS_PNAME_LOOPSTATUS     "LoopStatus"
#define MPRIS_PNAME_METADATA       "Metadata"

#define MPRIS_ARG_PLAYER_IDENTITY  "Identity"

#define DBUS_PATH                  "/"
#define DBUS_METHOD_LIST_NAMES     "ListNames"
#define DBUS_METHOD_GET_ALL        "GetAll"
#define DBUS_METHOD_GET            "Get"
#define DBUS_METHOD_GET_ID         "GetId"

#define MPRIS_METADATA_BITRATE      "bitrate"
#define MPRIS_METADATA_ART_URL      "mpris:artUrl"
#define MPRIS_METADATA_LENGTH       "mpris:length"
#define MPRIS_METADATA_TRACKID      "mpris:trackid"
#define MPRIS_METADATA_ALBUM        "xesam:album"
#define MPRIS_METADATA_ALBUM_ARTIST "xesam:albumArtist"
#define MPRIS_METADATA_ARTIST       "xesam:artist"
#define MPRIS_METADATA_COMMENT      "xesam:comment"
#define MPRIS_METADATA_TITLE        "xesam:title"
#define MPRIS_METADATA_TRACK_NUMBER "xesam:trackNumber"
#define MPRIS_METADATA_URL          "xesam:url"
#define MPRIS_METADATA_GENRE        "xesam:genre"
#define MPRIS_METADATA_YEAR         "year"

#define MPRIS_METADATA_MUSICBRAINZ_TRACK_ID         "xesam:musicBrainzTrackID"
#define MPRIS_METADATA_MUSICBRAINZ_ALBUM_ID         "xesam:musicBrainzAlbumID"
#define MPRIS_METADATA_MUSICBRAINZ_ARTIST_ID        "xesam:musicBrainzArtistID"
#define MPRIS_METADATA_MUSICBRAINZ_ALBUMARTIST_ID   "xesam:musicBrainzAlbumArtistID"

#define DBUS_SIGNAL_PROPERTIES_CHANGED   "PropertiesChanged"
#define DBUS_SIGNAL_NAME_OWNER_CHANGED   "NameOwnerChanged"

// The default timeout leads to hangs when calling
//   certain players which don't seem to reply to MPRIS methods
#define DBUS_CONNECTION_TIMEOUT    100 //ms

#if 0
static DBusMessage *call_dbus_method(DBusConnection *conn, char *destination, char *path, char *interface, char *method)
{
    if (NULL == conn) { return NULL; }
    if (NULL == destination) { return NULL; }

    DBusMessage *msg;
    DBusPendingCall *pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return NULL; }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // free message
    dbus_message_unref(msg);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage *reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);

    // free the pending message handle
    dbus_pending_call_unref(pending);
    // free message
    dbus_message_unref(msg);

    return reply;

_unref_message_err:
    {
        dbus_message_unref(msg);
    }
    return NULL;
}
#endif

static void extract_double_var(DBusMessageIter *iter, double *result, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_DOUBLE == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, result);
        _trace2("\tdbus::loaded_basic_double[%p]: %f", result, *result);
    }
    return;
}

static int extract_string_array_var(DBusMessageIter *iter, char result[MAX_PROPERTY_COUNT][MAX_PROPERTY_LENGTH], DBusError *err)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(err, "iter_should_be_variant", "This message iterator must be have variant type");
        return 0;
    }

    size_t read_count = 0;
    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&variantIter)) {
        DBusMessageIter arrayIter;
        dbus_message_iter_recurse(&variantIter, &arrayIter);

        //size_t count = dbus_message_iter_get_element_count(&variantIter);
        while (read_count < MAX_PROPERTY_COUNT) {
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayIter)) {
                char *temp = NULL;
                dbus_message_iter_get_basic(&arrayIter, &temp);
                if (NULL == temp || strlen(temp) == 0) { continue; }

                memcpy(result[read_count], temp, strlen(temp));
            }
            read_count++;
            if (!dbus_message_iter_has_next(&arrayIter)) {
                break;
            }
            dbus_message_iter_next(&arrayIter);
        }
    }
    //int res_count = arrlen(result);
    for (int i = read_count - 1; i >= 0; i--) {
        _trace2("\tdbus::loaded_array_of_strings[%zd//%zd//%p]: %s", i, read_count, (result)[i], (result)[i]);
    }
    return read_count;
}

static void extract_string_var(DBusMessageIter *iter, char *result, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (
        DBUS_TYPE_OBJECT_PATH == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&variantIter)
    ) {
        char *temp = NULL;
        dbus_message_iter_get_basic(&variantIter, &temp);
        if (strlen(temp) > 0) {
            memcpy(result, temp, strlen(temp));
        }
        _trace2("\tdbus::loaded_basic_string[%p]: %s", result, result);
    }
}

static void extract_int32_var(DBusMessageIter *iter, int32_t *result, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, result);
        _trace2("\tdbus::loaded_basic_int32[%p]: %" PRId32, result, *result);
    }
    return;
}

static void extract_int64_var(DBusMessageIter *iter, int64_t *result, DBusError *error)
{
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (
        DBUS_TYPE_UINT64 == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_INT64 == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type(&variantIter) ||
        DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&variantIter)
    ) {
        dbus_message_iter_get_basic(&variantIter, result);
#if 0
        _trace("\tdbus::loaded_basic_int64[%p]: %" PRId64, result, *result);
#endif
    }
    return;
}

static void extract_boolean_var(DBusMessageIter *iter, bool *result, DBusError *error)
{
    dbus_bool_t res = false;
    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(error, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);

    if (DBUS_TYPE_BOOLEAN == dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_message_iter_get_basic(&variantIter, &res);
        _trace2("\tdbus::loaded_basic_bool[%p]: %s", result, res ? "true" : "false");
    }
    *result = (res == 1);
    return;
}

static void load_metadata(DBusMessageIter *iter, struct mpris_metadata *track, struct mpris_event *changes)
{
    if (NULL == track) { return; }
    DBusError err;
    dbus_error_init(&err);

    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(iter)) {
        dbus_set_error_const(&err, "iter_should_be_variant", "This message iterator must be have variant type");
        return;
    }

    DBusMessageIter variantIter;
    dbus_message_iter_recurse(iter, &variantIter);
    if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&variantIter)) {
        dbus_set_error_const(&err, "variant_should_be_array", "This variant reply message must have array content");
        return;
    }
    DBusMessageIter arrayIter;
    dbus_message_iter_recurse(&variantIter, &arrayIter);
    _debug("mpris::loading_metadata");
    unsigned short max = 0;
    while (true && max++ < 50) {
        char *key = NULL;
        if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayIter)) {
            DBusMessageIter dictIter;
            dbus_message_iter_recurse(&arrayIter, &dictIter);
            if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictIter)) {
                dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have key");
            }
            dbus_message_iter_get_basic(&dictIter, &key);

            if (!dbus_message_iter_has_next(&dictIter)) {
                continue;
            }
            dbus_message_iter_next(&dictIter);
            if (!strncmp(key, MPRIS_METADATA_BITRATE, strlen(MPRIS_METADATA_BITRATE))) {
                extract_int32_var(&dictIter, (int32_t*)&track->bitrate, &err);
                _debug("  loaded::metadata::bitrate: %" PRId32, track->bitrate);
            } else if (!strncmp(key, MPRIS_METADATA_ART_URL, strlen(MPRIS_METADATA_ART_URL))) {
                extract_string_var(&dictIter, track->art_url, &err);
                _debug("  loaded::metadata::art_url: %s", track->art_url);
            } else if (!strncmp(key, MPRIS_METADATA_LENGTH, strlen(MPRIS_METADATA_LENGTH))) {
                extract_int64_var(&dictIter, (int64_t*)&track->length, &err);
                _debug("  loaded::metadata::length: %" PRId64, track->length);
            } else if (!strncmp(key, MPRIS_METADATA_TRACKID, strlen(MPRIS_METADATA_TRACKID))) {
                extract_string_var(&dictIter, track->track_id, &err);
                _debug("  loaded::metadata::track_id: %s", track->track_id);
            } else if (!strncmp(key, MPRIS_METADATA_ALBUM, strlen(MPRIS_METADATA_ALBUM)) && strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST)) ) {
                extract_string_var(&dictIter, track->album, &err);
                _debug("  loaded::metadata::album: %s", track->album);
            } else if (!strncmp(key, MPRIS_METADATA_ALBUM_ARTIST, strlen(MPRIS_METADATA_ALBUM_ARTIST))) {
                int cnt = extract_string_array_var(&dictIter, track->album_artist, &err);
                print_array1(track->album_artist, cnt, log_debug, "  loaded::metadata::album_artist");
            } else if (!strncmp(key, MPRIS_METADATA_ARTIST, strlen(MPRIS_METADATA_ARTIST))) {
                int cnt = extract_string_array_var(&dictIter, track->artist, &err);
                print_array1(track->artist, cnt, log_debug, "  loaded::metadata::artist");
            } else if (!strncmp(key, MPRIS_METADATA_COMMENT, strlen(MPRIS_METADATA_COMMENT))) {
                int cnt = extract_string_array_var(&dictIter, track->comment, &err);
                print_array1(track->comment, cnt, log_debug, "  loaded::metadata::comment");
            } else if (!strncmp(key, MPRIS_METADATA_TITLE, strlen(MPRIS_METADATA_TITLE))) {
                extract_string_var(&dictIter, track->title, &err);
                _debug("  loaded::metadata::title: %s", track->title);
            } else if (!strncmp(key, MPRIS_METADATA_TRACK_NUMBER, strlen(MPRIS_METADATA_TRACK_NUMBER))) {
                extract_int32_var(&dictIter, (int32_t*)&track->track_number, &err);
                _debug("  loaded::metadata::track_number: %2" PRId32, track->track_number);
            } else if (!strncmp(key, MPRIS_METADATA_URL, strlen(MPRIS_METADATA_URL))) {
                extract_string_var(&dictIter, track->url, &err);
                _debug("  loaded::metadata::url: %s", track->url);
            } else if (!strncmp(key, MPRIS_METADATA_GENRE, strlen(MPRIS_METADATA_GENRE))) {
                int cnt = extract_string_array_var(&dictIter, track->genre, &err);
                print_array1(track->genre, cnt, log_debug, "  loaded::metadata::genre");
            } else if (!strncmp(key, MPRIS_METADATA_MUSICBRAINZ_TRACK_ID, strlen(MPRIS_METADATA_MUSICBRAINZ_TRACK_ID))) {
                // check for music brainz tags - players supporting this: Rhythmbox
                extract_string_var(&dictIter, track->mb_track_id, &err);
                _debug("  loaded::metadata::musicbrainz::track_id: %s", track->mb_track_id);
            } else if (!strncmp(key, MPRIS_METADATA_MUSICBRAINZ_ALBUM_ID, strlen(MPRIS_METADATA_MUSICBRAINZ_ALBUM_ID))) {
                extract_string_var(&dictIter, track->mb_album_id, &err);
                _debug("  loaded::metadata::musicbrainz::album_id: %s", track->mb_album_id);
            } else if (!strncmp(key, MPRIS_METADATA_MUSICBRAINZ_ARTIST_ID, strlen(MPRIS_METADATA_MUSICBRAINZ_ARTIST_ID))) {
                extract_string_var(&dictIter, track->mb_artist_id, &err);
                _debug("  loaded::metadata::musicbrainz::artist_id: %s", track->mb_artist_id);
            } else if (!strncmp(key, MPRIS_METADATA_MUSICBRAINZ_ALBUMARTIST_ID, strlen(MPRIS_METADATA_MUSICBRAINZ_ALBUMARTIST_ID))) {
                extract_string_var(&dictIter, track->mb_album_artist_id, &err);
                _debug("  loaded::metadata::musicbrainz::album_artist_id: %s", track->mb_album_artist_id);
            }
            changes->track_changed = true;
            if (dbus_error_is_set(&err)) {
                _error("dbus::value_error: %s, %s", key, err.message);
                dbus_error_free(&err);
            }
        }
        if (!dbus_message_iter_has_next(&arrayIter)) {
            break;
        }
        dbus_message_iter_next(&arrayIter);
    }
    // TODO(marius): this is not ideal, as we don't know at this level if the track has actually started now
    //               or earlier.
    track->timestamp = time(0);
    _debug("  loaded::metadata::timestamp: %zu", track->timestamp);
}

static void get_player_identity(DBusConnection *conn, const char *destination, char *identity)
{
    if (NULL == conn) { return; }
    if (NULL == destination) { return; }
    if (strncmp(MPRIS_PLAYER_NAMESPACE, destination, strlen(MPRIS_PLAYER_NAMESPACE)) != 0) { return; }

    const char *interface = DBUS_INTERFACE_PROPERTIES;
    const char *method = DBUS_METHOD_GET;
    const char *path = MPRIS_PLAYER_PATH;
    const char *arg_interface = MPRIS_PLAYER_NAMESPACE;
    const char *arg_identity = MPRIS_ARG_PLAYER_IDENTITY;

    // create a new method call and check for errors
    DBusMessage *msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return /*NULL*/; }

    DBusMessageIter params;
    // append interface we want to get the property from
    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_interface)) {
        goto _unref_message_err;
    }

    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_identity)) {
        goto _unref_message_err;
    }

    DBusPendingCall *pending;
    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);
    DBusMessage *reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) { goto _unref_pending_err; }

    DBusMessageIter rootIter;

    DBusError err = {0};
    dbus_error_init(&err);
    if (dbus_message_iter_init(reply, &rootIter)) {
        extract_string_var(&rootIter, identity, &err);
    }
    if (dbus_error_is_set(&err)) {
        _error("  mpris::failed_to_load_player_name: %s", err.message);
        dbus_error_free(&err);
    } else {
        if (strlen(identity) == 0) {
            _error("  mpris::empty_player_name");
        } else {
            _trace("  mpris::player_name: %s", identity);
        }
    }

    dbus_message_unref(reply);
_unref_pending_err:
    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);
    return;
}

#if 0
static char *get_dbus_string_scalar(DBusMessage *message)
{
    if (NULL == message) { return NULL; }
    char *status = NULL;

    DBusMessageIter rootIter;
    if (dbus_message_iter_init(message, &rootIter) &&
        DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&rootIter)) {

        dbus_message_iter_get_basic(&rootIter, &status);
    }

    return status;
}
#endif

int load_player_namespaces(DBusConnection *conn, struct mpris_player *players, int max_player_count)
{
    if (NULL == conn) { return -1; }

    const char *method = DBUS_METHOD_LIST_NAMES;
    const char *destination = DBUS_INTERFACE_DBUS;
    const char *path = DBUS_PATH;
    const char *interface = DBUS_INTERFACE_DBUS;
    const char *mpris_namespace = MPRIS_PLAYER_NAMESPACE;

    DBusMessage *msg;
    DBusPendingCall *pending;

    // create a new method call and check for errors
    msg = dbus_message_new_method_call(destination, path, interface, method);
    if (NULL == msg) { return -1; }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);

    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusMessage *reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) { goto _unref_pending_err; }

    int count = 0;
    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) &&
        DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        DBusMessageIter arrayElementIter;

        dbus_message_iter_recurse(&rootIter, &arrayElementIter);
        while (dbus_message_iter_has_next(&arrayElementIter)) {
            if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                char *value = NULL;
                dbus_message_iter_get_basic(&arrayElementIter, &value);
                if (strncmp(value, mpris_namespace, strlen(mpris_namespace)) == 0) {
                    strncpy(players[count].mpris_name, value, MAX_PROPERTY_LENGTH);
                    count++;
                }
            }
            dbus_message_iter_next(&arrayElementIter);
        }
    }
    dbus_message_unref(reply);
_unref_pending_err:
    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);
    return count;
}

static void load_properties(DBusMessageIter *rootIter, struct mpris_properties *properties, struct mpris_event *changes)
{
    if (NULL == properties) { return; }
    if (NULL == rootIter) { return; }

    DBusError err;
    dbus_error_init(&err);
    unsigned short max = 0;

    if (DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(rootIter)) {
        DBusMessageIter arrayElementIter;

        dbus_message_iter_recurse(rootIter, &arrayElementIter);
        while (true && max++ < 200) {
            char *key = NULL;
            if (DBUS_TYPE_DICT_ENTRY == dbus_message_iter_get_arg_type(&arrayElementIter)) {
                DBusMessageIter dictIter;
                dbus_message_iter_recurse(&arrayElementIter, &dictIter);
                if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dictIter)) {
                    dbus_set_error_const(&err, "missing_key", "This message iterator doesn't have key");
                }
                dbus_message_iter_get_basic(&dictIter, &key);

                if (!dbus_message_iter_has_next(&dictIter)) {
                    continue;
                }
                dbus_message_iter_next(&dictIter);

                if (!strncmp(key, MPRIS_PNAME_CANCONTROL, strlen(MPRIS_PNAME_CANCONTROL))) {
                    extract_boolean_var(&dictIter, &properties->can_control, &err);
                    _debug("  loaded::can_control: %s", (properties->can_control ? "true" : "false"));
                } else if (!strncmp(key, MPRIS_PNAME_CANGONEXT, strlen(MPRIS_PNAME_CANGONEXT))) {
                    extract_boolean_var(&dictIter, &properties->can_go_next, &err);
                    _debug("  loaded::can_go_next: %s", (properties->can_go_next ? "true" : "false"));
                } else if (!strncmp(key, MPRIS_PNAME_CANGOPREVIOUS, strlen(MPRIS_PNAME_CANGOPREVIOUS))) {
                    extract_boolean_var(&dictIter, &properties->can_go_previous, &err);
                    _debug("  loaded::can_go_previous: %s", (properties->can_go_previous ? "true" : "false"));
                } else if (!strncmp(key, MPRIS_PNAME_CANPAUSE, strlen(MPRIS_PNAME_CANPAUSE))) {
                    extract_boolean_var(&dictIter, &properties->can_pause, &err);
                    _debug("  loaded::can_pause: %s", (properties->can_pause ? "true" : "false"));
                } else if (!strncmp(key, MPRIS_PNAME_CANPLAY, strlen(MPRIS_PNAME_CANPLAY))) {
                    extract_boolean_var(&dictIter, &properties->can_play, &err);
                    _debug("  loaded::can_play: %s", (properties->can_play ? "true" : "false"));
                } else if (!strncmp(key, MPRIS_PNAME_CANSEEK, strlen(MPRIS_PNAME_CANSEEK))) {
                    extract_boolean_var(&dictIter, &properties->can_seek, &err);
                    _debug("  loaded::can_seek: %s", (properties->can_seek ? "true" : "false"));
                } else if (!strncmp(key, MPRIS_PNAME_LOOPSTATUS, strlen(MPRIS_PNAME_LOOPSTATUS))) {
                    extract_string_var(&dictIter, properties->loop_status, &err);
                    _debug("  loaded::loop_status: %s", properties->loop_status);
                } else if (!strncmp(key, MPRIS_PNAME_PLAYBACKSTATUS, strlen(MPRIS_PNAME_PLAYBACKSTATUS))) {
                    extract_string_var(&dictIter, properties->playback_status, &err);
                    _debug("  loaded::playback_status: %s", properties->playback_status);
                    changes->playback_status_changed = true;
                    changes->player_state = get_mpris_playback_status(properties);
                } else if (!strncmp(key, MPRIS_PNAME_POSITION, strlen(MPRIS_PNAME_POSITION))) {
                    extract_int64_var(&dictIter, &properties->position, &err);
                    changes->position_changed = true;
                    _debug("  loaded::position: %" PRId64, properties->position);
                } else if (!strncmp(key, MPRIS_PNAME_SHUFFLE, strlen(MPRIS_PNAME_SHUFFLE))) {
                    extract_boolean_var(&dictIter, &properties->shuffle, &err);
                    _debug("  loaded::shuffle: %s", (properties->shuffle ? "yes" : "no"));
                } else if (!strncmp(key, MPRIS_PNAME_VOLUME, strlen(MPRIS_PNAME_VOLUME))) {
                    extract_double_var(&dictIter, &properties->volume, &err);
                    changes->volume_changed = true;
                    _debug("  loaded::volume: %.2f", properties->volume);
                } else if (!strncmp(key, MPRIS_PNAME_METADATA, strlen(MPRIS_PNAME_METADATA))) {
                    load_metadata(&dictIter, &properties->metadata, changes);
                }
                if (dbus_error_is_set(&err)) {
                    _error("dbus::value_error: %s", err.message);
                    dbus_error_free(&err);
                }
            }
            if (!dbus_message_iter_has_next(&arrayElementIter)) {
                break;
            }
            dbus_message_iter_next(&arrayElementIter);
        }
        if (properties->metadata.timestamp > 0) {
            // TODO(marius): more uglyness - subtract play time from the start time
            properties->metadata.timestamp -= (unsigned)(properties->position / 1000000.0f);
        }
#if 0
    } else {
        if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(rootIter)) {
            char *key;
            dbus_message_iter_get_basic(rootIter, &key);

            _trace("\tUncoming key %s", key);
        }
#endif
    }
}

void get_mpris_properties(DBusConnection *conn, struct mpris_player *player)
{
    if (NULL == conn) { return; }
    if (NULL == player) { return; }

    struct mpris_properties *properties = &player->properties;
    if (NULL == properties) { return; }
    struct mpris_event *changes = &player->changed;
    if (NULL == changes) { return; }

    DBusMessage *msg;
    DBusPendingCall *pending;
    DBusMessageIter params;

    const char *interface = DBUS_INTERFACE_PROPERTIES;
    const char *method = DBUS_METHOD_GET_ALL;
    const char *path = MPRIS_PLAYER_PATH;
    const char *arg_interface = MPRIS_PLAYER_INTERFACE;

    const char *identity = player->mpris_name;
    if (strlen(identity)) {
        identity = player->bus_id;
    }
    // create a new method call and check for errors
    msg = dbus_message_new_method_call(identity, path, interface, method);
    if (NULL == msg) { return; }

    // append interface we want to get the property from
    dbus_message_iter_init_append(msg, &params);
    if (!dbus_message_iter_append_basic(&params, DBUS_TYPE_STRING, &arg_interface)) {
        goto _unref_message_err;
    }

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply (conn, msg, &pending, DBUS_CONNECTION_TIMEOUT)) {
        goto _unref_message_err;
    }
    if (NULL == pending) {
        goto _unref_message_err;
    }
    dbus_connection_flush(conn);
    // block until we receive a reply
    dbus_pending_call_block(pending);

    DBusError err;
    dbus_error_init(&err);

    DBusMessage *reply;
    // get the reply message
    reply = dbus_pending_call_steal_reply(pending);
    if (NULL == reply) {
        goto _unref_pending_err;
    }
    const char *bus_id = dbus_message_get_sender(reply);
    if (NULL != bus_id) {
        memcpy(&player->bus_id, bus_id, strlen(bus_id));
        memcpy(&player->changed.sender_bus_id, bus_id, strlen(bus_id));
    }
    DBusMessageIter rootIter;
    if (dbus_message_iter_init(reply, &rootIter) && DBUS_TYPE_ARRAY == dbus_message_iter_get_arg_type(&rootIter)) {
        _debug("mpris::loading_properties");
        //mpris_properties_zero(properties, true);
        load_properties(&rootIter, properties, changes);
    }

    if (dbus_error_is_set(&err)) {
        _error("mpris::loading_properties_error: %s", err.message);
        dbus_error_free(&err);
    }
    dbus_message_unref(reply);
_unref_pending_err:
    // free the pending message handle
    dbus_pending_call_unref(pending);
_unref_message_err:
    // free message
    dbus_message_unref(msg);
}

#if 0
void check_for_player(DBusConnection *conn, char **destination, time_t *last_load_time)
{
    if (NULL == conn) { return; }
    time_t current_time;
    bool valid_incoming_player = mpris_player_is_valid(destination);
    time(&current_time);

    if (difftime(current_time, *last_load_time) < 10 && valid_incoming_player) { return; }

    //get_player_namespace(conn, destination);
    time(last_load_time);

    if (mpris_player_is_valid(destination)) {
        if (!valid_incoming_player) { _debug("mpris::found_player: %s", *destination); }
    } else {
        if (valid_incoming_player) { _debug("mpris::lost_player"); }
    }
}
#endif

static int load_player_identity_from_message(DBusMessage *msg, struct mpris_player *player)
{
    if (NULL == msg) {
        _warn("dbus::invalid_signal_message(%p)", msg);
        return false;
    }
    if (NULL == player) {
        _warn("dbus::invalid_player_target(%p)", player);
        return false;
    }
    DBusError err;
    dbus_error_init(&err);

    int loaded = 0;
    char *initial = NULL;
    char *old_name = NULL;
    char *new_name = NULL;
    dbus_message_get_args(msg, &err,
        DBUS_TYPE_STRING, &initial,
        DBUS_TYPE_STRING, &old_name,
        DBUS_TYPE_STRING, &new_name,
        DBUS_TYPE_INVALID);
    if (dbus_error_is_set(&err)) {
        _error("mpris::loading_args: %s", err.message);
        dbus_error_free(&err);
        return 0;
    }

    int len_initial = strlen(initial);
    int len_old = strlen(old_name);
    int len_new = strlen(new_name);
    if (strncmp(initial, MPRIS_PLAYER_NAMESPACE, strlen(MPRIS_PLAYER_NAMESPACE)) == 0) {
        memcpy(player->mpris_name, initial, len_initial);
        if (len_new == 0 && len_old > 0) {
            loaded = -1;
            memcpy(player->bus_id, old_name, len_old);
        }
        if (len_new > 0 && len_old == 0) {
            loaded = 1;
            memcpy(player->bus_id, new_name, len_new);
        }
#if 0
        _trace2("message:%s %d %s -> %s %s::%s",
            dbus_message_get_member(msg),
            dbus_message_get_type(msg),
            dbus_message_get_sender(msg),
            dbus_message_get_destination(msg),
            dbus_message_get_path(msg),
            dbus_message_get_interface(msg)
        );
#endif
    }

    return loaded;
}

static bool load_properties_from_message(DBusMessage *msg, struct mpris_properties *data, struct mpris_event *changes)
{
    if (NULL == msg) {
        _warn("dbus::invalid_signal_message(%p)", msg);
        return false;
    }
    char *interface = NULL;
    if (NULL == data) {
        _warn("dbus::invalid_properties_target(%p)", data);
        return false;
    }
    const char *bus_id = dbus_message_get_sender(msg);
    if (NULL != bus_id) {
        memcpy(&changes->sender_bus_id, bus_id, strlen(bus_id));
    }
    struct mpris_properties *properties = mpris_properties_new();
    DBusMessageIter args;
    // read the parameters
    const char *signal_name = dbus_message_get_member(msg);
    if (!dbus_message_iter_init(msg, &args)) {
        _warn("dbus::missing_signal_args: %s", signal_name);
    }
    _debug("dbus::signal(%p): %s:%s:%s.%s", msg, changes->sender_bus_id, dbus_message_get_path(msg), dbus_message_get_interface(msg), signal_name);
    // skip first arg and then load properties from all the remaining ones
    if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args)) {
        dbus_message_iter_get_basic(&args, &interface);
    }
    if (NULL == interface) {
        _warn("dbus::invalid_interface(%p)", interface);
        goto _free_properties;
    }
    dbus_message_iter_next(&args);

    bool loaded = false;
    if (strncmp(interface, MPRIS_PLAYER_NAMESPACE, strlen(MPRIS_PLAYER_NAMESPACE)) == 0) {
        _debug("mpris::loading_properties");
        //mpris_properties_zero(data, true);
        unsigned short max = 0;
        while (true && max++ < 50) {
            load_properties(&args, properties, changes);
            if (!dbus_message_iter_has_next(&args)) {
                break;
            }
            dbus_message_iter_next(&args);
        }
    }
    if (!mpris_properties_equals(properties, data)) {
        mpris_properties_copy(data, properties);
        loaded = true;
    }
_free_properties:
    mpris_properties_free(properties);
    return loaded;
}

static void dispatch(int fd, short ev, void *data)
{
    DBusConnection *conn = data;
    while (dbus_connection_get_dispatch_status(conn) == DBUS_DISPATCH_DATA_REMAINS) {
        dbus_connection_dispatch(conn);
    }
    _trace("dbus::dispatching fd=%d, data=%p ev=%d", fd, (void*)data, ev);
}

static void handle_dispatch_status(DBusConnection *conn, DBusDispatchStatus status, void *data)
{
    struct state *s = data;
    if (status == DBUS_DISPATCH_DATA_REMAINS) {
        struct timeval tv = { .tv_sec = 0, .tv_usec = 100000, };

        event_add (s->events.dispatch, &tv);
        //_trace("dbus::new_dispatch_status(%p): %s", (void*)conn, "DATA_REMAINS");
        //_trace("events::add_event(%p):dispatch", s->events->dispatch);
    }
    if (status == DBUS_DISPATCH_COMPLETE) {
        _trace("dbus::new_dispatch_status(%p): %s", (void*)conn, "COMPLETE");

#if 0
        if (strlen(player->properties->player_name) == 0) {
            // we have cleared the properties as we failed to load_properties_from_message
            state_loaded_properties(conn, player, player->properties, player->changed);
        }
#endif
    }
    if (status == DBUS_DISPATCH_NEED_MEMORY) {
        _error("dbus::new_dispatch_status(%p): %s", (void*)conn, "OUT_OF_MEMORY");
    }
}

static void handle_watch(int fd, short events, void *data)
{
    struct state *state = data;
    DBusWatch *watch = state->dbus->watch;

    unsigned flags = 0;
    if (events & EV_READ) { flags |= DBUS_WATCH_READABLE; }

    if (dbus_watch_handle(watch, flags) == false) {
       _error("dbus::handle_event_failed: fd=%d, watch=%p ev=%d", fd, (void*)watch, events);
    } else {
        //_trace("dbus::handle_event: fd=%d, watch=%p ev=%d", fd, (void*)watch, events);
    }

    handle_dispatch_status(state->dbus->conn, DBUS_DISPATCH_DATA_REMAINS, data);
}

static unsigned add_watch(DBusWatch *watch, void *data)
{
    if (!dbus_watch_get_enabled(watch)) { return true;}

    struct state *state = data;
    state->dbus->watch = watch;

    int fd = dbus_watch_get_unix_fd(watch);
    unsigned flags = dbus_watch_get_flags(watch);

    short cond = EV_PERSIST;
    if (flags & DBUS_WATCH_READABLE) { cond |= EV_READ; }

    struct event *event = event_new(state->events.base, fd, cond, handle_watch, state);

    if (NULL == event) { return false; }
    event_add(event, NULL);
    _trace("events::add_event(%p):add_watch", event);

    dbus_watch_set_data(watch, event, NULL);

    _trace("dbus::add_watch: watch=%p data=%p", (void*)watch, data);
    return true;
}

static void remove_watch(DBusWatch *watch, void *data)
{
    if (!dbus_watch_get_enabled(watch)) { return; }

    struct event *event = dbus_watch_get_data(watch);

    _trace("events::del_event(%p):remove_watch data=%p", (void*)event, data);
    if (NULL != event) { event_free(event); }

    dbus_watch_set_data(watch, NULL, NULL);

    _trace("dbus::removed_watch: watch=%p data=%p", (void*)watch, data);
}

static void toggle_watch(DBusWatch *watch, void *data)
{
    _trace("dbus::toggle_watch watch=%p data=%p", (void*)watch, data);
    if (dbus_watch_get_enabled(watch)) {
        add_watch(watch, data);
    } else {
        remove_watch(watch, data);
    }
}

static DBusHandlerResult add_filter(DBusConnection *conn, DBusMessage *message, void *data)
{
    _trace2("dbus::filter(%p:%p):%s %d %s -> %s %s::%s",
           conn,
           message,
           dbus_message_get_member(message),
           dbus_message_get_type(message),
           dbus_message_get_sender(message),
           dbus_message_get_destination(message),
           dbus_message_get_path(message),
           dbus_message_get_interface(message)
    );
    struct state *s = data;
    if (dbus_message_is_signal(message, DBUS_INTERFACE_PROPERTIES, DBUS_SIGNAL_PROPERTIES_CHANGED)) {
        if (!strncmp(dbus_message_get_path(message), MPRIS_PLAYER_PATH, strlen(MPRIS_PLAYER_PATH))) {
            struct mpris_properties *properties = mpris_properties_new();
            struct mpris_event changed = {0};
            if (load_properties_from_message(message, properties, &changed)) {
                for (int i = 0; i < s->player_count; i++) {
                    struct mpris_player *player = &(s->players[i]);
                    if (!strncmp(player->bus_id, changed.sender_bus_id, strlen(changed.sender_bus_id))) {
                        mpris_properties_copy(&player->properties, properties);
                    }
                }
            }
            debug_event(&changed);
            mpris_properties_free(properties);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    } else if (dbus_message_is_signal(message, DBUS_INTERFACE_DBUS, DBUS_SIGNAL_NAME_OWNER_CHANGED)) {
        // @todo(marius): see if the new dbus client is a mpris media player
        struct mpris_player player = {0};
        int loaded_or_deleted = load_player_identity_from_message(message, &player);
        if (loaded_or_deleted > 0) {
            // player was opened
            mpris_player_init(s->dbus, &player, &s->events, s->scrobbler, s->config->ignore_players, s->config->ignore_players_count);
            memcpy(&s->players[s->player_count], &player, sizeof(struct mpris_player));
            s->player_count++;
            _debug("mpris_player::opened[%d]: %s%s", s->player_count, player.mpris_name, player.bus_id);
        } else if (loaded_or_deleted < 0) {
            // player was closed
            for (int i = 0; i < s->player_count; i++) {
                struct mpris_player *pl = &(s->players[i]);
                if (strncmp(pl->bus_id, player.bus_id, strlen(player.bus_id)) == 0) {
                    s->player_count--;
                    mpris_player_free(pl);
                }
            }
            _debug("mpris_player::closed[%d]: %s%s", s->player_count, player.mpris_name, player.bus_id);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    } else {
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void dbus_close(struct state *state)
{
    if (NULL == state->dbus) { return; }
    if (NULL != state->dbus->conn) {
        _trace2("mem::free::dbus_connection(%p)", state->dbus->conn);
        dbus_connection_flush(state->dbus->conn);
        dbus_connection_close(state->dbus->conn);
        dbus_connection_unref(state->dbus->conn);
    }
    free(state->dbus);
}

struct dbus *dbus_connection_init(struct state *state)
{
    DBusConnection *conn = NULL;
    state->dbus = calloc(1, sizeof(struct dbus));
    if (NULL == state->dbus) {
        _error("dbus::failed_to_init_libdbus");
        goto _cleanup;
    }
    DBusError err;
    dbus_error_init(&err);

    conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (NULL == conn) {
        _error("dbus::unable_to_get_on_bus");
        goto _cleanup;
    } else {
        _trace2("mem::inited_dbus_connection(%p)", conn);
    }

    unsigned int flags = DBUS_NAME_FLAG_DO_NOT_QUEUE;
    int name_acquired = dbus_bus_request_name (conn, LOCAL_NAME, flags, &err);
    if (name_acquired == DBUS_REQUEST_NAME_REPLY_EXISTS) {
        _error("dbus::another_instance_running: exiting");
        goto _cleanup;
    }

    event_assign(state->events.dispatch, state->events.base, -1, EV_TIMEOUT, dispatch, conn);

    const char *properties_match_signal = "type='signal',interface='" DBUS_INTERFACE_PROPERTIES "',member='" DBUS_SIGNAL_PROPERTIES_CHANGED "',path='" MPRIS_PLAYER_PATH "'";
    dbus_bus_add_match(conn, properties_match_signal, &err);
    _trace("dbus::add_match: %s", properties_match_signal);
    if (dbus_error_is_set(&err)) {
        _error("dbus::add_match: %s", err.message);
        dbus_error_free(&err);
        goto _cleanup;
    }
    const char *names_signal = "type='signal',interface='" DBUS_INTERFACE_DBUS "',member='" DBUS_SIGNAL_NAME_OWNER_CHANGED "',path='" DBUS_PATH_DBUS "'";
    dbus_bus_add_match(conn, names_signal, &err);
    _trace("dbus::add_match: %s", names_signal);
    if (dbus_error_is_set(&err)) {
        _error("dbus::add_match: %s", err.message);
        dbus_error_free(&err);
        goto _cleanup;
    }
    // adding dbus filter/watch events
    if (!dbus_connection_add_filter(conn, add_filter, state, NULL)) {
        _error("dbus::add_filter: failed");
        goto _cleanup;
    }

    if (!dbus_connection_set_watch_functions(conn, add_watch, remove_watch, toggle_watch, state, NULL)) {
        _error("dbus::add_watch_functions: failed");
        goto _cleanup;
    }

    dbus_connection_set_dispatch_status_function(conn, handle_dispatch_status, state, NULL);

    dbus_connection_set_exit_on_disconnect(conn, false);
    state->dbus->conn = conn;

    return state->dbus;
_cleanup:
    if (dbus_error_is_set(&err)) {
        _trace("dbus::err: %s", err.message);
        dbus_error_free(&err);
    }
    dbus_close(state);
    return NULL;
}
#endif // MPRIS_SCROBBLER_SDBUS_H
