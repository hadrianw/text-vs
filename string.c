#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"

string_t *
string_alloc(uint16_t size)
{
	string_t *str = malloc(sizeof(str[0]) + size * sizeof(str->buf[0]));
	str->len = 0;
	str->free = size;
	return str;
}

string_t *
string_realloc(string_t *str, uint16_t size)
{
	string_t *new_str = realloc(str, sizeof(str[0]) + size * sizeof(str->buf[0]));
	new_str->len = 0;
	new_str->free = size;
	return new_str;
}

int
string_replace(string_t **pstr, uint16_t off, uint16_t off_len, char *buf, uint16_t len)
{
	assert(buf != 0 || len == 0);
	*str = string_resize(*str, off, off_len, len);
	memcpy(&(*str)->buf[off], buf, len);
	return str;
}
