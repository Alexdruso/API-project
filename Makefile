CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -std=c11
BIN     := bin/api
SRC     := src/main.c

$(BIN): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $<

PY      := python/api.py

.PHONY: run test test-py bench format format-check clean
run: $(BIN)
	./$(BIN)

# Apply clang-format in place.
format:
	clang-format -i $(SRC)

# Fail if any source file is not formatted (used in CI).
format-check:
	clang-format --dry-run --Werror $(SRC)

# Golden-snapshot tests: every <name>.txt must reproduce its committed
# <name>.expected.txt. Covers the big examples/ scenario plus the focused
# basic-case suite in tests/.
TEST_INPUTS := $(filter-out %.expected.txt,$(wildcard examples/*.txt tests/*.txt))

test: $(BIN)
	@fail=0; \
	for in in $(TEST_INPUTS); do \
	  exp="$${in%.txt}.expected.txt"; \
	  [ -f "$$exp" ] || continue; \
	  if ./$(BIN) < "$$in" | diff -u "$$exp" - >/dev/null; then \
	    echo "PASS: $$in"; \
	  else \
	    echo "FAIL: $$in"; ./$(BIN) < "$$in" | diff -u "$$exp" - || true; fail=1; \
	  fi; \
	done; \
	if [ $$fail -eq 0 ]; then echo "OK: all snapshots match"; fi; \
	exit $$fail

# Same snapshots, run against the readable Python port, to prove the two
# implementations agree on every case.
test-py:
	@fail=0; \
	for in in $(TEST_INPUTS); do \
	  exp="$${in%.txt}.expected.txt"; \
	  [ -f "$$exp" ] || continue; \
	  if python3 $(PY) < "$$in" | diff -u "$$exp" - >/dev/null; then \
	    echo "PASS: $$in"; \
	  else \
	    echo "FAIL: $$in"; python3 $(PY) < "$$in" | diff -u "$$exp" - || true; fail=1; \
	  fi; \
	done; \
	if [ $$fail -eq 0 ]; then echo "OK: Python output matches all snapshots"; fi; \
	exit $$fail

# Benchmark C vs Python (peak memory + latency) and regenerate the scatter plot.
bench: $(BIN)
	python3 -m pip install --quiet -r bench/requirements.txt
	python3 bench/benchmark.py

clean:
	rm -rf bin
