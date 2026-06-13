#ifndef WTF_VARINT_H
#define WTF_VARINT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The maximum value that can be encoded in a variable-length integer
#define WTF_VARINT_MAX ((1ULL << 62U) - 1)

// Variable-length integer type
typedef uint64_t wtf_varint_t;

//! Determine the number of bytes required to encode a value
//! @param value The value to encode
//! @return Number of bytes needed (1, 2, 4, or 8)
size_t wtf_varint_size(wtf_varint_t value);

//! Encode a variable-length integer
//! @param value The value to encode (must be <= WTF_VARINT_MAX)
//! @param buffer Output buffer (must have sufficient space)
//! @return Pointer to next byte after encoded data
uint8_t* wtf_varint_encode(wtf_varint_t value, uint8_t* buffer);

//! Encode a variable-length integer into exactly 2 bytes
//! @param value The value to encode (must be < 0x4000)
//! @param buffer Output buffer (must have at least 2 bytes)
//! @return Pointer to next byte after encoded data
uint8_t* wtf_varint_encode_2bytes(wtf_varint_t value, uint8_t* buffer);

//! Decode a variable-length integer
//! @param buffer_length Total length of input buffer
//! @param buffer Input buffer containing encoded data
//! @param offset Pointer to current offset in buffer (updated on success)
//! @param value Pointer to store decoded value
//! @return true on success, false on failure (insufficient data)
bool wtf_varint_decode(size_t buffer_length, const uint8_t* buffer, size_t* offset,
                       wtf_varint_t* value);

#ifdef __cplusplus
}
#endif

#endif /* WTF_VARINT_H */
