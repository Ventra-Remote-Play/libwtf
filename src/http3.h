#ifndef WTF_HTTP3_H
#define WTF_HTTP3_H
#include "types.h"
#ifdef __cplusplus
extern "C" {
#endif
QUIC_STREAM_CALLBACK wtf_http3_stream_callback;

bool wtf_http3_create_control_stream(wtf_connection* conn);

wtf_http3_stream* wtf_http3_stream_create(wtf_connection* conn, HQUIC quic_stream,
                                          uint64_t stream_id);
void wtf_http3_stream_destroy(wtf_http3_stream* stream);
void wtf_http3_stream_add_ref(wtf_http3_stream* stream);
void wtf_http3_stream_release(wtf_http3_stream* stream);

bool wtf_http3_create_qpack_streams(wtf_connection* conn);
wtf_result_t wtf_http3_client_open_session(wtf_client* client, wtf_session** session);
bool wtf_http3_client_drain_pending_connects(wtf_connection* conn);


#ifdef __cplusplus
}
#endif
#endif  // WTF_HTTP3_H
