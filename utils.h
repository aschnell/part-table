

#include <stdint.h>


void
error(const char* message);


char*
sformat(const char* format, ...) __attribute__ ((format(printf, 1, 2)));


uint32_t
chksum_crc32(const void* buf, unsigned len);
