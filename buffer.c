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
lines_replace(file_t *file, uint32_t off, uint32_t off_len, rawbuf_t *lines, uint32_t nlines)
{
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
	string_t **sel_last = &file->lines[rng->end.line];

	if(nmod == nsel) {
		uint16_t off = 0;
		if(nmod == 1) {
			off = rng->start.offset;
		}

 		(void)string_replace(sel_last, off, rng->end.offset - off, mod[nmod-1].buf, mod[nmod-1].len);

		if(nmod == 1) {
			return;
		}

	} else if nmod < nsel {
		realloc(SEL.(MOD.last.nr), SEL.tail.len + MOD.last.len);
		memcpy(SEL.(MOD.last.nr) + MOD.last.len, SEL.tail, SEL.tail.len);
		memcpy(SEL.last, MOD.last, MOD.last.len);
		for(i = nmod; i < nsel; i++) {
			free(SEL.i);
		}

		// shift rest of lines up
		memmove(FILE + SEL.first.nr + nmod, FILE + SEL.first.nr + nsel, nFILE - (SEL.first.nr + nsel));
	}

	// resize FILE arrray
	realloc(FILE, nFILE + nmod - nsel);

	string_t *new_sel_last = 0;
	if(nmod > nsel) {
		// copy over the tail so it will not be overwritten
		(void)string_resize(&new_sel_last, 0, 0, mod[nmod-1].len + (*sel_last)->len - rng->end.offset);
		memcpy(new_sel_last->buf, mod[nmod-1].buf, mod[nmod-1].len);
		memcpy(&new_sel_last->buf[mod[nmod-1].len], &(*sel_last)->buf[rng->end.offset], (*sel_last)->len - rng->end.offset);

		// shift rest of lines down
		memmove(FILE + SEL.first.nr + nmod, FILE + SEL.last.nr, nFILE - SEL.last.nr);
	}

	nFILE += nmod - nsel;

	// (re)alloc and copy
	file->lines[rng->start.line]->len = rng->start.offset;
	string_replace(&file->lines[rng->start.line], rng->start.offset, 0, mod[0].buf, mod[0].len);

	for(i = 1 ; i < nmod - 1, i++) {
		file->lines[rng->start.line + i]->len = 0;
		string_replace(&file->lines[rng->start.line + i], 0, 0, mod[i].buf, mod[i].len);
	}
	if(nmod > nsel) {
		SEL.last = tmp.SEL.last;
	}





	rawbuf_t empty = {0};
	if(!nlines) {
		lines = &empty;
	}

	uint32_t rng_nlines = rng->end.line - rng->start.line;

	string_t **rng_first_line = &file->lines[rng->start.line];
	string_t *rng_last_line = file->lines[rng->end.line];
	rawbuf_t *mod_first_line = &lines[0];

	uint16_t off_len;
	if(nlines > 1) {

	}
	if(rng->start.line == rng->end.line) {
		off_len = rng->end.offset - rng->start.offset;
	} else {
		off_len = (*rng_first_line)->len - rng->start.offset;
	}

	string_replace(rng_first_line, rng->start.offset, off_len, mod_first_line->buf, mod_first_line->len);

	if(nlines <= 1 && rng_nlines > 1) {
		string_replace(rng_first_line, rng->start.offset + off_len, 0, rng_last_line->buf, rng_last_line->len);
		// FIXME: remove middle lines
		return;
	}







	size_t rest_len;
	bool insert_new_line;

	rest_len = line->len - end_offset;
	insert_new_line = (rest_len > 0 || rng->start.line == rng->end.line) &&
			mod_len > 0 && mod_line[mod_len - 1] == '\n';

	if(insert_new_line) {
		char *rest = &line->buf[end_offset];
		file_insert_line(file, rng->end.line+1, rest, rest_len);
		// lines could be realloced
		line = rng->file->content.data[rng->start.line];
		line->len -= rest_len;
	}

	line = string_insert(line, rng->start.offset, end_offset - rng->start.offset, mod_line, mod_len);
	rng->file->content.data[rng->start.line] = line;

	if(mod_len > 0 && mod_line[mod_len - 1] == '\n') {
		rng->start.line++;
		rng->start.offset = 0;
		range_fix_end(rng);
		return;
	}

	rng->start.offset += mod_len;

	bool join_end_line = rest_len == 0 && rng->start.line+1 < rng->file->content.nmemb;
	if(join_end_line) {
		if(rng->start.line == rng->end.line) {
			rng->end.line++;
			rng->end.offset = 0;
		}
		string_t *rest_line = rng->file->content.data[rng->end.line];
		char *rest = &rest_line->buf[rng->end.offset];
		rest_len = rest_line->len - rng->end.offset;

		rest_line = string_insert(rest_line, rng->start.offset, line->len, rest, rest_len);
		rng->file->content.data[rng->end.line] = rest_line;

		for(size_t i = rng->start.line+1; i < rng->end.line+1; i++) {
			free(rng->file->content.data[i]);
		}
		ARR_FRAG_RESIZE(&rng->file->content, rng->start.line+1, rng->end.line+1, 0);
	}

	rng->end.line = rng->start.line;
	rng->end.offset = rng->start.offset;
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
