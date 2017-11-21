#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "string.h"
#include "string_next_size.h"

string_t *
string_alloc(stroff_t size)
{
	string_t *str = malloc(sizeof(str[0]) + size * sizeof(str->buf[0]));
	str->len = 0;
	str->size = size;
	return str;
}

string_t *
string_realloc(string_t *str, stroff_t size)
{
	string_t *new_str = realloc(str, sizeof(str[0]) + size * sizeof(str->buf[0]));
	if(!str) {
		new_str->len = 0;
	}
	new_str->size = size;
	return new_str;
}

string_t *string_resize_priv(string_t **pstr, stroff_t size, stroff_t sub_off, stroff_t sub_end, stroff_t new_sub_len);

string_t *
string_resize(string_t **pstr, stroff_t sub_off, stroff_t sub_len, stroff_t new_sub_len)
{
	string_t *str = *pstr;
	if(!pstr || !str) {
		assert(!sub_off && !sub_len);
		str = string_alloc(string_next_size(new_sub_len));
		str->len = new_sub_len;
		*pstr = str;
		return str;
	}
	assert(sub_off + sub_len <= str->len);

	stroff_t new_len = str->len - sub_len + new_sub_len;
	stroff_t sub_end = sub_off + sub_len;

	if(new_sub_len < sub_len) {
		memmove(&str->buf[sub_off + sub_len], &str->buf[sub_end], str->len - sub_end);
	}

	if(new_len > str->size || new_len * 4 < str->size) {
		str = string_resize_priv(pstr, string_next_size(new_len), sub_off, sub_end, new_sub_len);
	}

	if(new_sub_len > sub_len) {
		memmove(&str->buf[sub_off + new_sub_len], &str->buf[sub_end], str->len - sub_end);
	}

	str->len = new_len;
	return str;
}

string_t *
string_replace_multi(string_t **pstr, stroff_t sub_off, stroff_t sub_len, rawbuf_t *mod, uint32_t nmod)
{
	rawbuf_t empty = {0};
	if(nmod == 0) {
		mod = &empty;
		nmod = 1;
	}
	stroff_t len = 0;
	for(uint32_t i = 0; i < nmod; i++) {
		len += mod[i].len;
	}
	string_t *str = string_resize(pstr, sub_off, sub_len, len);
	for(uint32_t i = 0; i < nmod; i++) {
		memcpy(&str->buf[sub_off], mod[i].buf, mod[i].len);
		sub_off += mod[i].len;
	}
	return str;
}

string_t *
string_replace(string_t **pstr, stroff_t sub_off, stroff_t sub_len, char *buf, stroff_t len)
{
	return string_replace_multi(pstr, sub_off, sub_len, &(rawbuf_t){buf, len}, 1);
}
