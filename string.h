typedef struct {
	uint16_t len;
	uint16_t free;
	char buf[];
} string_t;

string_t *string_alloc(uint16_t size);
string_t *string_realloc(string_t *str, uint16_t size);
string_t *string_resize(string_t *str, uint16_t off, uint16_t off_len, uint16_t len);
int string_replace(string_t **pstr, uint16_t off, uint16_t off_len, char *buf, uint16_t len);
