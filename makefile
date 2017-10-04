LIBS = `pkg-config --libs libnsfb`
CC = gcc
CFLAGS = -Wall -Werror -O2 -g
BIN = raster

all: $(BIN)

%: %.c
	$(CC) -o $@ $< $(LIBS) $(CFLAGS)

clean:
	rm -f $(BIN)
