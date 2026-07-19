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
    (void)request;
    (void)user_context;
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
wtf_server_add_http_route(server, "GET", "/health", health, NULL);
wtf_server_start(server);
```

Route matching is case-sensitive and ignores the query string. Unmatched paths return `404`, and a
known path with the wrong method returns `405`. Request bodies are delivered once complete and are
currently limited to 1 MiB. Response bodies accept up to 64 scatter-gather buffers and are copied
synchronously, so handler-owned memory does not need to outlive the callback. Route callbacks run
on MsQuic worker threads and may execute concurrently.

## License

MIT
