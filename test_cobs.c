#include "cobs.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

struct test_data
{
    uint8_t encoded[1024];
    ssize_t encoded_length;
    uint8_t unencoded[1024];
    ssize_t unencoded_length;
};

static struct test_data test_data_make(void)
{
    struct test_data data;
    memset(data.encoded, 0xca, sizeof(data.encoded));
    data.encoded_length = 0;
    memset(data.unencoded, 0xca, sizeof(data.unencoded));
    data.unencoded_length = 0;
    return data;
}

static void test_cobs_maximum_sizeof(void)
{
    assert(1 == cobs_maximum_sizeof(0));
    assert(2 == cobs_maximum_sizeof(1));
    assert(3 == cobs_maximum_sizeof(2));
    assert(254 == cobs_maximum_sizeof(253));
    assert(256 == cobs_maximum_sizeof(254));
    assert(257 == cobs_maximum_sizeof(255));
    assert(0 == cobs_maximum_sizeof(SIZE_MAX - 1));
    assert(0 == cobs_maximum_sizeof(SIZE_MAX));
}

static void test_cobs_encode(void)
{
    struct test_data data = test_data_make();

    assert(-EFAULT == cobs_encode(NULL,      0, NULL, 0));
    assert(-EFAULT == cobs_encode(data.unencoded, 0, NULL, 0));
    assert(-ENOSPC == cobs_encode(data.unencoded, 2, data.encoded, 2));
}

static void test_cobs_encode_api(void)
{
    struct test_data data = test_data_make();
    struct cobs_encode_state *s;

    s = cobs_encode_new();

    assert(-EFAULT == cobs_encode_clear(NULL));
    assert(      0 == cobs_encode_clear(s));
    assert(-EFAULT == cobs_encode_start(NULL, data.encoded, sizeof(data.encoded)));
    assert(-EFAULT == cobs_encode_start(s,   NULL,    sizeof(data.encoded)));
    assert(-ENOSPC == cobs_encode_start(s,   data.encoded, 1));
    assert(-EFAULT == cobs_encode_add(NULL, NULL, 42));
    assert(-EFAULT == cobs_encode_add(s,   NULL, 42));
    assert(-EFAULT == cobs_encode_finish(NULL));

    {
        struct test_data data = test_data_make();
        uint8_t m[] = { 0x04, 0x00 };
        assert(0       == cobs_encode_start(s, data.encoded, 2));
        assert(-ENOSPC == cobs_encode_add(s,   m, sizeof(m)));
    }
    {
        struct test_data data = test_data_make();
        uint8_t m[] = { 0x00, 0x04 };
        assert(0       == cobs_encode_start(s, data.encoded, 2));
        assert(-ENOSPC == cobs_encode_add(s,   m, sizeof(m)));
    }
    {
        struct test_data data = test_data_make();
        assert(0       == cobs_encode_start(s, data.encoded, 255));
        assert(-ENOSPC == cobs_encode_add(s,   data.unencoded, 256));
    }

    cobs_encode_delete(s);
}

static void test_cobs_decode(void)
{
    struct test_data data = test_data_make();

    assert(-EFAULT == cobs_decode(NULL,    0, NULL, 0, false));
    assert(-EFAULT == cobs_decode(data.encoded, 0, NULL, 0, false));

    {
        uint8_t m[] = { 0x01, 0x11, 0x22 };
        assert(-ENOSPC == cobs_decode(m, sizeof(m), data.unencoded, 1, false));
    }
}

