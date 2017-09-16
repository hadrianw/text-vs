#define _XOPEN_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "string.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a) / sizeof(a)[0])

#define TEST(X) void TEST_##X(void)

typedef struct {
	uint32_t len;
	uint32_t size;
	string_t **lines;
} file_t;

typedef struct {
	uint32_t line;
	uint16_t offset;
} address_t;

typedef struct {
	address_t start;
	address_t end;
} range_t;

/*
typedef struct {
	struct __attribute__((__packed__)) {
		uint32_t line;
		uint16_t offset;
	} start;
	struct __attribute__((__packed__)) {
		uint16_t offset;
		uint32_t line;
	} end;
} range_t;
*/

static inline bool
is_str_eq(const char *s1, size_t n1, const char *s2, size_t n2)
{
	if(n1 != n2) {
		return false;
	}
	return !memcmp(s1, s2, MIN(n1, n2));
}

#define SEGMENT_SIZE 512
static uint32_t
next_size(uint32_t len)
{
	return (len / SEGMENT_SIZE + 1) * SEGMENT_SIZE;
}

static void
file_lines_mod(file_t *file, range_t *rng, rawbuf_t *mod, uint32_t nmod)
{
	rawbuf_t empty = {0};
	if(nmod == 0) {
		mod = &empty;
		nmod = 1;
	}

	uint32_t nsel = rng->end.line - rng->start.line + 1;
	uint32_t file_mod_last =  rng->start.line + nmod-1;

	if(nmod == nsel) {
		uint16_t off = 0;
		if(nmod == 1) {
			off = rng->start.offset;
		}
		
 		(void)string_replace_multi(
 			&file->lines[file_mod_last],
 			off, rng->end.offset - off,
 			&mod[nmod-1], 1
 		);

		if(nmod == 1) {
			return;
		}

	}
	
	uint32_t new_file_len = file->len + nmod - nsel;
	uint32_t file_mod_end = rng->start.line + nmod;
	uint32_t file_sel_end = rng->start.line + nsel;

	// abCDEFgh_ -> abXYZgh
	if(nmod < nsel) {
		// abCDZFgh_
		string_t *sel_last = file->lines[rng->end.line];
		rawbuf_t mod_last[] = {
			mod[nmod-1],
			{&sel_last->buf[rng->end.offset], sel_last->len - rng->end.offset}
		};
		(void)string_replace_multi(
			&file->lines[file_mod_last],
			0, file->lines[file_mod_last]->len,
			mod_last, LEN(mod_last)
		);

		// abCDZ.gh_
		for(uint32_t i = file_mod_end; i < file_sel_end; i++) {
			free(file->lines[i]);
			file->lines[i] = 0;
		}

		// abCDZghh_
		memmove(
			&file->lines[file_mod_end],
			&file->lines[file_sel_end],
			(file->len - file_sel_end) * sizeof(file->lines[0])
		);
		// abCDZgh._
		for(uint32_t i = new_file_len; i < file->len; i++) {
			file->lines[i] = 0;
		}
	}

	uint32_t new_file_size = next_size(new_file_len);

	// abCDZgh_.
	for(uint32_t i = new_file_size; i < file->size; i++) {
		free(file->lines[i]);
	}

	// abCDZgh_-
	// hgFEDCba_+
	file->lines = realloc(file->lines, new_file_size * sizeof(file->lines[0]));

	// hgFEDCba_ -> hgUVWXYZba
	if(new_file_size > file->size) {
		// hgFEDCba_.
		memset(&file->lines[file->size], 0, (new_file_size - file->size) * sizeof(file->lines[0]));
	}

	if(nmod > nsel) {
		// hgFEDCbaba
		memmove(
			&file->lines[file_mod_end],
			&file->lines[file_sel_end],
			(file->len - file_sel_end) * sizeof(file->lines[0])
		);

		// hgFEDC..ba
		for(uint32_t i = file_sel_end; i < file_mod_end; i++) {
			file->lines[i] = 0;
		}

		// hgFEDC_Zba
		string_t *sel_last = file->lines[rng->end.line];
		rawbuf_t mod_last[] = {
			mod[nmod-1],
			{&sel_last->buf[rng->end.offset], sel_last->len - rng->end.offset}
		};
		(void)string_replace_multi(
			&file->lines[file_mod_last],
			0, 0,
			mod_last, LEN(mod_last)
		);
	}

	file->len = new_file_len;
	file->size = new_file_size;

	// abXDZgh_
	// hgUEDC_Zba
	file->lines[rng->start.line]->len = rng->start.offset;
	(void)string_replace_multi(
		&file->lines[rng->start.line],
		rng->start.offset, 0,
		&mod[0], 1
	);

	// abXYZgh_
	// hgUVWXYZba
	for(uint32_t i = 1 ; i < nmod - 1; i++) {
		(void)string_replace_multi(
			&file->lines[rng->start.line + i],
			0, file->lines[rng->start.line + i]->len,
			&mod[i], 1
		);
	}
}

static void
file_mod(file_t *file, range_t *rng, char *mod, size_t mod_len)
{
	size_t rest = mod_len;
	char *next;

	rawbuf_t lines[BUFSIZ / 2];
	uint32_t nlines;

	while(rest > 0) {
		nlines = 0;
		do {
			next = memchr(mod, '\n', rest);
			if(next != NULL) {
				next++;
				mod_len = next - mod;
			} else {
				mod_len = rest;
			}

			if(mod_len > UINT16_MAX) {
				// FIXME: error
			}

			lines[nlines++] = (rawbuf_t){mod, mod_len};
			mod = next;
			rest -= mod_len;
		} while(rest > 0 && nlines < LEN(lines));

		file_lines_mod(file, rng, lines, nlines);
		rng->start = rng->end;
	}
	file_lines_mod(file, rng, 0, 0);
}

TEST(file_mod) {
	file_t file = { 1, 1 };
	file.lines = calloc(file.size, sizeof(file.lines[0]));
	file.lines[0] = string_alloc(8);

	{
		char mod[] = "123\n456\n";
		file_mod(&file, &(range_t){0}, mod, LEN(mod)-1);
	}

	range_t rng = {
		{0, 1}, {1, 2}
	};
	char mod[] = "abc\ndef";
	file_mod(&file, &rng, mod, LEN(mod)-1);

	assert(is_str_eq(
		file.lines[0]->buf, file.lines[0]->len,
		"1abc\n", 5
	));
	assert(is_str_eq(
		file.lines[1]->buf, file.lines[1]->len,
		"def6\n", 5
	));

	for(uint32_t i = 0; i < file.size; i++) {
		free(file.lines[i]);
	}
	free(file.lines);
}

int
file_read(file_t *file, range_t *rng, int fd)
{
	char buf[BUFSIZ];
	ssize_t buf_len;
	while(( buf_len = read(fd, buf, sizeof(buf)) )) {
		if(buf_len < 0) {
			return -1;
		}
		file_mod(file, rng, buf, buf_len);
	}

	return 0;
}

int
main(int argc, char **argv)
{
	TEST_file_mod();
	return 0;
}
