#ifndef WTF_STREAM_H
#define WTF_STREAM_H
#include "types.h"
#ifdef __cplusplus
extern "C" {
#endif

QUIC_STREAM_CALLBACK wtf_upgraded_stream_callback;

wtf_stream* wtf_stream_create(wtf_session* session, uint64_t stream_id, wtf_stream_type_t type);

void wtf_stream_destroy(wtf_stream* stream);
void wtf_stream_add_ref(wtf_stream* stream);
void wtf_stream_release(wtf_stream* stream);
wtf_result_t wtf_stream_start(wtf_stream* stream);
bool wtf_stream_deliver_received(wtf_stream* stream, const wtf_buffer_t* buffers,
                                 uint32_t buffer_count, uint64_t total_length, bool peer_fin);
void wtf_stream_deliver_peer_closed(wtf_stream* stream);

#ifdef __cplusplus
}
#endif
#endif  // WTF_STREAM_H
