/**
 * @file buffer.h
 * @brief Byte buffer type and helpers.
 */

#pragma once
#include <stddef.h>
#include <stdint.h>

typedef uint8_t Byte;

/**
 * @brief A view of a contiguous block of bytes.
 *
 * A Buffer does not carry ownership information by itself — whether the bytes
 * are heap-allocated and who is responsible for freeing them depends on where
 * the Buffer came from:
 *
 * - Buffers returned by serialize_*() own their bytes. The caller must call
 *   free(buf.bytes) when done, or pass the Buffer to buf_append() which takes
 *   ownership.
 * - Buffers returned by message_bin(), message_str(), or message_err() are
 *   owned by the parent Message. Free them through message_free(), not
 *   directly.
 */
typedef struct {
	Byte*  bytes; ///< Pointer to the first byte.
	size_t len;   ///< Number of bytes.
} Buffer;

/**
 * @brief Append @p after onto the end of @p base and return the merged Buffer.
 *
 * This function **consumes both arguments**: it extends (or reallocates)
 * @p base.bytes in place and frees @p after.bytes. Do not use either Buffer
 * after calling this function.
 *
 * Typical usage — build a multi-message stream before sending over a socket:
 * @code
 *   Buffer stream = serialize((uint32_t)42);
 *   stream = buf_append(stream, serialize((float)3.14f));
 *   stream = buf_append(stream, serialize_str(name));
 *   send(fd, stream.bytes, stream.len, 0);
 *   free(stream.bytes);
 * @endcode
 *
 * @param base   The buffer to extend. Its bytes pointer is consumed.
 * @param after  The buffer to append. Its bytes pointer is consumed.
 * @return       A new Buffer whose bytes pointer is owned by the caller.
 */
[[nodiscard]] Buffer buf_append(Buffer base, Buffer after);
