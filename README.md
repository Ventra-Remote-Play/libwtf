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

## License

MIT
