LIBS = `pkg-config --libs libnsfb`
CC = gcc
CFLAGS += -Wall -Werror -O0 -g
BIN = paint

all: $(BIN)

%: %.c
	$(CC) -o $@ $< $(LIBS) $(CFLAGS)

clean:
	rm -f $(BIN)
