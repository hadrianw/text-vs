#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"

string_t *
string_resize_priv(string_t **pstr, stroff_t size, stroff_t sub_off, stroff_t sub_end, stroff_t new_sub_len)
{
	(void)sub_off; (void)sub_end; (void)new_sub_len;

	string_t *new_str = string_realloc(*pstr, size);
	if(!new_str) {
		// FIXME: error handling, also free(*str) ?
	}
	*pstr = new_str;
	return new_str;
}
