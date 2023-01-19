#ifndef LIBCOBS_COBS_H_
#define LIBCOBS_COBS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/// Get maximum encoded size for data of given length.
/// @return Number of bytes required to encode data of given length, or 0 on overflow.
size_t cobs_maximum_sizeof(size_t length);

/// Models a consistent overhead byte stuffing encoder.
/// @see https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
struct cobs_encode_state;

/// Create encoder object.
/// @note Memory ownership: Caller must cobs_encode_delete() the returned pointer.
struct cobs_encode_state *cobs_encode_new(void);

/// Clears encoder state.
/// @return Zero on success, negative errno otherwise.
int cobs_encode_clear(struct cobs_encode_state *);

/// Start encoding.
/// Encode a byte stream so that there are no NUL bytes.
/// @return Zero on success, negative errno otherwise.
int cobs_encode_start(struct cobs_encode_state *, uint8_t *output, size_t capacity);

/// Add @c data.
/// @return Zero on success, negative errno otherwise.
int cobs_encode_add(struct cobs_encode_state *, const uint8_t *data, size_t length);

/// Finish encoding.
/// @return Number of bytes written to @c output, negative errno otherwise.
ssize_t cobs_encode_finish(struct cobs_encode_state *);

/// Destructor.
/// @note Memory ownership: Takes ownership of the pointer.
void cobs_encode_delete(struct cobs_encode_state *);

/// Encode @c data using consistent overhead byte stuffing.
/// Convenience function.
/// @note For efficiency, use the lower-level API if encoding large data.
/// @see cobs_encode_start
/// @see cobs_encode_add
/// @see cobs_encode_finish
/// @return Number of bytes written to @c output on success, negative errno otherwise.
/// @note Memory ownership: Caller retains ownership of all pointers.
ssize_t cobs_encode(const uint8_t *data, size_t length, uint8_t *output, size_t capacity);

/// Models a consistent overhead byte stuffing decoder.
/// @see https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
struct cobs_decode_state;

/// Create decoder object.
/// @note Memory ownership: Caller must cobs_decode_delete() the returned pointer.
struct cobs_decode_state *cobs_decode_new(void);

/// Clears decoder state.
/// @return Zero on success, negative errno otherwise.
int cobs_decode_clear(struct cobs_decode_state *);

/// Start decoding.
/// Decode a byte-stuffed stream.
/// @return Zero on success, negative errno otherwise.
int cobs_decode_start(struct cobs_decode_state *, uint8_t *output, size_t capacity);

/// Add @c data.
/// @note The byte stream may not legally contain a NUL byte.
/// @return Zero on success, negative errno otherwise.
int cobs_decode_add(struct cobs_decode_state *, const uint8_t *data, size_t length);

/// Finish decoding.
/// @param strict In strict mode, the final run of non-NUL data bytes must complete.
/// @return Number of bytes written to @c output, negative errno otherwise.
ssize_t cobs_decode_finish(struct cobs_decode_state *, bool strict);

/// Destructor.
/// @note Memory ownership: Takes ownership of the pointer.
void cobs_decode_delete(struct cobs_decode_state *);

/// Decode byte stuffed @c data.
/// Convenience function.
/// @note For efficiency, use the lower-level API if decoding large data.
/// @see cobs_decode_start
/// @see cobs_decode_add
/// @see cobs_decode_finish
/// @return Number of bytes written to @c output on success, negative errno otherwise.
/// @note Memory ownership: Caller retains ownership of all pointers.
ssize_t cobs_decode(const uint8_t *data, size_t length, uint8_t *output, size_t capacity, bool strict);

#endif
