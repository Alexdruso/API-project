#!/usr/bin/env python3
"""Benchmark the C and Python implementations head to head.

Both programs solve the same problem; this script measures how differently they
do it. For a single deterministic workload it runs each implementation many
times, recording end-to-end latency and peak memory (RSS) per run, then writes
the raw numbers to ``bench/results.csv`` and a memory-vs-latency scatter plot to
``bench/benchmark.png``.

Measurement notes (kept deliberately simple and fair):
  * Latency is wall-clock time around the whole subprocess (``perf_counter``),
    so it includes process/interpreter startup -- that is part of "running the
    tool", and the workload is large enough that it does not dominate.
  * Peak memory is the child's ``ru_maxrss`` from ``os.wait4`` (kilobytes on
    Linux), i.e. the real high-water mark of the process we launched.
  * Before timing anything we run both implementations once and confirm their
    output is byte-for-byte identical, so we are comparing two *correct* programs.
"""

import os
import random
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
C_BIN = REPO / "bin" / "api"
PY_IMPL = REPO / "python" / "api.py"
CSV_PATH = REPO / "bench" / "results.csv"
PNG_PATH = REPO / "bench" / "benchmark.png"

TRIALS = 30
SEED = 1234

ENGINES = {
    "C": [str(C_BIN)],
    "Python": [sys.executable, str(PY_IMPL)],
}


def generate_workload(path):
    """Write a deterministic mix of commands to ``path``.

    Short names keep us under the C implementation's 40-char name buffer. The
    mix is dominated by addrel (the hot path) with periodic reports and a few
    deletions, so both the edge bookkeeping and the report logic get exercised.
    """
    rng = random.Random(SEED)
    n_entities = 3000
    n_addrel = 120000
    n_reports = 50
    relations = ["likes", "fights", "knows", "kills"]

    report_every = n_addrel // n_reports

    with open(path, "w") as f:
        names = [f"e{i:05d}" for i in range(n_entities)]
        for name in names:
            f.write(f'addent "{name}"\n')

        for i in range(n_addrel):
            origin = rng.choice(names)
            dest = rng.choice(names)
            rel = rng.choice(relations)
            f.write(f'addrel "{origin}" "{dest}" "{rel}"\n')

            if i and i % report_every == 0:
                f.write("report\n")
                # A handful of deletions to exercise the max-recompute paths.
                f.write(f'delrel "{rng.choice(names)}" "{rng.choice(names)}" '
                        f'"{rng.choice(relations)}"\n')
                f.write(f'delent "{rng.choice(names)}"\n')

        f.write("report\n")
        f.write("end\n")


def run_once(cmd, workload):
    """Run ``cmd`` with ``workload`` on stdin; return (latency_s, peak_kb, stdout)."""
    with open(workload, "rb") as stdin:
        start = time.perf_counter()
        proc = subprocess.Popen(cmd, stdin=stdin, stdout=subprocess.PIPE)
        out = proc.stdout.read()
        proc.stdout.close()
        _, status, rusage = os.wait4(proc.pid, 0)
        latency = time.perf_counter() - start
    if status != 0:
        raise RuntimeError(f"{cmd!r} exited with status {status}")
    return latency, rusage.ru_maxrss, out


def main():
    if not C_BIN.exists():
        sys.exit(f"C binary not found at {C_BIN}; run `make` first.")

    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as tmp:
        workload = tmp.name
    try:
        print(f"Generating workload -> {workload}")
        generate_workload(workload)
        print(f"Workload size: {os.path.getsize(workload) / 1024:.0f} KiB")

        # Parity gate: both implementations must agree before we trust timings.
        print("Checking output parity ...")
        _, _, c_out = run_once(ENGINES["C"], workload)
        _, _, py_out = run_once(ENGINES["Python"], workload)
        if c_out != py_out:
            sys.exit("PARITY FAILURE: C and Python produced different output.")
        print("Parity OK: identical output.\n")

        results = {name: {"latency_ms": [], "peak_mb": []} for name in ENGINES}
        for name, cmd in ENGINES.items():
            for trial in range(TRIALS):
                latency, peak_kb, _ = run_once(cmd, workload)
                results[name]["latency_ms"].append(latency * 1000.0)
                results[name]["peak_mb"].append(peak_kb / 1024.0)
            lat = results[name]["latency_ms"]
            mem = results[name]["peak_mb"]
            print(f"{name:7s}  latency median {statistics.median(lat):8.1f} ms   "
                  f"peak mem median {statistics.median(mem):7.1f} MB   ({TRIALS} runs)")
    finally:
        os.unlink(workload)

    # Raw data.
    with open(CSV_PATH, "w") as f:
        f.write("engine,trial,latency_ms,peak_mb\n")
        for name in ENGINES:
            for i in range(TRIALS):
                f.write(f"{name},{i},{results[name]['latency_ms'][i]:.3f},"
                        f"{results[name]['peak_mb'][i]:.3f}\n")
    print(f"\nWrote {CSV_PATH.relative_to(REPO)}")

    # Scatter plot.
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    fig, ax = plt.subplots(figsize=(8, 6))
    colors = {"C": "#1f77b4", "Python": "#d62728"}
    for name in ENGINES:
        ax.scatter(results[name]["latency_ms"], results[name]["peak_mb"],
                   label=name, color=colors[name], alpha=0.75, edgecolors="white",
                   s=70)
    ax.set_xlabel("Latency (ms)  —  lower is better")
    ax.set_ylabel("Peak memory (MB)  —  lower is better")
    ax.set_title(f"C vs Python: latency and peak memory ({TRIALS} runs each)")
    ax.legend(title="implementation")
    ax.grid(True, linestyle=":", alpha=0.5)
    fig.tight_layout()
    fig.savefig(PNG_PATH, dpi=120)
    print(f"Wrote {PNG_PATH.relative_to(REPO)}")


if __name__ == "__main__":
    main()
