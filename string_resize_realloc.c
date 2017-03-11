#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"

string_t *
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
		str = string_realloc(old_str, string_next_size(new_len));
		if(!str) {
			// FIXME: error handling, also free(old_str) ?
		}
		str->free = new_alloc - new_len;
		str->free -= new_len;
	}
	memmove(&str->buf[off + len], &str->buf[end], str->len - end);
	str->len = new_len;
	return str;
}