static void test_cobs_decode_api(void)
{
    struct test_data data = test_data_make();
    struct cobs_decode_state *s;

    s = cobs_decode_new();

    assert(-EFAULT == cobs_decode_clear(NULL));
    assert(      0 == cobs_decode_clear(s));
    assert(-EFAULT == cobs_decode_start(NULL, NULL,      sizeof(data.unencoded)));
    assert(-EFAULT == cobs_decode_start(s,   NULL,      sizeof(data.unencoded)));
    assert(-ENOSPC == cobs_decode_start(s,   data.unencoded, 0));
    assert(-EFAULT == cobs_decode_add(NULL, NULL, 42));
    assert(-EFAULT == cobs_decode_add(s,   NULL, 42));
    assert(-EFAULT  == cobs_decode_finish(NULL, false));

    {
        struct test_data data = test_data_make();
        uint8_t m[] = { 0x00 };
        assert(0        == cobs_decode_start(s, data.unencoded, 1));
        assert(0        == cobs_decode_add(s,   m, 0));
        assert(-EILSEQ  == cobs_decode_add(s,   m, sizeof(m)));
    }
    {
        struct test_data data = test_data_make();
        uint8_t m[] = { 0x01 };
        assert(0        == cobs_decode_start(s, data.unencoded, 1));
        assert(0        == cobs_decode_add(s,   m, sizeof(m)));
        assert(0        == cobs_decode_finish(s, false));
    }
    {
        struct test_data data = test_data_make();
        uint8_t m[] = { 0x02, 0x01 };
        assert(0        == cobs_decode_start(s, data.unencoded, 1));
        assert(0        == cobs_decode_add(s,   m, sizeof(m)));
        assert(1        == cobs_decode_finish(s, false));
    }
    {
        struct test_data data = test_data_make();
        uint8_t m[] = { 0x03, 0x01, 0x02 };
        assert(0        == cobs_decode_start(s, data.unencoded, 1));
        assert(-ENOSPC  == cobs_decode_add(s,   m, sizeof(m)));
    }
    {
        struct test_data data = test_data_make();
        uint8_t m[] = { 0x01, 0x01, 0x02 };
        assert(0        == cobs_decode_start(s, data.unencoded, 1));
        assert(-ENOSPC  == cobs_decode_add(s,   m, sizeof(m)));
    }

    cobs_decode_delete(s);
}

static void test_roundtrip_empty(void)
{
    struct test_data data = test_data_make();
    uint8_t m[] = { 0xff };

    data.encoded_length = cobs_encode(m, 0, data.encoded, sizeof(data.encoded));
    assert(data.encoded_length == 1);
    assert(data.encoded[0] == 0x01);

    data.unencoded_length = cobs_decode(data.encoded, (size_t)data.encoded_length, data.unencoded, sizeof(data.unencoded), false);
    assert(data.unencoded_length == 0);
}

static void test_roundtrip_no_zeroes(void)
{
    struct test_data data = test_data_make();
    uint8_t m[] = { 0x11, 0x22 };

    data.encoded_length = cobs_encode(m, sizeof(m), data.encoded, sizeof(data.encoded));
    assert(data.encoded_length == 3);
    assert(data.encoded[0] == 0x03);
    assert(data.encoded[1] == 0x11);
    assert(data.encoded[2] == 0x22);

    data.unencoded_length = cobs_decode(data.encoded, (size_t)data.encoded_length, data.unencoded, sizeof(data.unencoded), false);
    assert(data.unencoded_length == sizeof(m));
    assert(memcmp(m, data.unencoded, sizeof(m)) == 0);
}

static void test_roundtrip_starting_with_zero(bool strict)
{
    struct test_data data = test_data_make();
    uint8_t m[] = { 0x00, 0x11 };

    data.encoded_length = cobs_encode(m, sizeof(m), data.encoded, sizeof(data.encoded));
    assert(data.encoded_length == 3);
    assert(data.encoded[0] == 0x01);
    assert(data.encoded[1] == 0x02);
    assert(data.encoded[2] == 0x11);

    data.unencoded_length = cobs_decode(data.encoded, (size_t)data.encoded_length, data.unencoded, sizeof(data.unencoded), false);
    assert(data.unencoded_length == sizeof(m));
    assert(memcmp(m, data.unencoded, sizeof(m)) == 0);

    data.encoded[1] = 0xbc;
    data.unencoded_length = cobs_decode(data.encoded, (size_t)data.encoded_length, data.unencoded, sizeof(data.unencoded), strict);
    if (strict) {
        assert(-EMSGSIZE == data.unencoded_length);
    } else {
        assert(data.unencoded_length == 2);
        assert(data.unencoded[0] == 0x00);
        assert(data.unencoded[1] == 0x11);
    }
}

