#ifndef WTF_DATAGRAM_H
#define WTF_DATAGRAM_H
#include "types.h"
#ifdef __cplusplus
extern "C" {
#endif

void wtf_datagram_process(wtf_connection* conn, const uint8_t* data, size_t data_len,
                          bool is_client_initiated);


#ifdef __cplusplus
}
#endif
#endif  // WTF_DATAGRAM_H
