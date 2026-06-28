CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -std=c11
BIN     := bin/api
SRC     := src/main.c

$(BIN): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: run test clean
run: $(BIN)
	./$(BIN)

# Golden-snapshot test: the example input must reproduce the committed output.
test: $(BIN)
	./$(BIN) < examples/doctor_who.txt | diff -u examples/doctor_who.expected.txt -
	@echo "OK: output matches examples/doctor_who.expected.txt"

clean:
	rm -rf bin
