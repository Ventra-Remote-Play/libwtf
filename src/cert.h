#ifndef WTF_CERT_H
#define WTF_CERT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "wtf.h"

#define WTF_CERT_SHA256_SIZE 32

wtf_result_t wtf_cert_load_der_from_file(const char* path, uint8_t** der, size_t* der_len);
bool wtf_cert_sha256(const uint8_t* data, size_t length,
                     uint8_t digest[WTF_CERT_SHA256_SIZE]);

#endif
