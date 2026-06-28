CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -std=c11
BIN     := bin/api
SRC     := src/main.c

$(BIN): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: run clean
run: $(BIN)
	./$(BIN)

clean:
	rm -rf bin
