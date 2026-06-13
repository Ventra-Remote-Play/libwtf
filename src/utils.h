#ifndef WTF_UTILS_H
#define WTF_UTILS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

wtf_result_t wtf_quic_status_to_result(QUIC_STATUS status);
uint64_t wtf_map_webtransport_error_to_h3(uint32_t wt_error);
uint32_t wtf_map_h3_error_to_webtransport(uint64_t h3_error);

char* wtf_strdup(const char* s);
size_t wtf_strncpy(char* dest, const char* src, size_t dest_size);
char* wtf_strndup(const char* s, size_t n);
bool wtf_path_valid(const char* path);
bool wtf_parse_thumbprint(const char* hex_thumbprint, uint8_t sha_hash[20]);

bool wtf_random_uint64(uint64_t* result);

wtf_result_t wtf_internal_send_context_create(size_t length, wtf_internal_send_context** send_ctx,
                                              uint8_t** data);
wtf_result_t wtf_internal_send_context_create_copy(const void* data, size_t length,
                                                   wtf_internal_send_context** send_ctx);
wtf_result_t wtf_internal_send_context_take_buffer(uint8_t* data, size_t length,
                                                   wtf_internal_send_context** send_ctx);
void wtf_internal_send_context_destroy(wtf_internal_send_context* send_ctx);

#ifdef __cplusplus
}
#endif
#endif  // WTF_UTILS_H
