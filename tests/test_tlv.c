#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "test.h"
#include "tlv.h"
#include "dynarr.h"

/*
 * Each test serializes a value, deserializes the resulting buffer, and checks
 * that the round-trip produces the original value with the correct type tag.
 *
 * Cleanup pattern:
 *   free(b.bytes)          frees the serialized frame
 *   message_free(&msgs[i]) frees inner allocations for STR/BIN/ERR messages
 *   da_free(msgs)          frees the Message array itself
 */

// integer round-trips
static void test_u08(void) {
	Buffer b = serialize_u08(255);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_U08);
	ASSERT_EQ(message_u08(msgs[0]), (uint8_t)255);
	free(b.bytes);
	da_free(msgs);
}

static void test_u16(void) {
	Buffer b = serialize_u16(0x1234);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_U16);
	ASSERT_EQ(message_u16(msgs[0]), (uint16_t)0x1234);
	free(b.bytes);
	da_free(msgs);
}

static void test_u32(void) {
	Buffer b = serialize_u32(0xDEADBEEF);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_U32);
	ASSERT_EQ(message_u32(msgs[0]), (uint32_t)0xDEADBEEF);
	free(b.bytes);
	da_free(msgs);
}

static void test_u64(void) {
	Buffer b = serialize_u64(0xCAFEBABEDEADBEEF);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_U64);
	ASSERT_EQ(message_u64(msgs[0]), (uint64_t)0xCAFEBABEDEADBEEF);
	free(b.bytes);
	da_free(msgs);
}

static void test_s08(void) {
	Buffer b = serialize_s08(-42);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_S08);
	ASSERT_EQ(message_s08(msgs[0]), (int8_t)-42);
	free(b.bytes);
	da_free(msgs);
}

static void test_s16(void) {
	Buffer b = serialize_s16(-1000);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_S16);
	ASSERT_EQ(message_s16(msgs[0]), (int16_t)-1000);
	free(b.bytes);
	da_free(msgs);
}

static void test_s32(void) {
	Buffer b = serialize_s32(-100000);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_S32);
	ASSERT_EQ(message_s32(msgs[0]), (int32_t)-100000);
	free(b.bytes);
	da_free(msgs);
}

static void test_s64(void) {
	Buffer b = serialize_s64(INT64_MIN);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_S64);
	ASSERT_EQ(message_s64(msgs[0]), INT64_MIN);
	free(b.bytes);
	da_free(msgs);
}

// floating-point round-trips

static void test_f32(void) {
	/* 0.5 is exactly representable in IEEE-754, so a bit-identical round-trip
	 * is guaranteed. The pragma suppresses -Wfloat-equal, which fires even for
	 * exact comparisons that are intentional. */
	Buffer b = serialize_f32(0.5f);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_F32);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
	ASSERT(message_f32(msgs[0]) == 0.5f);
#pragma GCC diagnostic pop
	free(b.bytes);
	da_free(msgs);
}

static void test_f64(void) {
	/* 0.125 = 2^-3: exactly representable. */
	Buffer b = serialize_f64(0.125);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_F64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
	ASSERT(message_f64(msgs[0]) == 0.125);
#pragma GCC diagnostic pop
	free(b.bytes);
	da_free(msgs);
}

// boolean

static void test_bol(void) {
	{
		Buffer b = serialize_bol(true);
		Message *msgs = deserialize(b);
		ASSERT_EQ(msgs[0].type, T_BOL);
		ASSERT_EQ(message_bol(msgs[0]), true);
		free(b.bytes);
		da_free(msgs);
	}
	{
		Buffer b = serialize_bol(false);
		Message *msgs = deserialize(b);
		ASSERT_EQ(msgs[0].type, T_BOL);
		ASSERT_EQ(message_bol(msgs[0]), false);
		free(b.bytes);
		da_free(msgs);
	}
}

// strings

static void test_str(void) {
	/* serialize_str with an explicit String struct */
	const char *text = "hello, world";
	String s = { .bytes = (Byte*)text, .len = strlen(text) };
	Buffer b = serialize_str(s);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_STR);
	String got = message_str(msgs[0]);
	ASSERT_EQ(got.len, strlen(text));
	ASSERT(memcmp(got.bytes, text, got.len) == 0);
	free(b.bytes);
	message_free(&msgs[0]);
	da_free(msgs);
}

static void test_cstr(void) {
	/* serialize_cstr accepts a plain C string literal */
	Buffer b = serialize_cstr("socket programming");
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_STR);
	/* message_cstr returns a heap-allocated, null-terminated copy */
	char *got = message_cstr(msgs[0]);
	ASSERT_STR_EQ(got, "socket programming");
	free(got);
	free(b.bytes);
	message_free(&msgs[0]);
	da_free(msgs);
}

static void test_cstr_empty(void) {
	Buffer b = serialize_cstr("");
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_STR);
	char *got = message_cstr(msgs[0]);
	ASSERT_STR_EQ(got, "");
	free(got);
	free(b.bytes);
	message_free(&msgs[0]);
	da_free(msgs);
}

// binary data

static void test_bin(void) {
	Byte data[] = { 0x00, 0xFF, 0xAB, 0xCD };
	Buffer buf = { .bytes = data, .len = sizeof(data) };
	Buffer b = serialize_bin(buf);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_BIN);
	Buffer got = message_bin(msgs[0]);
	ASSERT_EQ(got.len, sizeof(data));
	ASSERT(memcmp(got.bytes, data, got.len) == 0);
	free(b.bytes);
	message_free(&msgs[0]);
	da_free(msgs);
}

// error string

