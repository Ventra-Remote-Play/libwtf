#include "varint.h"

#include <string.h>

#ifdef _WIN32
    #include <stdlib.h>
    #define CxPlatByteSwapUint16 _byteswap_ushort
    #define CxPlatByteSwapUint32 _byteswap_ulong
    #define CxPlatByteSwapUint64 _byteswap_uint64
#else
    #define CxPlatByteSwapUint16(value) __builtin_bswap16((unsigned short)(value))
    #define CxPlatByteSwapUint32(value) __builtin_bswap32((value))
    #define CxPlatByteSwapUint64(value) __builtin_bswap64((value))
#endif

size_t wtf_varint_size(wtf_varint_t value)
{
    if (value < 0x40)
        return sizeof(uint8_t);
    if (value < 0x4000)
        return sizeof(uint16_t);
    if (value < 0x40000000)
        return sizeof(uint32_t);
    return sizeof(uint64_t);
}

uint8_t* wtf_varint_encode(wtf_varint_t value, uint8_t* buffer)
{
    if (value < 0x40) {
        buffer[0] = (uint8_t)value;
        return buffer + sizeof(uint8_t);
    } else if (value < 0x4000) {
        const uint16_t tmp = CxPlatByteSwapUint16((0x40 << 8) | (uint16_t)value);
        memcpy(buffer, &tmp, sizeof(tmp));
        return buffer + sizeof(uint16_t);
    } else if (value < 0x40000000) {
        const uint32_t tmp = CxPlatByteSwapUint32((0x80UL << 24) | (uint32_t)value);
        memcpy(buffer, &tmp, sizeof(tmp));
        return buffer + sizeof(uint32_t);
    } else {
        const uint64_t tmp = CxPlatByteSwapUint64((0xc0ULL << 56) | value);
        memcpy(buffer, &tmp, sizeof(tmp));
        return buffer + sizeof(uint64_t);
    }
}

uint8_t* wtf_varint_encode_2bytes(wtf_varint_t value, uint8_t* buffer)
{
    const uint16_t tmp = CxPlatByteSwapUint16((0x40 << 8) | (uint16_t)value);
    memcpy(buffer, &tmp, sizeof(tmp));
    return buffer + sizeof(uint16_t);
}

bool wtf_varint_decode(size_t buffer_length, const uint8_t* buffer, size_t* offset,
                       wtf_varint_t* value)
{
    if (!buffer || !offset || !value) {
        return false;
    }

    if (*offset > buffer_length || buffer_length - *offset < sizeof(uint8_t)) {
        return false;
    }

    if (buffer[*offset] < 0x40) {
        *value = buffer[*offset];
        *offset += sizeof(uint8_t);
    } else if (buffer[*offset] < 0x80) {
        if (buffer_length - *offset < sizeof(uint16_t)) {
            return false;
        }
        *value = ((uint64_t)(buffer[*offset] & 0x3fUL)) << 8;
        *value |= buffer[*offset + 1];
        *offset += sizeof(uint16_t);
    } else if (buffer[*offset] < 0xc0) {
        if (buffer_length - *offset < sizeof(uint32_t)) {
            return false;
        }
        uint32_t v;
        memcpy(&v, buffer + *offset, sizeof(uint32_t));
        *value = CxPlatByteSwapUint32(v) & 0x3fffffffUL;
        *offset += sizeof(uint32_t);
    } else {
        if (buffer_length - *offset < sizeof(uint64_t)) {
            return false;
        }
        uint64_t v;
        memcpy(&v, buffer + *offset, sizeof(uint64_t));
        *value = CxPlatByteSwapUint64(v) & 0x3fffffffffffffffULL;
        *offset += sizeof(uint64_t);
    }
    return true;
}
