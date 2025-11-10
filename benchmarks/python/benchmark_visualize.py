import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# ============================================================
# Load CSV results
# ============================================================
df = pd.read_csv("benchmark.csv")

# Convert columns to numeric and compute ops/sec
df["dump_time_ms"] = df["dump_time_ms"].astype(float)
df["load_time_ms"] = df["load_time_ms"].astype(float)
df["dump_ops_per_sec"] = 1000 / df["dump_time_ms"]
df["load_ops_per_sec"] = 1000 / df["load_time_ms"]

# Ensure consistent ordering
df["data_size"] = pd.Categorical(df["data_size"], categories=["small", "medium", "large", "xlarge"], ordered=True)

# ============================================================
# Plot 1: Dump (serialize) rates by library and data size
# ============================================================
plt.figure(figsize=(10, 6))
x = np.arange(len(df["library"].unique()))
bar_width = 0.2

for i, size in enumerate(df["data_size"].cat.categories):
    subset = df[df["data_size"] == size]
    plt.bar(x + i * bar_width, subset["dump_ops_per_sec"], width=bar_width, label=size)

plt.title("YAML Libraries - Dump (Serialize) Rate (Higher is Better)")
plt.xlabel("Library")
plt.ylabel("Operations per second")
plt.xticks(x + bar_width * 1.5, df["library"].unique(), rotation=20)
plt.legend(title="Data Size")
plt.grid(axis="y", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("yaml-benchmark-dump-rate.png", dpi=300)
plt.close()

# ============================================================
# Plot 2: Load (deserialize) rates by library and data size
# ============================================================
plt.figure(figsize=(10, 6))
for i, size in enumerate(df["data_size"].cat.categories):
    subset = df[df["data_size"] == size]
    plt.bar(x + i * bar_width, subset["load_ops_per_sec"], width=bar_width, label=size)

plt.title("YAML Libraries - Load (Deserialize) Rate (Higher is Better)")
plt.xlabel("Library")
plt.ylabel("Operations per second")
plt.xticks(x + bar_width * 1.5, df["library"].unique(), rotation=20)
plt.legend(title="Data Size")
plt.grid(axis="y", linestyle="--", alpha=0.5)
plt.tight_layout()
plt.savefig("yaml-benchmark-load-rate.png", dpi=300)
plt.close()

# ============================================================
# Plot 3: Combined comparison per data size
# ============================================================
for size in df["data_size"].cat.categories:
    subset = df[df["data_size"] == size]
    libraries = subset["library"].unique()
    bar_x = np.arange(len(libraries))
    width = 0.35

    plt.figure(figsize=(10, 6))
    plt.bar(bar_x - width/2, subset["dump_ops_per_sec"], width, label="Dump (serialize)")
    plt.bar(bar_x + width/2, subset["load_ops_per_sec"], width, label="Load (deserialize)")

    plt.title(f"YAML Benchmark Rate Comparison - {size.capitalize()} Dataset")
    plt.xlabel("Library")
    plt.ylabel("Operations per second")
    plt.xticks(bar_x, libraries, rotation=20)
    plt.legend()
    plt.grid(axis="y", linestyle="--", alpha=0.5)
    plt.tight_layout()
    plt.savefig(f"yaml-benchmark-{size}-rate-comparison.png", dpi=300)
    plt.close()

print("✅ Charts saved as:")
print(" - yaml-benchmark-dump-rate.png")
print(" - yaml-benchmark-load-rate.png")
print(" - yaml-benchmark-small-rate-comparison.png ... etc.")
