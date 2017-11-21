typedef uint16_t stroff_t;

typedef struct {
	stroff_t len;
	stroff_t size;
	char buf[];
} string_t;

typedef struct {
	char *buf;
	stroff_t len;
} rawbuf_t;

string_t *string_alloc(stroff_t size);
string_t *string_realloc(string_t *str, stroff_t size);

string_t *string_resize(string_t **pstr, stroff_t sub_off, stroff_t sub_len, stroff_t new_sub_len);
string_t *string_replace_multi(string_t **pstr, stroff_t sub_off, stroff_t sub_len, rawbuf_t *mod, uint32_t nmod);
string_t *string_replace(string_t **pstr, stroff_t sub_off, stroff_t sub_len, char *buf, stroff_t len);
