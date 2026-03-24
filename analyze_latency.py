import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# Load CSV
df = pd.read_csv("latency_log.csv")
print(f"\nTotal Orders: {len(df)}") 
print("==== BASIC INFO ====")
print(df.head())

# -----------------------------
# Convert units (IMPORTANT)
# -----------------------------
df["engine_us"] = df["engine_ns"] / 1000
df["queue_us"] = df["queue_ns"] / 1000

# -----------------------------
# Percentiles
# -----------------------------
p50 = np.percentile(df["total_us"], 50)
p90 = np.percentile(df["total_us"], 90)
p99 = np.percentile(df["total_us"], 99)

print("\n==== LATENCY STATS ====")
print(f"P50: {p50:.2f} us")
print(f"P90: {p90:.2f} us")
print(f"P99: {p99:.2f} us")

# -----------------------------
# Histogram (Latency Distribution)
# -----------------------------
plt.figure()
plt.hist(df["total_us"], bins=50)
plt.title("Total Latency Distribution")
plt.xlabel("Latency (microseconds)")
plt.ylabel("Frequency")
plt.savefig("latency_distribution.png")

# -----------------------------
# Engine latency distribution
# -----------------------------
plt.figure()
plt.hist(df["engine_us"], bins=50)
plt.title("Engine Latency Distribution")
plt.xlabel("Latency (microseconds)")
plt.ylabel("Frequency")
plt.savefig("engine_latency.png")

# -----------------------------
# Throughput vs latency
# -----------------------------
# Assume each row = 1 order
# Create time buckets

df["time_bucket"] = df.index // 100  # 100 orders per bucket

throughput = df.groupby("time_bucket").size()
avg_latency = df.groupby("time_bucket")["total_us"].mean()

plt.figure()
plt.plot(throughput.values, avg_latency.values)
plt.title("Throughput vs Latency")
plt.xlabel("Orders per bucket")
plt.ylabel("Avg Latency (us)")
plt.savefig("throughput_vs_latency.png")

print("\nGraphs saved:")
print(" - latency_distribution.png")
print(" - engine_latency.png")
print(" - throughput_vs_latency.png")