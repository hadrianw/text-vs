#define SEGMENT_SIZE 8

static inline uint32_t
string_next_size(uint32_t len)
{
	return (len / SEGMENT_SIZE + 1) * SEGMENT_SIZE;
}
