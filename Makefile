

TARGET = part-table

SRCS = create-partition.c create-partition-table.c	\
	disk.c gpt.c linux.c mbr.c modify-partition.c	\
	part-table.c print.c remove-partition.c 	\
	utils1.c utils2.c

INCPATH =
LIBPATH =

CPPFLAGS = $(INCPATH)

CFLAGS = -Wall -Wextra -Wformat -O2

LDFLAGS = $(LIBPATH) -luuid -lblkid -ldevmapper -ljson-c

OBJS = $(SRCS:%.c=%.o)


$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)


depend::
	$(CC) -MM $(CPPFLAGS) $(SRCS) > depend

ifeq (depend,$(wildcard depend))
include depend
endif


clean::
	rm -f $(OBJS) depend core

