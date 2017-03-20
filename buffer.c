typedef struct {
	uint32_t len;
	uint32_t free;
	string_t **lines;
} file_t;

private
typedef struct {
	char *buf;
	uint16_t len;
} rawbuf_t;

typedef struct {
	uint32_t line;
	uint16_t offset;
} address_t;

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

static void
file_lines_mod(file_t *file, range_t *rng, rawbuf_t *mod, uint32_t nmod)
{
	rawbuf_t empty = {0};
	if(nmod == 0) {
		mod = &empty;
		nmod = 1;
	}

	uint32_t nsel = rng->end.line - rng->start.line + 1;
	string_t **sel_mod_last = &file->lines[rng->start.line + nmod-1];

	if(nmod == nsel) {
		uint16_t off = 0;
		if(nmod == 1) {
			off = rng->start.offset;
		}
		
 		(void)string_replace(sel_mod_last, off, rng->end.offset - off, mod[nmod-1], 1);

		if(nmod == 1) {
			return;
		}

	} else {
		string_t *sel_last = file->lines[rng->end.line];
		rawbuf_t mod_last[] = { mod[nmod-1], {&sel_last->buf[rng->end.offset], sel_last->len - rng->end.offset} };

		(*sel_mod_last)->len = 0;
		(void)string_replace(&sel_mod_last, 0, 0, mod_last, LEN(mod_last));
	}

	if(nmod < nsel) {
		memswap(&file->lines[file->len], &file->lines[rng->start.line + nmod], nsel - nmod);
		memmove(&file->lines[rng->start.line + nmod], &file->lines[rng->end.line], file->len - rng->end.line);
	}

	file = realloc(file, file->len + nmod - nsel); // zero extended or free shrinked

	if(nmod > nsel) {
		memswap(&file->lines[rng->start.line + nmod], &file->lines[file->len], nmod - nsel);
		memmove(&file->lines[rng->start.line + nmod], &file->lines[rng->end.line], file->len - rng->end.line);
	}

	file->len += nmod - nsel;

	file->lines[rng->start.line]->len = rng->start.offset;
	(void)string_replace(&file->lines[rng->start.line], rng->start.offset, 0, mod[0], 1);

	for(i = 1 ; i < nmod - 1, i++) {
		file->lines[rng->start.line + i]->len = 0;
		(void)string_replace(&file->lines[rng->start.line + i], 0, 0, mod[i], 1);
	}
}

TEST(range_mod_line) {
	file_t file = { 0 };
	file_insert_line(&file, 0, "123\n", 4);
	file_insert_line(&file, 1, "456\n", 4);
	range_t rng = {
		{0, 1}, {1, 2}, &file
	};
	{
		char mod_line[] = "abc\n";
		range_mod_line(&rng, mod_line, sizeof(mod_line)-1);
	} {
		char mod_line[] = "def";
		range_mod_line(&rng, mod_line, sizeof(mod_line)-1);
	}
	assert(is_str_eq(file.content.data[0].data, file.content.data[0].nmemb,
		"1abc\n", 5));
	assert(is_str_eq(file.content.data[1].data, file.content.data[0].nmemb,
		"def6\n", 5));
	file_free(&file);
}

static void
file_mod(file_t **file, range_t *rng, char *mod, size_t mod_len)
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
		} while(rest > 0 || nlines < LEN(lines));

		file_lines_mod(file, rng, lines, nlines);
	}
	file_mod_line(file, rng, "", 0);
}

TEST(range_mod) {
	file_t file = { 0 };
	file_insert_line(&file, 0, "123\n", 4);
	file_insert_line(&file, 1, "456\n", 4);
	range_t rng = {
		{0, 1}, {1, 2}, &file
	};
	char mod_line[] = "abc\ndef";
	range_mod(&rng, mod_line, sizeof(mod_line)-1);
	assert(is_str_eq(file.content.data[0].data, file.content.data[0].nmemb,
		"1abc\n", 5));
	assert(is_str_eq(file.content.data[1].data, file.content.data[0].nmemb,
		"def6\n", 5));
	file_free(&file);
}

int
file_read(file_t **file, range_t *rng, int fd)
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
