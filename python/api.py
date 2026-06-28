#!/usr/bin/env python3
"""A small, readable (deliberately non-optimized) port of the C graph tool.

This mirrors the behavior of ``src/main.c``: it reads commands from standard
input and tracks a directed graph of entities and named relations, answering
``report`` with the entities that have the most incoming relations of each type.

The goal here is clarity, not speed. Where the C version maintains the running
maxima incrementally with hand-rolled hash tables, this version keeps a plain
dictionary of sets and simply recomputes the maxima whenever ``report`` is
called. The output format is identical, so both implementations pass the same
golden-snapshot tests in ``examples/`` and ``tests/``.

Commands (names are always double-quoted in the input):
    addent "ent"                    add an entity
    delent "ent"                    remove an entity and every relation touching it
    addrel "orig" "dest" "rel"      add an incoming "rel" edge orig -> dest
    delrel "orig" "dest" "rel"      remove that edge
    report                          print the leaders per relation type
    end                             stop
"""

import re
import sys

# Pulls the double-quoted arguments out of a command line, without the quotes.
QUOTED = re.compile(r'"([^"]*)"')


def report(relations):
    """Return the report line for the current graph state.

    For each relation type (sorted by name) we count how many origins point at
    each destination, find the highest count, and list the destinations tied at
    that count in sorted order. Relation types with no incoming edges are
    skipped. When nothing qualifies, the answer is the single word ``none``.
    """
    segments = []

    for rel_name in sorted(relations):
        incoming = relations[rel_name]

        # destination -> number of distinct origins pointing at it
        counts = {dest: len(origins) for dest, origins in incoming.items() if origins}
        if not counts:
            continue

        highest = max(counts.values())
        leaders = sorted(dest for dest, count in counts.items() if count == highest)

        names = " ".join(f'"{name}"' for name in [rel_name, *leaders])
        segments.append(f"{names} {highest};")

    return " ".join(segments) if segments else "none"


def main():
    entities = set()
    # relations[rel_name][destination] = set of origins (the incoming edges)
    relations = {}
    output = []

    for line in sys.stdin:
        command = line.split(maxsplit=1)[0] if line.strip() else ""
        args = QUOTED.findall(line)

        if command == "addent":
            (name,) = args
            entities.add(name)

        elif command == "delent":
            (name,) = args
            entities.discard(name)
            # Drop the entity wherever it appears, as a destination or an origin.
            for incoming in relations.values():
                incoming.pop(name, None)
                for origins in incoming.values():
                    origins.discard(name)

        elif command == "addrel":
            origin, dest, rel_name = args
            if origin in entities and dest in entities:
                relations.setdefault(rel_name, {}).setdefault(dest, set()).add(origin)

        elif command == "delrel":
            origin, dest, rel_name = args
            incoming = relations.get(rel_name)
            if incoming and dest in incoming:
                incoming[dest].discard(origin)

        elif command == "report":
            output.append(report(relations))

        elif command == "end":
            break

    sys.stdout.write("\n".join(output))
    if output:
        sys.stdout.write("\n")


if __name__ == "__main__":
    main()
