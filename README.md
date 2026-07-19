# libwtf

C WebTransport over HTTP/3, built on [MsQuic](https://github.com/microsoft/msquic).

Supports server and native client use, streams, datagrams, pooled sessions, and draft-15,
draft-07, and draft-02 negotiation.

## Build

```sh
git clone --recurse-submodules https://github.com/andrewmd5/libwtf.git
cd libwtf
make
```

## Test

```sh
make test
make report
```

## Run

```sh
./build/output/wtf_echo_server --port 4433 --cert certs/localhost.crt --key certs/localhost.key
./build/output/wtf_echo_client --host localhost --port 4433 --draft auto
```

## API

See [include/wtf.h](include/wtf.h).

### HTTP/3 routes

Ordinary HTTP/3 endpoints can share the listener and UDP port with WebTransport. Register exact
method/path routes after `wtf_server_create()` and before `wtf_server_start()`:

```c
static void health(const wtf_http_request_t* request,
                   wtf_http_response_t* response,
                   void* user_context)
{
    client_state_t* client = find_or_create_client(user_context, request);
    client->health_passed = true;
    wtf_connection_set_context(request->connection, client);
    static const wtf_http_header_t headers[] = {
        {"content-type", "text/plain"},
    };
    static const uint8_t first[] = "libwtf ";
    static const uint8_t second[] = "ok\n";
    static const wtf_buffer_t body[] = {
        {sizeof(first) - 1, first},
        {sizeof(second) - 1, second},
    };

    response->status_code = 200;
    response->headers = headers;
    response->header_count = 1;
    response->body_buffers = body;
    response->body_buffer_count = 2;
}

wtf_server_t* server = NULL;
wtf_server_create(context, &config, &server);
wtf_server_add_http_route(server, "GET", "/health", health, client_registry);
wtf_server_start(server);
```

The connection context is shared by every HTTP/3 request and WebTransport session carried by the
same physical QUIC connection:

```c
static wtf_connection_decision_t validate_connect(
    const wtf_connection_request_t* request,
    wtf_connection_response_t* response,
    void* user_context)
{
    (void)response;
    (void)user_context;
    client_state_t* client = wtf_connection_get_context(request->connection);
    return client && client->health_passed ? WTF_CONNECTION_ACCEPT : WTF_CONNECTION_REJECT;
}

static void on_session(const wtf_session_event_t* event)
{
    if (event->type != WTF_SESSION_EVENT_CONNECTED)
        return;

    wtf_connection_t* connection = wtf_session_get_connection(event->session);
    client_state_t* client = wtf_connection_get_context(connection);
    wtf_session_set_context(event->session, client);
}
```

Connection handles returned in callbacks are borrowed. The application owns the stored context and
must keep it alive while it can be read. Context pointer publication is atomic; synchronization for
later mutations of the pointed-to object remains the application's responsibility.

Route matching is case-sensitive and ignores the query string. Unmatched paths return `404`, and a
known path with the wrong method returns `405`. Request bodies are delivered once complete and are
currently limited to 1 MiB. Response bodies accept up to 64 scatter-gather buffers and are copied
synchronously, so handler-owned memory does not need to outlive the callback. Route callbacks run
on MsQuic worker threads and may execute concurrently.

## License

MIT
