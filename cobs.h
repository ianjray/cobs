#ifndef COBS__H
#define COBS__H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/// @return size_t The maximum encoded size of data having given @c length.
size_t cobs_maximum_sizeof(size_t length);

/// Create encoder object.
struct cobs_encode_state *cobs_encode_new(void);

/// Clears encoder state.
int cobs_encode_clear(struct cobs_encode_state *);

/// Start encoding.
/// Encode a byte stream so that there are no NUL bytes.
/// @see https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
/// @return int Zero on success, otherwise negative errno.
int cobs_encode_start(struct cobs_encode_state *, uint8_t *output, size_t capacity);

/// Add @c data.
/// @return int Zero on success, otherwise negative errno.
int cobs_encode_add(struct cobs_encode_state *, const uint8_t *data, size_t length);

/// Finish encoding.
/// @return ssize_t Number of bytes written to @c output, otherwise negative errno.
ssize_t cobs_encode_finish(struct cobs_encode_state *);

/// Destructor.
void cobs_encode_delete(struct cobs_encode_state *);

/// Encode @c data using consistent overhead byte stuffing.
/// @discussion Convenience function.
/// @see cobs_encode_start
/// @see cobs_encode_add
/// @see cobs_encode_finish
/// @return ssize_t Number of bytes written to @c output, otherwise negative errno.
ssize_t cobs_encode(const uint8_t *data, size_t length, uint8_t *output, size_t capacity);

/// Create decoder object.
struct cobs_decode_state *cobs_decode_new(void);

/// Clears decoder state.
int cobs_decode_clear(struct cobs_decode_state *);

/// Start decoding.
/// Decode a byte-stuffed stream.
/// @return int Zero on success, otherwise negative errno.
int cobs_decode_start(struct cobs_decode_state *, uint8_t *output, size_t capacity);

/// Add @c data.
/// @note The byte stream may not legally contain a NUL byte.
/// @return int Zero on success, otherwise negative errno.
int cobs_decode_add(struct cobs_decode_state *, const uint8_t *data, size_t length);

/// Finish decoding.
/// In strict mode, the final run of non-NUL data bytes must complete.
/// @return ssize_t Number of bytes written to @c output, otherwise negative errno.
ssize_t cobs_decode_finish(struct cobs_decode_state *, bool strict);

/// Destructor.
void cobs_decode_delete(struct cobs_decode_state *);

/// Decode byte stuffed @c data.
/// @discussion Convenience function.
/// @see cobs_decode_start
/// @see cobs_decode_add
/// @see cobs_decode_finish
/// @return ssize_t Number of bytes written to @c output on success, otherwise negative errno.
ssize_t cobs_decode(const uint8_t *data, size_t length, uint8_t *output, size_t capacity, bool strict);

#endif
