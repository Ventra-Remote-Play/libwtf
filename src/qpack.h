#ifndef WTF_QPACK_H
#define WTF_QPACK_H

#include "lsqpack.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


bool wtf_qpack_preinit(wtf_qpack_context* ctx, uint32_t max_table_capacity,
                       uint32_t max_blocked_streams);
void wtf_qpack_cleanup(wtf_qpack_context* qpack);

bool wtf_qpack_init_encoder(wtf_context* ctx, wtf_qpack_context* qpack);

bool wtf_qpack_process_decoder(wtf_context* ctx, wtf_qpack_context* qpack, const uint8_t* data,
                               size_t length);

bool wtf_qpack_process_encoder(wtf_context* ctx, wtf_qpack_context* qpack, const uint8_t* data,
                               size_t length);

wtf_result_t wtf_qpack_parse_connect_headers(wtf_context* ctx, wtf_http3_stream* stream,
                                             const uint8_t* data, size_t data_len,
                                             wtf_connect_request* request);

wtf_result_t wtf_qpack_parse_response_headers(wtf_context* ctx, wtf_http3_stream* stream,
                                              const uint8_t* data, size_t data_len,
                                              wtf_connect_response* response);

void wtf_connect_request_cleanup(wtf_connect_request* request);
void wtf_connect_response_cleanup(wtf_connect_response* response);

#ifdef __cplusplus
}
#endif
#endif  // WTF_QPACK_H
