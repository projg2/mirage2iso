CPPFLAGS += $(shell pkg-config --cflags libmirage)
LDFLAGS += $(shell pkg-config --libs libmirage)

PROG = mirage2iso
OBJS = mirage-wrapper.o

all: $(PROG)

$(PROG): $(OBJS)

clean:
	rm -f $(PROG) $(OBJS)

.PHONY: clean
