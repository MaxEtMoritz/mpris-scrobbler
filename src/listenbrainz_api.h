/**
 * @author Marius Orcsik <marius@habarnam.ro>
 */
#ifndef MPRIS_SCROBBLER_LISTENBRAINZ_API_H
#define MPRIS_SCROBBLER_LISTENBRAINZ_API_H

#ifndef LISTENBRAINZ_API_KEY
#include "credentials_listenbrainz.h"
#endif


#define API_LISTEN_TYPE_NOW_PLAYING     "playing_now"
#define API_LISTEN_TYPE_SCROBBLE        "single"

#define API_LISTEN_TYPE_NODE_NAME       "listen_type"
#define API_PAYLOAD_NODE_NAME           "payload"
#define API_LISTENED_AT_NODE_NAME       "listened_at"
#define API_METADATA_NODE_NAME          "track_metadata"
#define API_ARTIST_NAME_NODE_NAME       "artist_name"
#define API_TRACK_NAME_NODE_NAME        "track_name"
#define API_ALBUM_NAME_NODE_NAME        "release_name"

#define API_CODE_NODE_NAME              "code"
#define API_ERROR_NODE_NAME             "error"

#define LISTENBRAINZ_AUTH_URL           "https://listenbrainz.org/api/auth/?api_key=%s&token=%s"
#define LISTENBRAINZ_API_BASE_URL       "api.listenbrainz.org"
#define LISTENBRAINZ_API_VERSION        "1"

#define API_ENDPOINT_SUBMIT_LISTEN      "submit-listens"
#define API_HEADER_AUTHORIZATION_TOKENIZED "Authorization: Token %s"


static bool listenbrainz_valid_credentials(const struct api_credentials *auth)
{
    bool status = false;
    if (NULL == auth) { return status; }
    if (auth->end_point != listenbrainz) { return status; }

    status = true;
    return status;
}

#if 0
static http_request *build_generic_request()
{
}
#endif

void print_http_request(struct http_request*);
/*
 */
struct http_request *listenbrainz_api_build_request_now_playing(const struct scrobble *track, CURL *handle, const struct api_credentials *auth)
{
    if (NULL == handle) { return NULL; }
    if (!listenbrainz_valid_credentials(auth)) { return NULL; }

    const char *token = auth->token;

    char *body = get_zero_string(MAX_BODY_SIZE);

    json_object *root = json_object_new_object();
    json_object_object_add(root, API_LISTEN_TYPE_NODE_NAME, json_object_new_string(API_LISTEN_TYPE_NOW_PLAYING));

    json_object *payload = json_object_new_array();

    json_object *payload_elem = json_object_new_object();
    json_object *metadata = json_object_new_object();
    json_object_object_add(metadata, API_ALBUM_NAME_NODE_NAME, json_object_new_string(track->album));
    json_object_object_add(metadata, API_ARTIST_NAME_NODE_NAME, json_object_new_string(track->artist));
    json_object_object_add(metadata, API_TRACK_NAME_NODE_NAME, json_object_new_string(track->title));

    //json_object_object_add(payload_elem, API_LISTENED_AT_NODE_NAME, json_object_new_int64(track->start_time));
    json_object_object_add(payload_elem, API_METADATA_NODE_NAME, metadata);

    json_object_array_add(payload, payload_elem);
    json_object_object_add(root, API_PAYLOAD_NODE_NAME, payload);

    const char *json_str = json_object_to_json_string(root);
    strncpy(body, json_str, strlen(json_str));

    char *authorization_header = get_zero_string(MAX_URL_LENGTH);
    snprintf(authorization_header, MAX_URL_LENGTH, API_HEADER_AUTHORIZATION_TOKENIZED, token);

    struct http_request *request = http_request_new();
    request->headers[0] = authorization_header;
    request->headers_count++;

    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth->end_point);
    request->url = api_get_url(request->end_point);
    strncpy(request->url + strlen(request->url), API_ENDPOINT_SUBMIT_LISTEN, strlen(API_ENDPOINT_SUBMIT_LISTEN));