static void test_roundtrip_start_and_end_with_zero(void)
{
    struct test_data data = test_data_make();
    uint8_t m[] = { 0x00, 0x04, 0x00 };

    data.encoded_length = cobs_encode(m, sizeof(m), data.encoded, sizeof(data.encoded));
    assert(data.encoded_length == 4);
    assert(data.encoded[0] == 0x01);
    assert(data.encoded[1] == 0x02);
    assert(data.encoded[2] == 0x04);
    assert(data.encoded[3] == 0x01);

    data.unencoded_length = cobs_decode(data.encoded, (size_t)data.encoded_length, data.unencoded, sizeof(data.unencoded), false);
    assert(data.unencoded_length == sizeof(m));
    assert(memcmp(m, data.unencoded, sizeof(m)) == 0);
}

static void test_roundtrip_253(void)
{
    struct test_data data = test_data_make();
    // 253 bytes, no NUL.
    uint8_t m[253];
    memset(m, 0xaa, sizeof(m));

    assert(254 == cobs_maximum_sizeof(sizeof(m)));

    data.encoded_length = cobs_encode(m, sizeof(m), data.encoded, sizeof(data.encoded));
    assert(data.encoded_length == 254);
    assert(data.encoded[  0] == 0xfe);
    assert(data.encoded[  1] == 0xaa);
    assert(data.encoded[253] == 0xaa);

    data.unencoded_length = cobs_decode(data.encoded, (size_t)data.encoded_length, data.unencoded, sizeof(m), false);
    assert(data.unencoded_length == sizeof(m));
    assert(memcmp(m, data.unencoded, sizeof(m)) == 0);
}

static void test_roundtrip_254(void)
{
    struct test_data data = test_data_make();
    // 254 bytes, no NUL.
    uint8_t m[254];
    memset(m, 0xaa, sizeof(m));

    assert(256 == cobs_maximum_sizeof(sizeof(m)));

    data.encoded_length = cobs_encode(m, sizeof(m), data.encoded, sizeof(data.encoded));
    assert(data.encoded_length == 256);
    assert(data.encoded[  0] == 0xff);
    assert(data.encoded[  1] == 0xaa);
    assert(data.encoded[254] == 0xaa);
    assert(data.encoded[255] == 0x01);

    data.unencoded_length = cobs_decode(data.encoded, (size_t)data.encoded_length, data.unencoded, sizeof(m), false);
    assert(data.unencoded_length == sizeof(m));
    assert(memcmp(m, data.unencoded, sizeof(m)) == 0);
}

static void test_roundtrip_255(void)
{
    struct test_data data = test_data_make();
    // 255 bytes, no NUL.
    uint8_t m[255];
    memset(m, 0xaa, sizeof(m));

    assert(257 == cobs_maximum_sizeof(sizeof(m)));

    data.encoded_length = cobs_encode(m, sizeof(m), data.encoded, sizeof(data.encoded));
    assert(data.encoded_length == 257);
    assert(data.encoded[  0] == 0xff);
    assert(data.encoded[  1] == 0xaa);
    assert(data.encoded[254] == 0xaa);
    assert(data.encoded[255] == 0x02);
    assert(data.encoded[256] == 0xaa);

    data.unencoded_length = cobs_decode(data.encoded, (size_t)data.encoded_length, data.unencoded, sizeof(m), false);
    assert(data.unencoded_length == sizeof(m));
    assert(memcmp(m, data.unencoded, sizeof(m)) == 0);
}

int main(void)
{
    test_cobs_maximum_sizeof();
    test_cobs_encode();
    test_cobs_encode_api();
    test_cobs_decode();
    test_cobs_decode_api();
    test_roundtrip_empty();
    test_roundtrip_no_zeroes();
    test_roundtrip_starting_with_zero(true);
    test_roundtrip_starting_with_zero(false);
    test_roundtrip_start_and_end_with_zero();
    test_roundtrip_253();
    test_roundtrip_254();
    test_roundtrip_255();
}
