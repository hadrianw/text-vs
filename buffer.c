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

	if(nmod == 1 && nsel == 1) {
		(void)string_replace_multi(
			&file->lines[rng->start.line],
			rng->start.offset, rng->end.offset - rng->start.offset,
			&mod[0], 1
		);
		return;
	}

	// nmod == nsel: ijKLMNop -> ijXYZWop // X = head(C) mod[0]; W = mod[-1] tail(N)
	// nmod < nsel: abCDEFgh_ -> abXYZgh // X = head(C) mod[0]; Z = mod[-1] tail(F)
	// nmod < nsel && nmod == 1: abCDEfg_ -> abXfg__ // X = head(C) mod[0] tail(E)
	// nmod > nsel: hgFEDCba_ -> hgUVWXYZba // U = head(F) mod[0]; Z = mod[-1] tail(C)
	// nmod > nsel && nsel == 1: edCba_ -> edUVWba // U = head(C) mod[0]; W = mod[-1] tail(C)

	if(nmod <= nsel){
		// ij[X]LMNop
		// ab[X]DEFgh_
		file->lines[rng->start.line]->len = rng->start.offset;
		(void)string_replace_multi(
			&file->lines[rng->start.line],
			rng->start.offset, 0,
			&mod[0], 1
		);
	}

	uint32_t file_mod_last =  rng->start.line + nmod-1;

	if(nmod == nsel) {
		// ijXLM[W]op
 		(void)string_replace_multi(
 			&file->lines[file_mod_last],
			0, rng->end.offset,
 			&mod[nmod-1], 1
 		);
	}
	
	uint32_t new_file_len = file->len + nmod - nsel;
	uint32_t file_mod_end = rng->start.line + nmod;
	uint32_t file_sel_end = rng->start.line + nsel;

	if(nmod < nsel) {
		// abXD[Z]Fgh_
		// ab[X]DEfg_
		uint16_t off = 0;
		if(nmod == 1) {
			off = rng->start.offset;
		}
		string_t *sel_last = file->lines[rng->end.line];
		rawbuf_t mod_last[] = {
			mod[nmod-1],
			{&sel_last->buf[rng->end.offset], sel_last->len - rng->end.offset}
		};
		(void)string_replace_multi(
			&file->lines[file_mod_last],
			off, file->lines[file_mod_last]->len - off,
			mod_last, LEN(mod_last)
		);

		// abXDZ[.]gh_
		// abX[..]fg_
		for(uint32_t i = file_mod_end; i < file_sel_end; i++) {
			free(file->lines[i]);
			file->lines[i] = 0;
		}

		// abXDZ[gh]h_
		// abX[fg]fg_
		memmove(
			&file->lines[file_mod_end],
			&file->lines[file_sel_end],
			(file->len - file_sel_end) * sizeof(file->lines[0])
		);
		// abXDZgh[.]_
		// abXfg[..]_
		for(uint32_t i = new_file_len; i < file->len; i++) {
			file->lines[i] = 0;
		}
	}

	uint32_t new_file_size = next_size(new_file_len);

	// abXDZgh_[.]
	// abXfg__[.]
	for(uint32_t i = new_file_size; i < file->size; i++) {
		free(file->lines[i]);
	}

	// abXDZgh_[-]
	// abXfg__[-]
	// hgFEDCba_[+]
	// edCba_[+]
	file->lines = realloc(file->lines, new_file_size * sizeof(file->lines[0]));

	if(new_file_size > file->size) {
		// hgFEDCba_[.]
		// edCba_[.]
		memset(&file->lines[file->size], 0, (new_file_size - file->size) * sizeof(file->lines[0]));
	}

	if(nmod > nsel) {
		// hgFEDCba[ba]
		// edCba[ba]
		memmove(
			&file->lines[file_mod_end],
			&file->lines[file_sel_end],
			(file->len - file_sel_end) * sizeof(file->lines[0])
		);

		// hgFEDC[..]ba
		// edC[..]ba
		for(uint32_t i = file_sel_end; i < file_mod_end; i++) {
			file->lines[i] = 0;
		}

		// hgFEDC_[Z]ba
		// edC_[W]ba
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

		// hgUEDC_[Z]ba
		// edU_[W]ba
		file->lines[rng->start.line]->len = rng->start.offset;
		(void)string_replace_multi(
			&file->lines[rng->start.line],
			rng->start.offset, 0,
			&mod[0], 1
		);
	}

	file->len = new_file_len;
	file->size = new_file_size;

	// abX[Y]Zgh_
	// hgU[VWXY]Zba
	// edU[V]Wba
	for(uint32_t i = 1 ; i < nmod - 1; i++) {
		if(file->lines[rng->start.line + i] != NULL) {
			file->lines[rng->start.line + i]->len = 0;
		}
		(void)string_replace_multi(
			&file->lines[rng->start.line + i],
			0, 0,
			&mod[i], 1
		);
	}
}

