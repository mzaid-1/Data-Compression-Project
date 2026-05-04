#!/usr/bin/env python3
"""
run_benchmarks.py
Runs the bzip2_impl binary on all benchmark files, verifies correctness,
and writes results.csv + prints a summary table.
"""
import subprocess, os, time, csv, sys

EXE      = "./bzip2_impl"
BENCH    = "benchmarks"
RESULTS  = "results"
CSV_OUT  = os.path.join(RESULTS, "results.csv")

os.makedirs(RESULTS, exist_ok=True)

files = [f for f in os.listdir(BENCH) if os.path.isfile(os.path.join(BENCH, f))]
files.sort()

rows = []
print(f"\n{'File':<30} {'Size':>10} {'CompSize':>10} {'Ratio':>8} {'Time(ms)':>10} {'OK':>4}")
print("-" * 75)

# Parse arguments
use_fast = "--fast" in sys.argv
cfg_arg = ["config_fast.ini"] if use_fast else []

for fname in files:
    infile   = os.path.join(BENCH, fname)
    cmpfile  = os.path.join(RESULTS, fname + ".bz2i")
    recfile  = os.path.join(RESULTS, fname + ".recovered")

    orig_size = os.path.getsize(infile)

    t0 = time.time()
    # Pass config if --fast was specified
    cmd = [EXE, "compress", infile, cmpfile] + cfg_arg
    r1 = subprocess.run(cmd, capture_output=True, text=True)
    t1 = time.time()
    elapsed_ms = (t1 - t0) * 1000

    if r1.returncode != 0:
        print(f"{fname:<30} {'ERROR':>10}")
        continue

    comp_size = os.path.getsize(cmpfile)
    ratio     = comp_size / orig_size * 100 if orig_size > 0 else 0

    # Decompress and verify
    cmd2 = [EXE, "decompress", cmpfile, recfile] + cfg_arg
    r2 = subprocess.run(cmd2, capture_output=True, text=True)

    ok = False
    if r2.returncode == 0:
        with open(infile, "rb") as f1, open(recfile, "rb") as f2:
            ok = f1.read() == f2.read()

    rows.append({
        "File": fname, "Size": orig_size, "BlockSize": 500000,
        "CompressionRatio": f"{ratio:.2f}", "Time": f"{elapsed_ms:.1f}", "Memory": "N/A"
    })

    flag = "OK" if ok else "FAIL"
    print(f"{fname:<30} {orig_size:>10,} {comp_size:>10,} {ratio:>7.1f}% {elapsed_ms:>10.1f} {flag:>4}")

print("-" * 75)

# Write CSV
with open(CSV_OUT, "w", newline="") as f:
    writer = csv.DictWriter(f, fieldnames=["File","Size","BlockSize","CompressionRatio","Time","Memory"])
    writer.writeheader()
    writer.writerows(rows)

print(f"\nResults saved to {CSV_OUT}")
