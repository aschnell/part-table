

#include <stdint.h>


void
error(const char* message);

void
error_with_errno(const char* message, int errnum);


char*
sformat(const char* format, ...) __attribute__ ((format(printf, 1, 2)));


uint32_t
chksum_crc32(const void* buf, unsigned len);


int
parse_size(const char* s, uint64_t* value);
