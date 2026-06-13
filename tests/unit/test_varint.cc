#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>

extern "C" {
#include "varint.h"
}

TEST(Varint, ReportsEncodedSizeAtBoundaries)
{
    EXPECT_EQ(wtf_varint_size(0), 1u);
    EXPECT_EQ(wtf_varint_size(63), 1u);
    EXPECT_EQ(wtf_varint_size(64), 2u);
    EXPECT_EQ(wtf_varint_size(16383), 2u);
    EXPECT_EQ(wtf_varint_size(16384), 4u);
    EXPECT_EQ(wtf_varint_size(0x3fffffffULL), 4u);
    EXPECT_EQ(wtf_varint_size(0x40000000ULL), 8u);
    EXPECT_EQ(wtf_varint_size(WTF_VARINT_MAX), 8u);
}

TEST(Varint, RoundTripsRepresentativeValues)
{
    const uint64_t values[] = {
        0,
        1,
        63,
        64,
        15293,
        16383,
        16384,
        0x3fffffffULL,
        0x40000000ULL,
        WTF_VARINT_MAX,
    };

    for (uint64_t value : values) {
        uint8_t encoded[8] = {};
        uint8_t* end = wtf_varint_encode(value, encoded);
        size_t encoded_len = static_cast<size_t>(end - encoded);
        size_t offset = 0;
        uint64_t decoded = 0;

        ASSERT_TRUE(wtf_varint_decode(encoded_len, encoded, &offset, &decoded)) << value;
        EXPECT_EQ(decoded, value);
        EXPECT_EQ(offset, encoded_len);
    }
}

TEST(Varint, DecodesNonMinimalTwoByteEncoding)
{
    uint8_t non_minimal[] = {0x40, 0x25};
    size_t offset = 0;
    uint64_t decoded = 0;

    ASSERT_TRUE(wtf_varint_decode(sizeof(non_minimal), non_minimal, &offset, &decoded));
    EXPECT_EQ(decoded, 0x25u);
    EXPECT_EQ(offset, sizeof(non_minimal));
}

TEST(Varint, RejectsTruncatedEncodingsWithoutMovingOffset)
{
    const uint8_t two_byte[] = {0x40};
    size_t offset = 0;
    uint64_t decoded = 0;

    EXPECT_FALSE(wtf_varint_decode(sizeof(two_byte), two_byte, &offset, &decoded));
    EXPECT_EQ(offset, 0u);
}

TEST(Varint, RejectsNullArguments)
{
    uint8_t encoded[] = {0};
    size_t offset = 0;
    uint64_t decoded = 0;

    EXPECT_FALSE(wtf_varint_decode(sizeof(encoded), nullptr, &offset, &decoded));
    EXPECT_FALSE(wtf_varint_decode(sizeof(encoded), encoded, nullptr, &decoded));
    EXPECT_FALSE(wtf_varint_decode(sizeof(encoded), encoded, &offset, nullptr));
}

TEST(Varint, DecodesAfterLargeOffset)
{
    const size_t start_offset = 70000;
    const uint64_t value = 15293;
    uint8_t* buffer = static_cast<uint8_t*>(calloc(start_offset + 8, 1));
    ASSERT_NE(buffer, nullptr);

    uint8_t* end = wtf_varint_encode(value, buffer + start_offset);
    size_t offset = start_offset;
    uint64_t decoded = 0;

    EXPECT_TRUE(wtf_varint_decode(static_cast<size_t>(end - buffer), buffer, &offset, &decoded));
    EXPECT_EQ(decoded, value);
    EXPECT_EQ(offset, static_cast<size_t>(end - buffer));

    free(buffer);
}