#if 0
    print_http_request(request);
#endif
    json_object_put(root);

    return request;
}

/*
 */
struct http_request *listenbrainz_api_build_request_scrobble(const struct scrobble *tracks[], size_t track_count, CURL *handle, const struct api_credentials *auth)
{
    if (NULL == handle) { return NULL; }
    if (!listenbrainz_valid_credentials(auth)) { return NULL; }

    const char *token = auth->token;

    char *authorization_header = get_zero_string(MAX_URL_LENGTH);
    snprintf(authorization_header, MAX_URL_LENGTH, API_HEADER_AUTHORIZATION_TOKENIZED, token);

    char *body = get_zero_string(MAX_BODY_SIZE);
    char *query = get_zero_string(MAX_BODY_SIZE);

    json_object *root = json_object_new_object();
    json_object_object_add(root, API_LISTEN_TYPE_NODE_NAME, json_object_new_string(API_LISTEN_TYPE_SCROBBLE));

    json_object *payload = json_object_new_array();
    for (size_t i = 0; i < track_count; i++) {
        const struct scrobble *track = tracks[i];
        json_object *payload_elem = json_object_new_object();
        json_object *metadata = json_object_new_object();
        json_object_object_add(metadata, API_ALBUM_NAME_NODE_NAME, json_object_new_string(track->album));
        json_object_object_add(metadata, API_ARTIST_NAME_NODE_NAME, json_object_new_string(track->artist));
        json_object_object_add(metadata, API_TRACK_NAME_NODE_NAME, json_object_new_string(track->title));

        json_object_object_add(payload_elem, API_LISTENED_AT_NODE_NAME, json_object_new_int64(track->start_time));
        json_object_object_add(payload_elem, API_METADATA_NODE_NAME, metadata);

        json_object_array_add(payload, payload_elem);
    }

    json_object_object_add(root, API_PAYLOAD_NODE_NAME, payload);

    const char *json_str = json_object_to_json_string(root);
    strncpy(body, json_str, strlen(json_str));

    //json_object_put(root);
    struct http_request *request = http_request_new();
    request->headers[0] = authorization_header;
    request->headers_count++;

    request->query = query;
    request->body = body;
    request->body_length = strlen(body);
    request->end_point = api_endpoint_new(auth->end_point);
    request->url = api_get_url(request->end_point);
    strncpy(request->url + strlen(request->url), API_ENDPOINT_SUBMIT_LISTEN, strlen(API_ENDPOINT_SUBMIT_LISTEN));

#if 0
    print_http_request(request);
#endif
    json_object_put(root);

    return request;
}

bool listenbrainz_json_document_is_error(const char *buffer, const size_t length)
{
    // { "code": 401, "error": "You need to provide an Authorization header." }
    bool result = false;

    if (NULL == buffer) { return result; }
    if (length == 0) { return result; }

    json_object *code_object = NULL;
    json_object *err_object = NULL;

    struct json_tokener *tokener = json_tokener_new();
    if (NULL == tokener) { return result; }
    json_object *root = json_tokener_parse_ex(tokener, buffer, length);

    if (NULL == root || json_object_object_length(root) < 1) {
        goto _exit;
    }
    json_object_object_get_ex(root, API_CODE_NODE_NAME, &code_object);
    json_object_object_get_ex(root, API_ERROR_NODE_NAME, &err_object);
    if (NULL == code_object || !json_object_is_type(code_object, json_type_string)) {
        goto _exit;
    }
    if (NULL == err_object || !json_object_is_type(err_object, json_type_int)) {
        goto _exit;
    }
    result = true;

_exit:
    if (NULL != root) { json_object_put(root); }
    json_tokener_free(tokener);
    return result;
}

#endif // MPRIS_SCROBBLER_LISTENBRAINZ_API_H
