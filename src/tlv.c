#include "tlv.h"
#include "dynarr.h"
#include "prelude.h"
#include <stdlib.h>
#include <arpa/inet.h>

#define simple_serialize(val, type) do {                                       \
	Byte* _b = malloc(1     /* type */                                         \
			+ 4             /* length */                                       \
			+ sizeof(val) /* value */);                                        \
	exists(_b);                                                                \
	uint8_t _tag = (uint8_t)type;                                              \
	uint32_t _len = htonl(sizeof(val));                                        \
	memcpy(_b, &_tag, 1);                                                      \
	memcpy(_b + 1, &_len, 4);                                                  \
	memcpy(_b + 5, &val, sizeof(val));                                         \
	return (Buffer){ .bytes = (Byte*)_b, .len = 5 + sizeof(val), };            \
} while(0)

#define simple_serialize_16(val, type) do {                                    \
	Byte* _b = malloc(1     /* type */                                         \
			+ 4             /* length */                                       \
			+ sizeof(val) /* value */);                                        \
	exists(_b);                                                                \
	uint8_t _tag = (uint8_t)type;                                              \
	uint32_t _len = htonl(sizeof(val));                                        \
	uint16_t _target = htons(val);                                             \
	memcpy(_b, &_tag, 1);                                                      \
	memcpy(_b + 1, &_len, 4);                                                  \
	memcpy(_b + 5, &_target, sizeof(val));                                     \
	return (Buffer){ .bytes = (Byte*)_b, .len = 5 + sizeof(val), };            \
} while(0)

#define simple_serialize_32(val, type) do {                                    \
	Byte* _b = malloc(1     /* type */                                         \
			+ 4             /* length */                                       \
			+ sizeof(val) /* value */);                                        \
	exists(_b);                                                                \
	uint8_t _tag = (uint8_t)type;                                              \
	uint32_t _len = htonl(sizeof(val));                                        \
	uint32_t _target = htonl(val);                                             \
	memcpy(_b, &_tag, 1);                                                      \
	memcpy(_b + 1, &_len, 4);                                                  \
	memcpy(_b + 5, &_target, sizeof(val));                                     \
	return (Buffer){ .bytes = (Byte*)_b, .len = 5 + sizeof(val), };            \
} while(0)

#define simple_serialize_64(val, type) do {                                    \
	Byte* _b = malloc(1     /* type */                                         \
			+ 4             /* length */                                       \
			+ sizeof(val) /* value */);                                        \
	exists(_b);                                                                \
	uint8_t _tag = (uint8_t)type;                                              \
	uint32_t _len = htonl(sizeof(val));                                        \
	uint64_t _target = hton64(val);                                            \
	memcpy(_b, &_tag, 1);                                                      \
	memcpy(_b + 1, &_len, 4);                                                  \
	memcpy(_b + 5, &_target, sizeof(val));                                     \
	return (Buffer){ .bytes = (Byte*)_b, .len = 5 + sizeof(val), };            \
} while(0)

#define complex_serialize(val, type) do {                                      \
	Byte* bytes = malloc(1 /* type */                                          \
			+ 4            /* length */                                        \
			+ val.len      /* value */);                                       \
	exists(bytes);                                                             \
	uint8_t tag = (uint8_t)type;                                               \
	if (val.len > UINT32_MAX) {                                              \
		fprintf(stderr, "%s:%s:%d sending a buffer that is too big: %zu\n",    \
				__FILE__, __func__, __LINE__, val.len);                        \
		exit(EXIT_FAILURE);                                                    \
	}                                                                          \
	uint32_t len = htonl((uint32_t)val.len);                                   \
	memcpy(bytes, &tag, 1);                                                    \
	memcpy(bytes + 1, &len, 4);                                                \
	memcpy(bytes + 5, val.bytes, val.len);                                     \
	return (Buffer){ .bytes = (Byte*)bytes, .len = 5 + val.len };              \
} while(0)

Buffer serialize_u08(uint8_t n) {
	simple_serialize(n, T_U08);
}

Buffer serialize_u16(uint16_t n) {
	simple_serialize_16(n, T_U16);
}

Buffer serialize_u32(uint32_t n) {
	simple_serialize_32(n, T_U32);
}

Buffer serialize_chr(uint32_t n) {
	simple_serialize_32(n, T_CHR);
}

