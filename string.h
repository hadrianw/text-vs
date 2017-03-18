typedef struct {
	uint16_t len;
	uint16_t size;
	char buf[];
} string_t;

string_t *string_alloc(uint16_t size);
string_t *string_realloc(string_t *str, uint16_t size);

string_t *string_resize(string_t **pstr, uint16_t sub_off, uint16_t sub_len, uint16_t new_sub_len);
string_t *string_replace(string_t **pstr, uint16_t sub_off, uint16_t sub_len, char *buf, uint16_t len);
