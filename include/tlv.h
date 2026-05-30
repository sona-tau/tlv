/**
 * @file tlv.h
 * @brief TLV (Type-Length-Value) serialization library.
 *
 * Each serialized frame has the following wire layout:
 * @code
 *   [1 byte: type tag][4 bytes: length, big-endian][N bytes: value]
 * @endcode
 *
 * Multiple frames can be concatenated with buf_append() to build a stream
 * that is sent over a socket in a single write().
 *
 * ## Ownership
 *
 * **Serializing:**
 * serialize() and all serialize_*() functions return an **owned** Buffer.
 * The caller must call @c free(buf.bytes) when done, or pass the Buffer to
 * buf_append() which consumes it.
 *
 * **Deserializing:**
 * deserialize() returns an **owned** Message* dynamic array (see dynarr.h).
 * Messages of type T_STR, T_BIN, and T_ERR also hold an inner heap
 * allocation for their payload bytes. You must free those first with
 * message_free(), then release the array with da_free():
 * @code
 *   Message *msgs = deserialize(incoming);
 *   for (size_t i = 0; i < da_length(msgs); i++)
 *       message_free(&msgs[i]);
 *   da_free(msgs);
 * @endcode
 */

#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "buffer.h"

/**
 * @brief UTF-8 text payload (semantic alias for Buffer).
 *
 * The bytes are **not** null-terminated. String, ErrString, and Buffer are
 * the same underlying C type — they exist only as documentation hints.
 *
 * Because they share a type, the serialize() macro cannot distinguish between
 * them. Call serialize_str(), serialize_err(), or serialize_bin() directly
 * when working with these types.
 */
typedef Buffer String;

/**
 * @brief UTF-8 error string payload (semantic alias for Buffer).
 * @see String
 */
typedef Buffer ErrString;

/**
 * @brief Unix timestamp (seconds since the epoch), encoded as a signed 64-bit integer.
 *
 * Timestamp is a semantic alias for int64_t. Because they share the same
 * underlying type, the serialize() macro cannot distinguish between them.
 * Call serialize_tim() directly when working with timestamps.
 */
typedef int64_t Timestamp;

/**
 * @brief A single Unicode codepoint, encoded as a 32-bit unsigned integer (UTF-32).
 *
 * Codepoint is a semantic alias for uint32_t. Because they share the same
 * underlying type, the serialize() macro cannot distinguish between them.
 * Call serialize_chr() directly when working with codepoints.
 */
typedef uint32_t Codepoint;

/**
 * @brief Identifies the value type carried by a TLV frame.
 */
typedef enum {
	T_NUL = 0, ///< No value.
	T_U08,     ///< Unsigned 8-bit integer.
	T_U16,     ///< Unsigned 16-bit integer.
	T_U32,     ///< Unsigned 32-bit integer.
	T_U64,     ///< Unsigned 64-bit integer.
	T_S08,     ///< Signed 8-bit integer.
	T_S16,     ///< Signed 16-bit integer.
	T_S32,     ///< Signed 32-bit integer.
	T_S64,     ///< Signed 64-bit integer.
	T_F32,     ///< IEEE-754 32-bit floating-point number.
	T_F64,     ///< IEEE-754 64-bit floating-point number.
	T_STR,     ///< UTF-8 string (not null-terminated).
	T_BOL,     ///< Boolean (0x00 = false, 0x01 = true).
	T_BIN,     ///< Raw binary data.
	T_TIM,     ///< Unix timestamp (Timestamp / int64_t).
	T_CHR,     ///< Single Unicode codepoint (Codepoint / uint32_t, UTF-32).
	T_ERR,     ///< UTF-8 error string (not null-terminated).
} Type;

