#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"

// assert: str is a valid pointer
// assert: off + off_len <= str->len

static string_t *
string_resize(string_t *str, uint16_t off, uint16_t off_len, uint16_t len)
{
	if(!str) {
		assert(!off & !off_len);
		str = string_alloc(string_next_size(len));
		str->len = len;
		str->free -= len;
		return str;
	}
	assert(off + off_len <= str->len);

	uint32_t new_len = str->len - off_len + len;
	uint32_t end = off + off_len;
	if(len > str->free + off_len) {
		string_t *old_str = str;
		uint32_t new_alloc = (new_len / SEGMENT_SIZE + 1) * SEGMENT_SIZE;
		str = string_alloc(new_alloc);
		if(!str) {
			// FIXME: error handling
		}
		memcpy(str->buf, old_str->buf, off);
		memcpy(&str->buf[off + len], &old_str->buf[end], old_str->len - end);
		str->free = new_alloc - new_len;
		free(old_str);
	} else {
		memmove(&str->buf[off + len], &str->buf[end], str->len - end);
	}
	str->len = new_len;
	return str;
}
