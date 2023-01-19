#include "cobs.h"

#include <errno.h>

/// Fixed overhead of one, for the initial byte.
/// Additional overhead of one byte for every 254 non-NUL bytes.
size_t cobs_maximum_sizeof(size_t length)
{
    return 1 + length + (length / 254);
}

/// Given input of the form @c DATA...,NUL,DATA...,NUL,... then the encoder generates
/// output comprised of an initial byte which tells the offset to the first NUL,
/// then a series of zero or more bytes of data.  The NUL is replaced by an offset
/// to the next NUL, and is again followed by zero or more bytes of data.
///
/// ```
/// Offset = 255    254 non-NUL data bytes follow.
/// Offset = 1 + N    N non-NUL data bytes follow, plus a NUL byte.
///
/// +--------+---//---+--------+---//---+--
/// | offset | data   | offset | data   | ...
/// +--------+---//---+--------+---//---+---
/// ```
int cobs_encode_start(struct cobs_encode_state *s, uint8_t *output, size_t capacity)
{
    if (!s || !output) {
        return -EFAULT;
    }

    if (capacity < 2) {
        return -ENOSPC;
    }

    s->output = output;
    s->capacity = capacity;

    s->encoded = output;
    s->ohb = s->encoded++;
    s->capacity--;

    s->offset = 1;
    return 0;
}

int cobs_encode_add(struct cobs_encode_state *s, const uint8_t *data, size_t length)
{
    if (!s || !data) {
        return -EFAULT;
    }

    while (length) {
        if (*data) {
            if (!s->capacity) {
                return -ENOSPC;
            }

            *s->encoded++ = *data;
            s->capacity--;

            s->offset++;
        }

        if (!*data || s->offset == 0xff) {
            /* Delimiter found, or offset reaches maximum. */
            *s->ohb = s->offset;

            if (!s->capacity) {
                return -ENOSPC;
            }

            s->ohb = s->encoded++;
            s->capacity--;

            s->offset = 1;
        }

        data++;
        length--;
    }

    return 0;
}

ssize_t cobs_encode_finish(struct cobs_encode_state *s)
{
    if (!s) {
        return -EFAULT;
    }

    *s->ohb = s->offset;

    return (ssize_t)(s->encoded - s->output);
}

ssize_t cobs_encode(const uint8_t *data, size_t length, uint8_t *output, size_t capacity)
{
    struct cobs_encode_state s;
    int r;

    if (!data) {
        return -EFAULT;
    }

    r = cobs_encode_start(&s, output, capacity);
    if (r < 0) {
        return r;
    }

    while (length--) {
        r = cobs_encode_add(&s, data++, 1);
        if (r < 0) {
            return r;
        }
    }

    return cobs_encode_finish(&s);
}

int cobs_decode_start(struct cobs_decode_state *s, uint8_t *output, size_t capacity)
{
    if (!s || !output) {
        return -EFAULT;
    }

    if (capacity < 1) {
        return -ENOSPC;
    }

    s->output = output;

    s->decoded = output;
    s->capacity = capacity;

    s->offset = 0xff;
    s->run = 0;
    return 0;
}

int cobs_decode_add(struct cobs_decode_state *s, const uint8_t *data, size_t length)
{
    if (!s || !data) {
        return -EFAULT;
    }

    while (length) {
        if (!*data) {
            /* Delimiter found. */
            return -EILSEQ;
        }

        if (s->run) {
            /* Run of data which does not contain 0x00. */
            if (!s->capacity) {
                return -ENOSPC;
            }

            *s->decoded++ = *data;
            s->capacity--;
            s->run--;

        } else {
            if (s->offset != 0xff) {
                if (!s->capacity) {
                    return -ENOSPC;
                }

                *s->decoded++ = 0x00;
                s->capacity--;
            }

            s->offset = *data;
            s->run = s->offset - 1;
        }

        data++;
        length--;
    }

    return 0;
}

ssize_t cobs_decode_finish(struct cobs_decode_state *s)
{
    if (!s) {
        return -EFAULT;
    }

#ifdef COBS_STRICT
    /* In strict mode, the final run of non-NUL data bytes must complete. */
    if (s->run) {
        return -EMSGSIZE;
    }
#endif

    return (ssize_t)(s->decoded - s->output);
}

ssize_t cobs_decode(const uint8_t *data, size_t length, uint8_t *output, size_t capacity)
{
    struct cobs_decode_state s;
    int r;

    if (!data) {
        return -EFAULT;
    }

    r = cobs_decode_start(&s, output, capacity);
    if (r < 0) {
        return r;
    }

    while (length--) {
        r = cobs_decode_add(&s, data++, 1);
        if (r < 0) {
            return r;
        }
    }

    return cobs_decode_finish(&s);
}
