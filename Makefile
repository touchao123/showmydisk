CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -O2
PREFIX   ?= /usr/local

TARGET   = showmydisk
SRC      = showmydisk.c
OBJ      = $(SRC:.c=.o)

.PHONY: all clean install uninstall docker-build

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ)

docker-build:
	docker build -t showmydisk .
