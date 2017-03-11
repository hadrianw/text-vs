#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
die(const char *msg)
{
	fputs("diff2edit: ", stderr);
	perror(msg);
	exit(-1);
}

int
skip_until(FILE *file, const char *str)
{
	int idx = 0;
	int ch;
	while(str[idx] && (ch = fgetc(file)) != EOF) {
		if(ch == str[idx]) {
			idx++;
		} else {
			idx = 0;
		}
	}
	return ch;
}

typedef struct {
	uint16_t len;
	uint16_t size;
} array_head_t;

typedef struct {
	uint16_t len;
	uint16_t size;
	char data[];
} string_t;

typedef struct {
	uint16_t len;
	uint16_t size;
	uint16_t start;
	int type;
	string_t *data[];
} change_t;

typedef struct {
	uint16_t len;
	uint16_t size;
	change_t *data[];
} diff_t;

static inline size_t
size_grow(size_t size)
{
	return size * 3 / 2;
}

void
array_extend(void **array, size_t head_size, size_t elem_size, uint16_t ext)
{
	array_head_t *prev = *array;
	array_head_t *next = prev;

	uint16_t next_len = ext;
	uint16_t prev_size = 0;
	if(prev) {
		next_len += prev->len;
		prev_size = prev->size;
	}
	
	if(next_len > prev_size) {
		size_t next_size = size_grow(next_len);
		size_t prev_complete = 0;
		if(prev_size > 0) {
			prev_complete = head_size + prev_size * elem_size;
		}
		size_t next_complete = head_size + next_size * elem_size;
		next = realloc(prev, next_complete);
		// if next is null
		{
			char *bptr = (char*)next;
			memset(bptr + prev_complete, 0, next_complete - prev_complete);
		}
		next->size = next_size;
	}
	next->len = next_len;
  
	*array = next;
}

#define ARR_EXTEND(arr, ext) array_extend( (void**)(arr), sizeof( **(arr) ), sizeof( (*(arr))->data[0] ), (ext) )

int
next_change(FILE *diff_file, diff_t **diff, int type, long *linenr)
{
	char buf[BUFSIZ];
	size_t len;
	int ch = type;

	ARR_EXTEND(diff, 1);
	change_t **change = &(*diff)->data[(*diff)->len - 1];
	if(!*change) {
		ARR_EXTEND(change, 1);
	}
	(*change)->len = 0;
	(*change)->start = *linenr;
	(*change)->type = type;

	do {
		ARR_EXTEND(change, 1);
		string_t **line = &(*change)->data[(*change)->len - 1];
		if(*line) {
			(*line)->len = 0;
		}
		
		do {
			if(fgets(buf, sizeof(buf), diff_file) == 0) {
				die("fgets failed");
			}
			len = strlen(buf);
			uint16_t prev_len = 0;
			if(*line) {
				prev_len = (*line)->len;
			}
			ARR_EXTEND(line, len);
			memcpy(&(*line)->data[prev_len], buf, len);
		} while(buf[len - 1] != '\n');
		(*linenr)++;
		ch = fgetc(diff_file);
	} while(ch == type);
	
	return ch;
}

int
next_chunk(FILE *diff_file, diff_t **diff)
{
	long prev;
	long next;
	int ch;

	ch = fscanf(diff_file, "@ -%ld,%*d +%ld,%*d @@", &prev, &next);
	if(ch != 2) {
		die("bad chunk header");
	}
	ch = skip_until(diff_file, "\n");
	if(ch == EOF) {
		die("unexpected EOF in chunk header");
	}

	// 0 indexing
	if(prev> 0) { prev--; }
	if(next > 0) { next--; }

	ch = fgetc(diff_file);
	while(1) {
		if(ch == ' ') {
			prev++;
			next++;
			skip_until(diff_file, "\n");
			ch = fgetc(diff_file);
		} else if(ch == '+') {
			ch = next_change(diff_file, diff, ch, &next);
		} else if(ch == '-') {
			ch = next_change(diff_file, diff, ch, &prev);
		} else {
			break;
		}
	}
	return ch;
}

int
next_diff(FILE *diff_file, diff_t **diff)
{
	int ch;
	(*diff)->len = 0;
	
	do {
		ch = next_chunk(diff_file, diff);
	} while(ch == '@');
	return ch;
}

int
main(int argc, char *argv[])
{
	if(argc != 2) {
		fputs("usage: diff2edit FILE\n", stderr);
		return -1;
	}
	FILE *diff_file = fopen(argv[1], "rb");
	if(!diff_file) {
		die("fopen failed");
	}

	int ch;
	diff_t *diff = 0;
	ARR_EXTEND(&diff, 8);
		
	do {
		if(skip_until(diff_file, "\n@") == EOF) {
			break;
		}

		ch = next_diff(diff_file, &diff);
		
		printf("diff %u changes\n", diff->len);
		for(uint16_t i = 0; i < diff->len; i++) {
			change_t *change = diff->data[i];
			printf("%c %u lines from %u\n", change->type, change->len, change->start);
			for(uint16_t j = 0; j < change->len; j++) {
				string_t *line = change->data[j];
				printf("%u %.*s", line->len, line->len, line->data);
			}
		}
	} while(ch == '\n');
	
	for(uint16_t i = 0; i < diff->size; i++) {
		change_t *change = diff->data[i];
		if(!change) {
			continue;
		}
		for(uint16_t j = 0; j < change->size; j++) {
			string_t *line = change->data[j];
			free(line);
		}
		free(change);
	}
	free(diff);

	return 0;
}
