import pandas as pd
import matplotlib.pyplot as plt

# ============================================================
# Load CSV results
# ============================================================
df = pd.read_csv("benchmark.csv")

# Convert columns if necessary
df["dump_time_ms"] = df["dump_time_ms"].astype(float)
df["load_time_ms"] = df["load_time_ms"].astype(float)

# ============================================================
# Plot 1: Dump (serialize) times by library and data size
# ============================================================
plt.figure(figsize=(10, 6))
for size in df["data_size"].unique():
    subset = df[df["data_size"] == size]
    plt.plot(subset["library"], subset["dump_time_ms"], marker="o", label=size)

plt.title("YAML Libraries - Dump (Serialize) Performance")
plt.xlabel("Library")
plt.ylabel("Time per operation (ms)")
plt.legend(title="Data Size")
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("yaml-benchmark-dump-times.png", dpi=300)
plt.close()

# ============================================================
# Plot 2: Load (deserialize) times by library and data size
# ============================================================
plt.figure(figsize=(10, 6))
for size in df["data_size"].unique():
    subset = df[df["data_size"] == size]
    plt.plot(subset["library"], subset["load_time_ms"], marker="o", label=size)

plt.title("YAML Libraries - Load (Deserialize) Performance")
plt.xlabel("Library")
plt.ylabel("Time per operation (ms)")
plt.legend(title="Data Size")
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("yaml-benchmark-load-times.png", dpi=300)
plt.close()

# ============================================================
# Plot 3: Combined bar chart for overall comparison
# =====================================================