Buffer serialize_u64(uint64_t n) {
	simple_serialize_64(n, T_U64);
}

Buffer serialize_s08(int8_t n) {
	simple_serialize(n, T_S08);
}

Buffer serialize_s16(int16_t n) {
	simple_serialize_16((uint16_t)n, T_S16);
}

Buffer serialize_s32(int32_t n) {
	simple_serialize_32((uint32_t)n, T_S32);
}

Buffer serialize_s64(int64_t n) {
	simple_serialize_64((uint64_t)n, T_S64);
}

Buffer serialize_tim(int64_t n) {
	simple_serialize_64((uint64_t)n, T_TIM);
}

Buffer serialize_f32(float n) {
	uint32_t tmp;
	memcpy(&tmp, &n, sizeof(float));
	simple_serialize_32(tmp, T_F32);
}

Buffer serialize_f64(double n) {
	uint64_t tmp;
	memcpy(&tmp, &n, sizeof(double));
	simple_serialize_64(tmp, T_F64);
}

Buffer serialize_bol(bool n) {
	simple_serialize(n, T_BOL);
}

Buffer serialize_bin(Buffer buf) {
	complex_serialize(buf, T_BIN);
}

Buffer serialize_str(String string) {
	complex_serialize(string, T_STR);
}

Buffer serialize_err(ErrString error) {
	complex_serialize(error, T_ERR);
}

Buffer serialize_cstr(const char *s) {
	return serialize_str((String){ .bytes = (Byte*)s, .len = strlen(s) });
}

Message* deserialize(Buffer buf) {
	exists(buf.bytes);
	Message* messages = da_new(Message);

	for (size_t offset = 0, len = 0; offset < buf.len; offset += 5 + len) {
		Byte* current = buf.bytes + offset;

		uint8_t tag;
		memcpy(&tag, current, 1);
		Type type = (Type)tag;

		uint32_t raw_len;
		memcpy(&raw_len, current + 1, 4);
		len = ntohl(raw_len);

		Message message = {0};
		message.type = type;

		switch (type) {
			case T_NUL: break;
			case T_U08:
				memcpy(&message.u08, current + 5, 1);
				break;
			case T_U16: {
				uint16_t tmp;
				memcpy(&tmp, current + 5, 2);
				message.u16 = ntohs(tmp);
			} break;
			case T_U32: {
				uint32_t tmp;
				memcpy(&tmp, current + 5, 4);
				message.u32 = ntohl(tmp);
			} break;
			case T_U64: {
				uint64_t tmp;
				memcpy(&tmp, current + 5, 8);
				message.u64 = ntoh64(tmp);
			} break;
			case T_S08:
				memcpy(&message.s08, current + 5, 1);
				break;
			case T_S16: {
				uint16_t tmp;
				memcpy(&tmp, current + 5, 2);
				message.s16 = (int16_t)ntohs(tmp);
			} break;
			case T_S32: {
				uint32_t tmp;
				memcpy(&tmp, current + 5, 4);
				message.s32 = (int32_t)ntohl(tmp);
			} break;
			case T_S64: {
				int64_t tmp;
				memcpy(&tmp, current + 5, 8);
				message.s64 = (int64_t)ntoh64((uint64_t)tmp);
			} break;
			case T_F32: {
				uint32_t tmp1;
				memcpy(&tmp1, current + 5, sizeof(float));
				tmp1 = ntohl(tmp1);
				float tmp2;
				memcpy(&tmp2, &tmp1, sizeof(float));
				message.f32 = tmp2;
			} break;
			case T_F64: {
				uint64_t tmp1;
				memcpy(&tmp1, current + 5, sizeof(double));
				tmp1 = ntoh64(tmp1);
				double tmp2;
				memcpy(&tmp2, &tmp1, sizeof(double));
				message.f64 = tmp2;
			} break;
			case T_BOL:
				memcpy(&message.bol, current + 5, 1);
				break;
			case T_TIM: {
				Timestamp tmp;
				memcpy(&tmp, current + 5, 8);
				message.tim = (Timestamp)ntoh64((uint64_t)tmp);
			} break;
			case T_CHR: {
				Codepoint tmp;
				memcpy(&tmp, current + 5, 4);
				message.chr = ntohl(tmp);
			} break;
			case T_BIN: {
				Byte* bytes = malloc(len);
				exists(bytes);
				memcpy(bytes, current + 5, len);
				message.bin.bytes = bytes;
				message.bin.len = len;
			} break;
			case T_ERR: {
				Byte* bytes = malloc(len);
				exists(bytes);
				memcpy(bytes, current + 5, len);
				message.err.bytes = bytes;
				message.err.len = len;
			} break;
			case T_STR: {
				Byte* bytes = malloc(len);
				exists(bytes);
				memcpy(bytes, current + 5, len);
				message.str.bytes = bytes;
				message.str.len = len;
			} break;
			default: {
				fprintf(stderr, "%s:%s:%d Received a bad type: %02X\n", __FILE__, __func__, __LINE__, type);
				exit(EXIT_FAILURE);
			};
		}

		da_append(messages, message);
	}
	return messages;
}

