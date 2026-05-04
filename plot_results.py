#!/usr/bin/env python3
"""
plot_results.py
Reads results/results.csv and generates performance graphs.
Requires: pip install matplotlib pandas
"""
import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

CSV_FILE = "results/results.csv"
OUT_DIR  = "results"

if not os.path.exists(CSV_FILE):
    print(f"Run run_benchmarks.py first to generate {CSV_FILE}")
    exit(1)

df = pd.read_csv(CSV_FILE)
df["CompressionRatio"] = df["CompressionRatio"].astype(float)
df["Time"]             = df["Time"].astype(float)
df["Size"]             = df["Size"].astype(int)
df["SpeedMBs"]         = df["Size"] / 1e6 / (df["Time"] / 1000)

fig, axes = plt.subplots(1, 3, figsize=(16, 5))
fig.suptitle("BZip2 Implementation — Performance Results", fontsize=14, fontweight="bold")

# 1. Compression ratio
ax = axes[0]
colors = ["#2196F3" if r < 100 else "#FF5722" for r in df["CompressionRatio"]]
bars = ax.bar(df["File"], df["CompressionRatio"], color=colors)
ax.axhline(100, color="gray", linestyle="--", linewidth=0.8, label="No compression")
ax.set_title("Compression Ratio (%)")
ax.set_ylabel("Encoded size / Original size (%)")
ax.set_xticklabels(df["File"], rotation=45, ha="right", fontsize=7)
ax.legend()
for bar, val in zip(bars, df["CompressionRatio"]):
    ax.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.5,
            f"{val:.0f}%", ha="center", va="bottom", fontsize=7)

# 2. Compression speed
ax = axes[1]
ax.bar(df["File"], df["SpeedMBs"], color="#4CAF50")
ax.set_title("Compression Speed (MB/s)")
ax.set_ylabel("MB/s")
ax.set_xticklabels(df["File"], rotation=45, ha="right", fontsize=7)

# 3. File sizes comparison
ax = axes[2]
x = range(len(df))
width = 0.35
orig_mb = df["Size"] / 1e6
comp_mb = df["Size"] * df["CompressionRatio"] / 100 / 1e6
ax.bar([i - width/2 for i in x], orig_mb, width, label="Original", color="#9C27B0")
ax.bar([i + width/2 for i in x], comp_mb, width, label="Compressed", color="#FF9800")
ax.set_title("Original vs Compressed Size (MB)")
ax.set_ylabel("Size (MB)")
ax.set_xticks(list(x))
ax.set_xticklabels(df["File"], rotation=45, ha="right", fontsize=7)
ax.legend()

plt.tight_layout()
out_path = os.path.join(OUT_DIR, "performance_graphs.png")
plt.savefig(out_path, dpi=150, bbox_inches="tight")
print(f"Graphs saved to {out_path}")
# plt.show()
