# WebTransport Certificates

Getting WebTransport working requires TLS certificates. This tool generates temporary development certificates for local libwtf testing.

## Basic Usage

Generate certificates for localhost:

```bash
./tools/certgen.sh
```

This creates several files in `./certs/`:
- `localhost.crt` and `localhost.key` - Certificate and private key
- `localhost.pfx` - Windows-compatible bundle
- `fingerprint.hex` - SHA-256 digest for external tooling
- `fingerprint.base64` - SHA-256 digest in base64 form
- `thumbprint.hex` - SHA-1 hash for Windows certificate store

## Browser Setup

For browser WebTransport testing, start the server with the generated PEM files:

```bash
./build/output/wtf_echo_server --port 4433 --cert ./certs/localhost.crt --key ./certs/localhost.key
```

Browsers that support WebTransport certificate hashes can use `fingerprint.hex` directly. The hash
is over the DER-encoded leaf certificate produced by `tools/certgen.sh`.

```js
const hex = "paste the contents of certs/fingerprint.hex here";
const hash = new Uint8Array(hex.match(/../g).map((byte) => parseInt(byte, 16)));

const transport = new WebTransport("https://localhost:4433/", {
  allowPooling: false,
  requireUnreliable: true,
  serverCertificateHashes: [
    {
      algorithm: "sha-256",
      value: hash,
    },
  ],
});

await transport.ready;
```

`serverCertificateHashes` is only valid for dedicated connections, so leave `allowPooling` false
when using the generated hash. If the browser or test path does not support certificate hashes,
install `localhost.crt` into the browser or OS trust store and connect without
`serverCertificateHashes`.

## Platform Differences

### macOS and Linux

Load certificates from files:

```c
wtf_certificate_config_t cert_config = {
    .cert_type = WTF_CERT_TYPE_FILE,
    .cert_data.file = {
        .cert_path = "./certs/localhost.crt",
        .key_path = "./certs/localhost.key"
    }
};
```

Safari requires installing the `.crt` file in Keychain Access. Just double-click the file and mark it as trusted.

### Windows

Windows doesn't support file-based certificates. Import the PFX file into your certificate store first:

1. Run `certlm.msc`
2. Go to Personal → Certificates → Import
3. Select your `.pfx` file

Then use the thumbprint:

```c
wtf_certificate_config_t cert_config = {
    .cert_type = WTF_CERT_TYPE_HASH,
    .cert_data.hash = {
        .thumbprint = "A1B2C3..." // From thumbprint.hex
    }
};
```

## libwtf Client Setup

### Exact Leaf Pinning

For local development or private endpoints, pin the generated leaf certificate file:

```c
wtf_client_config_t config = {
    .url = "https://localhost:4433/",
    .pinned_server_certificate_file = "./certs/localhost.crt",
};
```

The certificate file is parsed through the platform certificate API and compared to the leaf
certificate received from MsQuic.

### Native Trust Validation

For public certificates, omit `ca_cert_file`, `pinned_server_certificate_file`, and
`skip_certificate_validation`:

```c
wtf_client_config_t config = {
    .url = "https://example.com:443/",
};
```

For private CAs on OpenSSL-backed MsQuic builds, set `.ca_cert_file` to a PEM CA bundle. On
Windows/Schannel, import the CA into the OS certificate store instead.

## Tool Options

Generate for specific hostname:
```bash
./tools/certgen.sh --host example.com
```

Generate for IP address:
```bash
./tools/certgen.sh --host 192.168.1.100
```

Set password on PFX file:
```bash
./tools/certgen.sh --pfx-password mypassword
```

Custom output directory:
```bash
./tools/certgen.sh --output /tmp/certs
```

## Production Considerations

### Hash Pinning Strategy

For Chromium-based browsers, you can skip traditional PKI by providing certificate hashes. The browser trusts any certificate matching the provided hashes.

Pros: No certificate authority needed, works immediately
Cons: Chromium-only, requires distributing hashes to clients

### Traditional PKI

Standard approach using certificates from a trusted CA. Works with all browsers but requires domain validation and certificate management.

### Wildcard Certificates

Generate certificates for multiple subdomains:
```bash
./tools/certgen.sh --host "*.api.example.com"
```

## Limitations

Development certificates have restrictions:
- Maximum 14 days validity (WebTransport spec requirement)
- Must use ECDSA with secp256r1 curve (RSA forbidden)
- Certificate rotation requires client updates when using hash pinning

## Certificate Types

The library supports multiple certificate configurations:

- `WTF_CERT_TYPE_FILE` - PEM files (macOS/Linux)
- `WTF_CERT_TYPE_HASH` - Imported certificate by thumbprint (Windows)
- `WTF_CERT_TYPE_PKCS12` - PFX/PKCS#12 files with optional password
- `WTF_CERT_TYPE_HASH_STORE` - Certificate from specific Windows store

## Testing

1. Generate certificate: `./tools/certgen.sh`
2. Start server: `./build/output/wtf_echo_server --port 4433 --cert ./certs/localhost.crt --key ./certs/localhost.key`
3. Open `tools/client.html` and connect to `https://localhost:4433/` with the hash from `certs/fingerprint.hex`
