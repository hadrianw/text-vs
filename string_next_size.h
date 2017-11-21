#define SEGMENT_SIZE 8

static inline stroff_t
string_next_size(stroff_t len)
{
	return (len / SEGMENT_SIZE + 1) * SEGMENT_SIZE;
}
