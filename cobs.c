#include "cobs.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/// Maximum offset value.
#define COBS_OFFSET_MAX 255

/// Maximum run length of non-NUL bytes.
#define COBS_MAX_RUN_LENGTH (COBS_OFFSET_MAX - 1)

struct cobs_encode_state
{
    /// Pointer to output buffer.
    uint8_t *output;
    /// Capacity of output buffer.
    size_t capacity;
    /// Pointer to last encoded byte.
    uint8_t *encoded;
    /// Pointer to offset storage.
    uint8_t *offset_storage;
    /// Offset to next NUL byte.
    uint8_t offset;
};

struct cobs_decode_state
{
    /// Decoded buffer.
    uint8_t *output;
    /// Decoded buffer capacity.
    size_t capacity;
    /// Pointer to last decoded byte.
    uint8_t *decoded;
    /// Offset to next NUL byte.
    uint8_t offset;
    /// Number of non-NUL bytes to emit.
    uint8_t run;
};

/// Fixed overhead of one, for the initial byte.
/// Additional overhead of one byte for every COBS_MAX_RUN_LENGTH non-NUL bytes.
size_t cobs_maximum_sizeof(size_t length)
{
    // Check for overflow conditions
    // Maximum safe length is SIZE_MAX - 1 - (SIZE_MAX / COBS_MAX_RUN_LENGTH)
    // This ensures 1 + length + (length / COBS_MAX_RUN_LENGTH) won't overflow

    if (length > SIZE_MAX - 1) {
        return 0; // Indicate overflow condition
    }

    size_t overhead = length / COBS_MAX_RUN_LENGTH;

    // Check if 1 + length + overhead would overflow
    if (length > SIZE_MAX - 1 - overhead) {
        return 0; // Indicate overflow condition
    }

    return 1 + length + overhead;
}

struct cobs_encode_state *cobs_encode_new(void)
{
    struct cobs_encode_state *s = calloc(1, sizeof(struct cobs_encode_state));
    return s;
}

int cobs_encode_clear(struct cobs_encode_state *s)
{
    if (!s) {
        return -EFAULT;
    }

    memset(s, 0, sizeof(struct cobs_encode_state));
    return 0;
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
    s->offset_storage = s->encoded++;
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

        if (!*data || s->offset == COBS_OFFSET_MAX) {
            // Delimiter found, or offset reaches maximum.
            *s->offset_storage = s->offset;

            if (!s->capacity) {
                return -ENOSPC;
            }

            s->offset_storage = s->encoded++;
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

    *s->offset_storage = s->offset;

    return (ssize_t)(s->encoded - s->output);
}

void cobs_encode_delete(struct cobs_encode_state *s)
{
    free(s);
}

ssize_t cobs_encode(const uint8_t *data, size_t length, uint8_t *output, size_t capacity)
{
    struct cobs_encode_state s;
    int r;

    if (!data) {
        return -EFAULT;
    }

    cobs_encode_clear(&s);

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

struct cobs_decode_state *cobs_decode_new(void)
{
    struct cobs_decode_state *s = calloc(1, sizeof(struct cobs_decode_state));
    return s;
}

int cobs_decode_clear(struct cobs_decode_state *s)
{
    if (!s) {
        return -EFAULT;
    }

    memset(s, 0, sizeof(struct cobs_decode_state));
    return 0;
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

    s->offset = COBS_OFFSET_MAX;
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
            // Delimiter found.
            return -EILSEQ;
        }

        if (s->run) {
            // Run of data which does not contain 0x00.
            if (!s->capacity) {
                return -ENOSPC;
            }

            *s->decoded++ = *data;
            s->capacity--;
            s->run--;

        } else {
            if (s->offset != COBS_OFFSET_MAX) {
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

ssize_t cobs_decode_finish(struct cobs_decode_state *s, bool strict)
{
    if (!s) {
        return -EFAULT;
    }

    // In strict mode, the final run of non-NUL data bytes must complete.
    if (strict && s->run) {
        return -EMSGSIZE;
    }

    return (ssize_t)(s->decoded - s->output);
}

void cobs_decode_delete(struct cobs_decode_state *s)
{
    free(s);
}

ssize_t cobs_decode(const uint8_t *data, size_t length, uint8_t *output, size_t capacity, bool strict)
{
    struct cobs_decode_state s;
    int r;

    if (!data) {
        return -EFAULT;
    }

    cobs_decode_clear(&s);

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

    return cobs_decode_finish(&s, strict);
}
