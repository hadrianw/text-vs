#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"

string_t *
string_resize_priv(string_t **pstr, stroff_t size, stroff_t sub_off, stroff_t sub_end, stroff_t new_sub_len)
{
	string_t *old_str = *pstr;
	string_t *new_str = string_alloc(size);
	if(!new_str) {
		// FIXME: error handling
	}
	memcpy(new_str->buf, old_str->buf, sub_off);
	memcpy(&new_str->buf[sub_off + new_sub_len], &old_str->buf[sub_end], old_str->len - sub_end);

	free(old_str);
	*pstr = new_str;
	return new_str;
}