void message_free(Message *m) {
	switch (m->type) {
		case T_STR:
		case T_ERR:
		case T_BIN:
			free(m->bin.bytes);
			m->bin.bytes = NULL;
			m->bin.len   = 0;
			break;
		default:
			break;
	}
}

uint8_t   message_u08(Message m) {
	expect(m.type == T_U08, "Tried accessing a message of type %s but got %s",
			strtype(T_U08), strtype(m.type));
	return m.u08;
}

uint16_t  message_u16(Message m) {
	expect(m.type == T_U16, "Tried accessing a message of type %s but got %s",
			strtype(T_U16), strtype(m.type));
	return m.u16;
}

uint32_t  message_u32(Message m) {
	expect(m.type == T_U32, "Tried accessing a message of type %s but got %s",
			strtype(T_U32), strtype(m.type));
	return m.u32;
}

uint64_t  message_u64(Message m) {
	expect(m.type == T_U64, "Tried accessing a message of type %s but got %s",
			strtype(T_U64), strtype(m.type));
	return m.u64;
}

int8_t    message_s08(Message m) {
	expect(m.type == T_S08, "Tried accessing a message of type %s but got %s",
			strtype(T_S08), strtype(m.type));
	return m.s08;
}

int16_t   message_s16(Message m) {
	expect(m.type == T_S16, "Tried accessing a message of type %s but got %s",
			strtype(T_S16), strtype(m.type));
	return m.s16;
}

int32_t   message_s32(Message m) {
	expect(m.type == T_S32, "Tried accessing a message of type %s but got %s",
			strtype(T_S32), strtype(m.type));
	return m.s32;
}

int64_t   message_s64(Message m) {
	expect(m.type == T_S64, "Tried accessing a message of type %s but got %s",
			strtype(T_S64), strtype(m.type));
	return m.s64;
}

Timestamp   message_tim(Message m) {
	expect(m.type == T_TIM, "Tried accessing a message of type %s but got %s",
			strtype(T_TIM), strtype(m.type));
	return m.tim;
}

Codepoint  message_chr(Message m) {
	expect(m.type == T_CHR, "Tried accessing a message of type %s but got %s",
			strtype(T_CHR), strtype(m.type));
	return m.chr;
}


float     message_f32(Message m) {
	expect(m.type == T_F32, "Tried accessing a message of type %s but got %s",
			strtype(T_F32), strtype(m.type));
	return m.f32;
}

double    message_f64(Message m) {
	expect(m.type == T_F64, "Tried accessing a message of type %s but got %s",
			strtype(T_F64), strtype(m.type));
	return m.f64;
}

bool      message_bol(Message m) {
	expect(m.type == T_BOL, "Tried accessing a message of type %s but got %s",
			strtype(T_BOL), strtype(m.type));
	return m.bol;
}

Buffer    message_bin(Message m) {
	expect(m.type == T_BIN, "Tried accessing a message of type %s but got %s",
			strtype(T_BIN), strtype(m.type));
	return m.bin;
}

String    message_str(Message m) {
	expect(m.type == T_STR, "Tried accessing a message of type %s but got %s",
			strtype(T_STR), strtype(m.type));
	return m.str;
}

char *message_cstr(Message m) {
	String s = message_str(m);
	char *out = malloc(s.len + 1);
	exists(out);
	memcpy(out, s.bytes, s.len);
	out[s.len] = '\0';
	return out;
}

ErrString message_err(Message m) {
	expect(m.type == T_ERR, "Tried accessing a message of type %s but got %s",
			strtype(T_ERR), strtype(m.type));
	return m.err;
}

