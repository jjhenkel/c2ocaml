SRCS = $(wildcard control-flow/*.c)

PROGS = $(patsubst %.c,%,$(SRCS))

all: $(PROGS)

%: %.c
	$(CC) -c -o $@.o $<