static void
file_mod(file_t *file, range_t *rng, char *mod, size_t mod_len)
{
	int newline_last = mod[mod_len-1] == '\n';
	size_t rest = mod_len;
	char *next;

	rawbuf_t lines[BUFSIZ / 2 + 1];
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
		} while(rest > 0 && nlines < LEN(lines)-1);

		if(rest == 0 && newline_last) {
			lines[nlines++] = (rawbuf_t){0};
		}

		file_lines_mod(file, rng, lines, nlines);
		rng->start = rng->end;
	}
	//  FIXME:
	//file_lines_mod(file, rng, 0, 0);
}

#define STR_PAIR(x) x, (sizeof(x)-1)

TEST(file_mod) {
	struct mod {
		range_t rng;
		char *mod;
		size_t mod_len;
		char *result;
		size_t result_len;
	};
	struct mod mods[] = {
		{
			{{0}},
			STR_PAIR(
				"123\n"
				"456\n"
				""
			),
			STR_PAIR(
				"123\n"
				"456\n"
				""
			),
		}, {
			{{0, 1}, {1, 2}},
			STR_PAIR(
				"abc\n"
				"def"
			),
			STR_PAIR(
				"1abc\n"
				"def6\n"
				""
			),
		},  {
			{{1, 2}, {1, 3}},
			STR_PAIR(
				"FFFF"
			),
			STR_PAIR(
				"1abc\n"
				"deFFFF6\n"
				""
			),
		},  {
			{{2, 0}, {2, 0}},
			STR_PAIR(
				"xyz"
			),
			STR_PAIR(
				"1abc\n"
				"deFFFF6\n"
				"xyz"
			),
		}, {
			{{1, 6}, {2, 1}},
			STR_PAIR(
				"qq"
			),
			STR_PAIR(
				"1abc\n"
				"deFFFFqqyz"
			),
		}, {
			{{1, 9}, {1, 10}},
			STR_PAIR(
				"\n"
				"I like milk\n"
				"and other liquids\n"
				"like water for example\n"
			),
			STR_PAIR(
				"1abc\n"
				"deFFFFqqy\n"
				"I like milk\n"
				"and other liquids\n"
				"like water for example\n"
			),
		},
	};

	file_t file = { 1, 1 };
	file.lines = calloc(file.size, sizeof(file.lines[0]));
	file.lines[0] = string_alloc(8);

	for(size_t i = 0; i < LEN(mods); i++) {
		file_mod(&file, &mods[i].rng, mods[i].mod, mods[i].mod_len);

		char *line = mods[i].result;
		char *next;
		size_t rest = mods[i].result_len;
		size_t len;
		for(size_t j = 0; j < file.len; j++) {
				next = memchr(line, '\n', rest);
				if(next) {
					next++;
					len = next - line;
				} else {
					len = rest;
				}
				assert(is_str_eq(
					file.lines[j]->buf, file.lines[j]->len,
					line, len
				));
				line = next;
				rest -= len;
		}
		assert(rest == 0);
	}

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
