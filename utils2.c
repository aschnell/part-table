

#define _POSIX_C_SOURCE 200809L
#undef _GNU_SOURCE


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void
error_with_errno(const char* message, int errnum)
{
    char buf[128];

    int r = strerror_r(errnum, buf, sizeof(buf) - 1);
    if (r != 0)
	fprintf(stderr, "%s (%d)\n", message, errnum);
    else
	fprintf(stderr, "%s (%s)\n", message, buf);

    exit(EXIT_FAILURE);
}
