#include "buffer.h"
#include "prelude.h"
#include <stdio.h>
#include <string.h>


Buffer buf_append(Buffer base, Buffer after) {
	Byte* new_bytes = realloc(base.bytes, base.len + after.len);
	exists(new_bytes);
	memcpy(new_bytes + base.len, after.bytes, after.len);
	free(after.bytes);
	return (Buffer){
		.bytes = new_bytes,
		.len = base.len + after.len,
	};
}
