#include "cobs.h"

#include <assert.h>
#include <errno.h>
#include <string.h>

static uint8_t encoded[1024];
static ssize_t encoded_length;
static uint8_t unencoded[1024];
static ssize_t unencoded_length;

static void reset(void)
{
    memset(encoded, 0xca, sizeof(encoded));
    encoded_length = 0;
    memset(unencoded, 0xca, sizeof(unencoded));
    unencoded_length = 0;
}

int main(void)
{
    {
        struct cobs_encode_state *s;

        s = cobs_encode_new();

        assert(-EFAULT == cobs_encode_clear(NULL));
        assert(      0 == cobs_encode_clear(s));
        assert(-EFAULT == cobs_encode_start(NULL, encoded, sizeof(encoded)));
        assert(-EFAULT == cobs_encode_start(s,   NULL,    sizeof(encoded)));
        assert(-ENOSPC == cobs_encode_start(s,   encoded, 1));
        assert(-EFAULT == cobs_encode_add(NULL, NULL, 42));
        assert(-EFAULT == cobs_encode_add(s,   NULL, 42));
        assert(-EFAULT == cobs_encode_finish(NULL));

        {
            uint8_t m[] = { 0x04, 0x00 };
            reset();
            assert(0       == cobs_encode_start(s, encoded, 2));
            assert(-ENOSPC == cobs_encode_add(s,   m, sizeof(m)));
        }
        {
            uint8_t m[] = { 0x00, 0x04 };
            reset();
            assert(0       == cobs_encode_start(s, encoded, 2));
            assert(-ENOSPC == cobs_encode_add(s,   m, sizeof(m)));
        }
        {
            reset();
            assert(0       == cobs_encode_start(s, encoded, 255));
            assert(-ENOSPC == cobs_encode_add(s,   unencoded, 256));
        }

        cobs_encode_delete(s);
    }

    {
        reset();
        assert(-EFAULT == cobs_encode(NULL,      0, NULL, 0));
        assert(-EFAULT == cobs_encode(unencoded, 0, NULL, 0));
        assert(-ENOSPC == cobs_encode(unencoded, 2, encoded, 2));
    }

    {
        struct cobs_decode_state *s;

        s = cobs_decode_new();

        assert(-EFAULT == cobs_decode_clear(NULL));
        assert(      0 == cobs_decode_clear(s));
        assert(-EFAULT == cobs_decode_start(NULL, NULL,      sizeof(unencoded)));
        assert(-EFAULT == cobs_decode_start(s,   NULL,      sizeof(unencoded)));
        assert(-ENOSPC == cobs_decode_start(s,   unencoded, 0));
        assert(-EFAULT == cobs_decode_add(NULL, NULL, 42));
        assert(-EFAULT == cobs_decode_add(s,   NULL, 42));
        assert(-EFAULT  == cobs_decode_finish(NULL, false));

        {
            uint8_t m[] = { 0x00 };
            reset();
            assert(0        == cobs_decode_start(s, unencoded, 1));
            assert(0        == cobs_decode_add(s,   m, 0));
            assert(-EILSEQ  == cobs_decode_add(s,   m, sizeof(m)));
        }
        {
            uint8_t m[] = { 0x01 };
            reset();
            assert(0        == cobs_decode_start(s, unencoded, 1));
            assert(0        == cobs_decode_add(s,   m, sizeof(m)));
            assert(0        == cobs_decode_finish(s, false));
        }
        {
            uint8_t m[] = { 0x02, 0x01 };
            reset();
            assert(0        == cobs_decode_start(s, unencoded, 1));
            assert(0        == cobs_decode_add(s,   m, sizeof(m)));
            assert(1        == cobs_decode_finish(s, false));
        }
        {
            uint8_t m[] = { 0x03, 0x01, 0x02 };
            reset();
            assert(0        == cobs_decode_start(s, unencoded, 1));
            assert(-ENOSPC  == cobs_decode_add(s,   m, sizeof(m)));
        }
        {
            uint8_t m[] = { 0x01, 0x01, 0x02 };
            reset();
            assert(0        == cobs_decode_start(s, unencoded, 1));
            assert(-ENOSPC  == cobs_decode_add(s,   m, sizeof(m)));
        }

        cobs_decode_delete(s);
    }

    assert(-EFAULT == cobs_decode(NULL,    0, NULL, 0, false));
    assert(-EFAULT == cobs_decode(encoded, 0, NULL, 0, false));
    {
        uint8_t m[] = { 0x01, 0x11, 0x22 };
        assert(-ENOSPC == cobs_decode(m, sizeof(m), unencoded, 1, false));
    }

    {
        uint8_t m[] = { 0xff };
        reset();
        encoded_length = cobs_encode(m, 0, encoded, sizeof(encoded));
        assert(encoded_length == 1);
        assert(encoded[0] == 0x01);

        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(unencoded), false);
        assert(unencoded_length == 0);
    }

    {
        uint8_t m[] = { 0x11, 0x22 };
        reset();
        encoded_length = cobs_encode(m, sizeof(m), encoded, sizeof(encoded));
        assert(encoded_length == 3);
        assert(encoded[0] == 0x03);
        assert(encoded[1] == 0x11);
        assert(encoded[2] == 0x22);

        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(unencoded), false);
        assert(unencoded_length == sizeof(m));
        assert(memcmp(m, unencoded, sizeof(m)) == 0);
    }

    {
        uint8_t m[] = { 0x00, 0x11 };
        reset();
        encoded_length = cobs_encode(m, sizeof(m), encoded, sizeof(encoded));
        assert(encoded_length == 3);
        assert(encoded[0] == 0x01);
        assert(encoded[1] == 0x02);
        assert(encoded[2] == 0x11);

        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(unencoded), false);
        assert(unencoded_length == sizeof(m));
        assert(memcmp(m, unencoded, sizeof(m)) == 0);

        encoded[1] = 0xbc;
        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(unencoded), false);
        assert(unencoded_length == 2);
        assert(unencoded[0] == 0x00);
        assert(unencoded[1] == 0x11);
    }

    {
        uint8_t m[] = { 0x00, 0x11 };
        reset();
        encoded_length = cobs_encode(m, sizeof(m), encoded, sizeof(encoded));
        assert(encoded_length == 3);
        assert(encoded[0] == 0x01);
        assert(encoded[1] == 0x02);
        assert(encoded[2] == 0x11);

        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(unencoded), false);
        assert(unencoded_length == sizeof(m));
        assert(memcmp(m, unencoded, sizeof(m)) == 0);

        encoded[1] = 0xbc;
        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(unencoded), true /*strict*/);
        assert(-EMSGSIZE == unencoded_length);
    }

    {
        uint8_t m[] = { 0x00, 0x04, 0x00 };
        reset();
        encoded_length = cobs_encode(m, sizeof(m), encoded, sizeof(encoded));
        assert(encoded_length == 4);
        assert(encoded[0] == 0x01);
        assert(encoded[1] == 0x02);
        assert(encoded[2] == 0x04);
        assert(encoded[3] == 0x01);

        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(unencoded), false);
        assert(unencoded_length == sizeof(m));
        assert(memcmp(m, unencoded, sizeof(m)) == 0);
    }

    {
        // 253 bytes, no NUL.
        uint8_t m[253];
        memset(m, 0xaa, sizeof(m));

        assert(254 == cobs_maximum_sizeof(sizeof(m)));

        reset();
        encoded_length = cobs_encode(m, sizeof(m), encoded, sizeof(encoded));
        assert(encoded_length == 254);
        assert(encoded[  0] == 0xfe);
        assert(encoded[  1] == 0xaa);
        assert(encoded[253] == 0xaa);

        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(m), false);
        assert(unencoded_length == sizeof(m));
        assert(memcmp(m, unencoded, sizeof(m)) == 0);
    }

    {
        // 254 bytes, no NUL.
        uint8_t m[254];
        memset(m, 0xaa, sizeof(m));

        assert(256 == cobs_maximum_sizeof(sizeof(m)));

        reset();
        encoded_length = cobs_encode(m, sizeof(m), encoded, sizeof(encoded));
        assert(encoded_length == 256);
        assert(encoded[  0] == 0xff);
        assert(encoded[  1] == 0xaa);
        assert(encoded[254] == 0xaa);
        assert(encoded[255] == 0x01);

        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(m), false);
        assert(unencoded_length == sizeof(m));
        assert(memcmp(m, unencoded, sizeof(m)) == 0);
    }

    {
        // 255 bytes, no NUL.
        uint8_t m[255];
        memset(m, 0xaa, sizeof(m));

        assert(257 == cobs_maximum_sizeof(sizeof(m)));

        reset();
        encoded_length = cobs_encode(m, sizeof(m), encoded, sizeof(encoded));
        assert(encoded_length == 257);
        assert(encoded[  0] == 0xff);
        assert(encoded[  1] == 0xaa);
        assert(encoded[254] == 0xaa);
        assert(encoded[255] == 0x02);
        assert(encoded[256] == 0xaa);

        unencoded_length = cobs_decode(encoded, (size_t)encoded_length, unencoded, sizeof(m), false);
        assert(unencoded_length == sizeof(m));
        assert(memcmp(m, unencoded, sizeof(m)) == 0);
    }

    return 0;
}