static void test_err(void) {
	const char *msg = "file not found";
	ErrString e = { .bytes = (Byte*)msg, .len = strlen(msg) };
	Buffer b = serialize_err(e);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_ERR);
	ErrString got = message_err(msgs[0]);
	ASSERT_EQ(got.len, strlen(msg));
	ASSERT(memcmp(got.bytes, msg, got.len) == 0);
	free(b.bytes);
	message_free(&msgs[0]);
	da_free(msgs);
}

// timestamp

static void test_tim(void) {
	Timestamp ts = 1748563200;
	Buffer b = serialize_tim(ts);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_TIM);
	ASSERT_EQ(message_tim(msgs[0]), ts);
	free(b.bytes);
	da_free(msgs);
}

static void test_tim_negative(void) {
	/* Timestamps before the Unix epoch are negative */
	Timestamp ts = -86400;
	Buffer b = serialize_tim(ts);
	Message *msgs = deserialize(b);
	ASSERT_EQ(msgs[0].type, T_TIM);
	ASSERT_EQ(message_tim(msgs[0]), ts);
	free(b.bytes);
	da_free(msgs);
}

// unicode codepoint

static void test_chr(void) {
	Codepoint cp = 0x1F600; /* 😀 */
	Buffer b = serialize_chr(cp);
	Message *msgs = deserialize(b);
	ASSERT_EQ(da_length(msgs), (size_t)1);
	ASSERT_EQ(msgs[0].type, T_CHR);
	ASSERT_EQ(message_chr(msgs[0]), cp);
	free(b.bytes);
	da_free(msgs);
}

// generic serialize() macro

static void test_generic_macro(void) {
	/* Verify _Generic dispatches to the right function for scalar types.
	 * Integer literals need an explicit cast since bare literals are int. */
	{
		Buffer b = serialize((uint32_t)99);
		Message *msgs = deserialize(b);
		ASSERT_EQ(msgs[0].type, T_U32);
		ASSERT_EQ(message_u32(msgs[0]), (uint32_t)99);
		free(b.bytes);
		da_free(msgs);
	}
	{
		Buffer b = serialize((int64_t)-1);
		Message *msgs = deserialize(b);
		ASSERT_EQ(msgs[0].type, T_S64);
		ASSERT_EQ(message_s64(msgs[0]), (int64_t)-1);
		free(b.bytes);
		da_free(msgs);
	}
	{
		Buffer b = serialize(true);
		Message *msgs = deserialize(b);
		ASSERT_EQ(msgs[0].type, T_BOL);
		ASSERT_EQ(message_bol(msgs[0]), true);
		free(b.bytes);
		da_free(msgs);
	}
}

// multi-message stream via buf_append

static void test_multi_message(void) {
	/*
	 * Typical socket send pattern:
	 *   build a stream with buf_append, then write(fd, stream.bytes, stream.len)
	 *
	 * buf_append consumes both arguments, so only the final stream needs to
	 * be freed.
	 */
	Buffer stream = serialize_u32(42);
	stream = buf_append(stream, serialize_cstr("hello"));
	stream = buf_append(stream, serialize_bol(true));
	stream = buf_append(stream, serialize_tim((Timestamp)1748563200));

	Message *msgs = deserialize(stream);
	ASSERT_EQ(da_length(msgs), (size_t)4);

	ASSERT_EQ(msgs[0].type, T_U32);
	ASSERT_EQ(message_u32(msgs[0]), (uint32_t)42);

	ASSERT_EQ(msgs[1].type, T_STR);
	char *text = message_cstr(msgs[1]);
	ASSERT_STR_EQ(text, "hello");
	free(text);

	ASSERT_EQ(msgs[2].type, T_BOL);
	ASSERT_EQ(message_bol(msgs[2]), true);

	ASSERT_EQ(msgs[3].type, T_TIM);
	ASSERT_EQ(message_tim(msgs[3]), (Timestamp)1748563200);

	free(stream.bytes);
	for (size_t i = 0; i < da_length(msgs); i++)
		message_free(&msgs[i]);
	da_free(msgs);
}

// endianness

static void test_endianness(void) {
	/* These values have distinct byte patterns at every width to catch
	 * byte-swap bugs: 0x0102 must not come back as 0x0201, etc. */
	{
		Buffer b = serialize_u16(0x0102);
		Message *msgs = deserialize(b);
		ASSERT_EQ(message_u16(msgs[0]), (uint16_t)0x0102);
		free(b.bytes);
		da_free(msgs);
	}
	{
		Buffer b = serialize_u32(0x01020304);
		Message *msgs = deserialize(b);
		ASSERT_EQ(message_u32(msgs[0]), (uint32_t)0x01020304);
		free(b.bytes);
		da_free(msgs);
	}
	{
		Buffer b = serialize_u64(0x0102030405060708ULL);
		Message *msgs = deserialize(b);
		ASSERT_EQ(message_u64(msgs[0]), (uint64_t)0x0102030405060708ULL);
		free(b.bytes);
		da_free(msgs);
	}
	{
		/* s64 and tim share the same 64-bit big-endian path */
		Buffer b = serialize_tim((Timestamp)0x0102030405060708LL);
		Message *msgs = deserialize(b);
		ASSERT_EQ(message_tim(msgs[0]), (Timestamp)0x0102030405060708LL);
		free(b.bytes);
		da_free(msgs);
	}
}

// main

int main(void) {
	test_u08();
	test_u16();
	test_u32();
	test_u64();
	test_s08();
	test_s16();
	test_s32();
	test_s64();
	test_f32();
	test_f64();
	test_bol();
	test_str();
	test_cstr();
	test_cstr_empty();
	test_bin();
	test_err();
	test_tim();
	test_tim_negative();
	test_chr();
	test_generic_macro();
	test_multi_message();
	test_endianness();
	printf("test_tlv: all tests passed\n");
	return 0;
}