/** @cond INTERNAL */
#define strtype(type) \
	  type == T_NUL ? "Null" \
	: type == T_U08 ? "Unsigned 8-bit Int" \
	: type == T_U16 ? "Unsigned 16-bit Int" \
	: type == T_U32 ? "Unsigned 32-bit Int" \
	: type == T_U64 ? "Unsigned 64-bit Int" \
	: type == T_S08 ? "Signed 8-bit Int" \
	: type == T_S16 ? "Signed 16-bit Int" \
	: type == T_S32 ? "Signed 32-bit Int" \
	: type == T_S64 ? "Signed 64-bit Int" \
	: type == T_F32 ? "32-bit IEEE-754 Float" \
	: type == T_F64 ? "64-bit IEEE-754 Float" \
	: type == T_STR ? "UTF-8 String" \
	: type == T_BOL ? "Boolean" \
	: type == T_BIN ? "Byte Buffer" \
	: type == T_TIM ? "UNIX Timestamp" \
	: type == T_CHR ? "Unicode Codepoint" \
	: type == T_ERR ? "UTF-8 Error String" : "Unknown"
/** @endcond */

/**
 * @brief A decoded TLV message.
 *
 * The @p type field says which union member is valid. Use the corresponding
 * message_*() accessor to read the value safely (it aborts with a clear error
 * if the type does not match).
 *
 * Messages of type T_STR, T_BIN, and T_ERR own a heap-allocated byte buffer.
 * Call message_free() before discarding them to avoid a memory leak.
 */
typedef struct {
	Type type; ///< Identifies which union member holds the value.
	union {
		uint8_t   u08;
		uint16_t  u16;
		uint32_t  u32;
		uint64_t  u64;
		int8_t    s08;
		int16_t   s16;
		int32_t   s32;
		int64_t   s64;
		float     f32;
		double    f64;
		String    str; ///< Valid when type == T_STR. Owns its bytes.
		bool      bol;
		Buffer    bin; ///< Valid when type == T_BIN. Owns its bytes.
		Timestamp tim;
		Codepoint chr;
		ErrString err; ///< Valid when type == T_ERR. Owns its bytes.
	};
} Message;

/** @name Serialize functions
 *
 * Each function serializes a single value into an owned TLV Buffer.
 * The caller is responsible for calling @c free(buf.bytes) when done,
 * or passing the Buffer to buf_append() which takes ownership.
 * @{
 */
[[nodiscard]] Buffer serialize_u08(uint8_t n);
[[nodiscard]] Buffer serialize_u16(uint16_t n);
[[nodiscard]] Buffer serialize_u32(uint32_t n);
[[nodiscard]] Buffer serialize_u64(uint64_t n);
[[nodiscard]] Buffer serialize_s08(int8_t n);
[[nodiscard]] Buffer serialize_s16(int16_t n);
[[nodiscard]] Buffer serialize_s32(int32_t n);
[[nodiscard]] Buffer serialize_s64(int64_t n);
[[nodiscard]] Buffer serialize_f32(float n);
[[nodiscard]] Buffer serialize_f64(double n);
[[nodiscard]] Buffer serialize_bol(bool n);
[[nodiscard]] Buffer serialize_bin(Buffer buf);
[[nodiscard]] Buffer serialize_str(String str);
[[nodiscard]] Buffer serialize_err(ErrString str);
[[nodiscard]] Buffer serialize_tim(Timestamp n);
[[nodiscard]] Buffer serialize_chr(Codepoint n);

/**
 * @brief Serialize a null-terminated C string.
 *
 * Convenience wrapper around serialize_str() for when you already have a
 * @c char* or a string literal. Equivalent to calling serialize_str() with
 * the length computed by strlen().
 *
 * @code
 *   Buffer b = serialize_cstr("hello");
 *   Buffer b = serialize_cstr(some_char_ptr);
 * @endcode
 *
 * @param s  Null-terminated string. Must not be NULL.
 * @return   Owned Buffer. Caller must call free(buf.bytes) when done.
 */
[[nodiscard]] Buffer serialize_cstr(const char *s);
/** @} */

/**
 * @brief Serialize any supported primitive value into a TLV Buffer.
 *
 * Uses C11 @c _Generic to dispatch to the correct serialize_*() function
 * based on the static type of @p val.
 *
 * **Ownership:** the returned Buffer is heap-allocated. Call
 * @c free(buf.bytes) when done, or pass it to buf_append().
 *
 * **Integer literals** have type @c int, which is not in the dispatch table.
 * Use an explicit cast:
 * @code
 *   Buffer b = serialize((uint32_t)42);
 *   Buffer b = serialize((int16_t)-7);
 * @endcode
 *
 * **String, ErrString, Buffer, Timestamp, and Codepoint** are type aliases
 * for primitive types and cannot be distinguished by @c _Generic. Call the
 * corresponding function directly:
 * @code
 *   Buffer b = serialize_str(my_string);
 *   Buffer b = serialize_err(my_error);
 *   Buffer b = serialize_bin(my_buffer);
 *   Buffer b = serialize_tim(my_timestamp);
 *   Buffer b = serialize_chr(my_codepoint);
 * @endcode
 */
