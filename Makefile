CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -std=c11
BIN     := bin/api
SRC     := src/main.c

$(BIN): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: run test format format-check clean
run: $(BIN)
	./$(BIN)

# Apply clang-format in place.
format:
	clang-format -i $(SRC)

# Fail if any source file is not formatted (used in CI).
format-check:
	clang-format --dry-run --Werror $(SRC)

# Golden-snapshot test: the example input must reproduce the committed output.
test: $(BIN)
	./$(BIN) < examples/doctor_who.txt | diff -u examples/doctor_who.expected.txt -
	@echo "OK: output matches examples/doctor_who.expected.txt"

clean:
	rm -rf bin
