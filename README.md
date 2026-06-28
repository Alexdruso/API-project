# API Project — Entity & Relationship Graph Monitor

An in-memory directed graph that tracks **entities** and the typed **relationships**
between them, and reports — for each relationship type — the entity (or entities)
with the most incoming relationships of that type.

This was my project for the *Algoritmi e Principi dell'Informatica* (Algorithms and
Principles of Computer Science) course during my bachelor's degree at Politecnico di
Milano. The grade was driven by an automated evaluator that scored both **execution
time** and **memory footprint**, so the implementation is deliberately tuned for
performance rather than brevity.

**Final mark: 30/30 cum laude.**

## Overview

The program reads a stream of commands from standard input. Entities and relationships
are created and removed on the fly, and at any point a `report` command prints, per
relationship type, which entities are currently "most pointed at" and how many incoming
relationships of that type they have. The program runs until it reads `end`.

## Build

Requires a C compiler (`gcc`) and `make`.

```sh
make          # compiles src/main.c into bin/api
make clean    # removes the bin/ directory
```

## Usage

The program reads commands from `stdin` and writes results to `stdout`:

```sh
./bin/api < examples/doctor_who.txt
```

or, equivalently, build-and-run the interactive prompt with:

```sh
make run
```

## Commands

| Command | Form | Effect |
| --- | --- | --- |
| `addent` | `addent "<ent>"` | Add entity `<ent>` to the graph (no-op if it already exists). |
| `addrel` | `addrel "<orig>" "<dest>" "<rel>"` | Add a relationship of type `<rel>` from `<orig>` to `<dest>` (both entities must exist). |
| `delrel` | `delrel "<orig>" "<dest>" "<rel>"` | Remove the `<rel>` relationship from `<orig>` to `<dest>`. |
| `delent` | `delent "<ent>"` | Remove `<ent>` and every relationship in which it takes part. |
| `report` | `report` | Print, for each relationship type, the entities with the most incoming relationships. |
| `end` | `end` | Terminate the program. |

## Input / output format

- Every entity and relationship name is given as a **double-quoted** string.
- Relationships are **directed**: `addrel "A" "B" "knows"` counts as one *incoming*
  `knows` relationship for `B`.
- `report` prints one line. For each relationship type that currently has at least one
  relationship, it emits a space-separated segment:

  ```
  "<rel>" "<ent>" ["<ent>" ...] <max_count>;
  ```

  where the listed entities are exactly those tied for the highest number of incoming
  relationships of that type, sorted by name, and `<max_count>` is that maximum.
  Segments are ordered by relationship name. If no relationships exist at all, the line
  is just:

  ```
  none
  ```

### Example

```
addent "a"
addent "b"
addent "c"
addrel "a" "c" "knows"
addrel "b" "c" "knows"
addrel "a" "b" "knows"
report
end
```

`c` receives two incoming `knows` relationships and `b` one, so `report` prints:

```
"knows" "c" 2;
```

A larger, runnable command stream is provided in
[`examples/doctor_who.txt`](examples/doctor_who.txt).

## Design notes

The implementation favors incremental, performance-oriented data structures:

- **Entities** live in a hash table (`MAGIC_NUMBER` buckets), each bucket a singly
  linked list kept sorted by name.
- **Relationships** are stored per origin entity; each relationship type owns its own
  hash table of destination entities (`MAGIC_NUMBER_2` buckets), again with sorted
  buckets.
- A separate **"current maxima"** structure (`max_rel`) is maintained incrementally as
  relationships are added and removed, so that `report` mostly reads off precomputed
  results instead of scanning the whole graph on every call. A full rescan
  (`refresh_max`) is only triggered when the reigning maximum for a relationship type
  is actually torn down.

## Project structure

```
.
├── src/
│   └── main.c            # the program (single translation unit)
├── examples/
│   └── doctor_who.txt    # a sample command stream
├── Makefile
├── README.md
└── LICENSE
```

## License

See [LICENSE](LICENSE).