#define serialize(val) _Generic((val),   \
	uint8_t:   serialize_u08,            \
	uint16_t:  serialize_u16,            \
	uint32_t:  serialize_u32,            \
	uint64_t:  serialize_u64,            \
	int8_t:    serialize_s08,            \
	int16_t:   serialize_s16,            \
	int32_t:   serialize_s32,            \
	int64_t:   serialize_s64,            \
	float:     serialize_f32,            \
	double:    serialize_f64,            \
	bool:      serialize_bol             \
)(val)


/**
 * @brief Deserialize a TLV byte stream into an array of Messages.
 *
 * Parses @p buf as a sequence of concatenated TLV frames and returns a
 * dynamic array (see dynarr.h) containing one decoded Message per frame.
 *
 * **Ownership:** the caller owns the returned array. Messages of type T_STR,
 * T_BIN, and T_ERR each own an inner heap allocation for their payload bytes.
 * Free those first with message_free(), then release the array with da_free():
 * @code
 *   Message *msgs = deserialize(incoming);
 *   for (size_t i = 0; i < da_length(msgs); i++)
 *       message_free(&msgs[i]);
 *   da_free(msgs);
 * @endcode
 *
 * @param buf  Buffer containing the raw TLV byte stream.
 * @return     Heap-allocated Message* array. Aborts on allocation failure.
 */
[[nodiscard]] Message* deserialize(Buffer buf);

/**
 * @brief Free the inner allocation of a Message, if any.
 *
 * Messages of type T_STR, T_BIN, and T_ERR hold a heap-allocated byte buffer
 * in their payload. This function frees that buffer and zeroes the relevant
 * fields. For all other types this is a no-op.
 *
 * The Message struct itself is stored by value inside the array returned by
 * deserialize() — it does not need to be freed separately.
 *
 * @param m  Pointer to the Message to free. Must not be NULL.
 */
void message_free(Message *m);

/** @name Message accessors
 *
 * Each accessor checks that the Message holds the expected type and aborts
 * with a descriptive error message if not. This catches type mismatches early
 * during development.
 * @{
 */
uint8_t   message_u08(Message m);
uint16_t  message_u16(Message m);
uint32_t  message_u32(Message m);
uint64_t  message_u64(Message m);
int8_t    message_s08(Message m);
int16_t   message_s16(Message m);
int32_t   message_s32(Message m);
int64_t   message_s64(Message m);
float     message_f32(Message m);
double    message_f64(Message m);
bool      message_bol(Message m);
Buffer    message_bin(Message m);
String    message_str(Message m);
ErrString message_err(Message m);
Timestamp message_tim(Message m);
Codepoint message_chr(Message m);

/**
 * @brief Extract a T_STR message as a heap-allocated, null-terminated C string.
 *
 * Convenience wrapper around message_str() for when you need a plain
 * @c char* you can pass to printf(), strcmp(), etc.
 *
 * @code
 *   char *text = message_cstr(msgs[i]);
 *   printf("%s\n", text);
 *   free(text);
 * @endcode
 *
 * @param m  A Message with type T_STR. Aborts if the type does not match.
 * @return   Heap-allocated null-terminated string. Caller must call free().
 */
[[nodiscard]] char *message_cstr(Message m);
/** @} */


/** @cond INTERNAL */
static inline uint64_t hton64(uint64_t val) {
	uint32_t hi = htonl((uint32_t)(val >> 32));
	uint32_t lo = htonl((uint32_t)(val & 0xFFFFFFFF));
	return ((uint64_t)lo << 32) | hi;
}

static inline uint64_t ntoh64(uint64_t val) {
	return hton64(val); // the operation is its own inverse
}
/** @endcond */
