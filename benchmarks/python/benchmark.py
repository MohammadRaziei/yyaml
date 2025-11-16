import time
import csv
import random
import string
from io import StringIO
from tqdm import tqdm

import yyaml              # OURS
import yaml               # PyYAML
import ruamel.yaml        # ruamel.yaml
import ruyaml             # ruyaml
import strictyaml         # StrictYAML
import yamlium            # Yamlium
import oyaml              # oyaml (Ordered YAML)
import ryaml              # ryaml (Rust-based YAML)

# ============================================================
# Benchmark configuration
# ============================================================
MAX_ITER = 200         # Maximum number of iterations
MAX_TIME = 6 * 60.0       # Maximum time per operation (seconds)
SIZES = {
    "small": 10,
    "medium": 100,
    "large": 1000,
    "xlarge": 5000,
}

# ============================================================
# Helper functions
# ============================================================

def random_dict(n: int):
    """Generate a random dictionary with nested structures."""
    return {
        f"key_{i}": {
            "name": "".join(random.choices(string.ascii_letters, k=8)),
            "value": random.random(),
            "flag": random.choice([True, False]),
            "nested": {"x": random.randint(0, 1000), "y": random.random()},
        }
        for i in range(n)
    }

def yaml_text_from_dict(d: dict):
    """Convert a dict to YAML text for load tests (using PyYAML)."""
    return yaml.safe_dump(d, sort_keys=False)

def limited_benchmark(operation_name, func, data):
    """
    Run a function up to MAX_ITER times but stop if it exceeds MAX_TIME seconds.
    Returns the average time per iteration (seconds).
    """
    start = time.perf_counter()
    times = []
    for i in tqdm(range(MAX_ITER), desc=operation_name, leave=True, ncols=80):
        iter_start = time.perf_counter()
        func(data)
        iter_end = time.perf_counter()
        times.append(iter_end - iter_start)
        # Stop if total time exceeds the limit
        if iter_end - start > MAX_TIME:
            print(f"\n⏱️  {operation_name} exceeded {MAX_TIME} s limit, stopping early at {i+1} iterations.")
            break
    return sum(times) / len(times) if times else float("inf")

def benchmark(library_name, dump_func, load_func, sample_dict, sample_yaml):
    """Run a benchmark for a given YAML library and return average times (ms)."""
    dump_time = limited_benchmark(f"{library_name} dump", dump_func, sample_dict)
    load_time = limited_benchmark(f"{library_name} load", load_func, sample_yaml)
    return dump_time * 1000, load_time * 1000


# ============================================================
# Initialize YAML library objects
# ============================================================

yaml_ruamel = ruamel.yaml.YAML(typ="safe")
yaml_ruyaml = ruyaml.YAML(typ="safe")

# ============================================================
# Run benchmarks
# ============================================================

results = []

for size_name, num_items in SIZES.items():
    print(f"\n=== Running benchmarks for data size: {size_name} ({num_items} items) ===")
    sample_dict = random_dict(num_items)
    sample_yaml = yaml_text_from_dict(sample_dict)

    results.append(("yyaml", size_name, *benchmark(
        "yyaml",
        lambda d: yyaml.dumps(d),
        lambda s: yyaml.loads(s),
        sample_dict, sample_yaml
    )))

    # PyYAML
    results.append(("PyYAML", size_name, *benchmark(
        "PyYAML",
        lambda d: yaml.safe_dump(d, sort_keys=False),
        lambda s: yaml.safe_load(s),
        sample_dict, sample_yaml
    )))

    # ruamel.yaml
    results.append(("ruamel.yaml", size_name, *benchmark(
        "ruamel.yaml",
        lambda d: (lambda buf: (yaml_ruamel.dump(d, buf), buf.getvalue()))(StringIO()),
        lambda s: yaml_ruamel.load(s),
        sample_dict, sample_yaml
    )))

    # ruyaml
    results.append(("ruyaml", size_name, *benchmark(
        "ruyaml",
        lambda d: (lambda buf: (yaml_ruyaml.dump(d, buf), buf.getvalue()))(StringIO()),
        lambda s: yaml_ruyaml.load(s),
        sample_dict, sample_yaml
    )))

    # StrictYAML
    results.append(("StrictYAML", size_name, *benchmark(
        "StrictYAML",
        lambda d: strictyaml.as_document(d).as_yaml(),
        lambda s: strictyaml.load(s).data,
        sample_dict, sample_yaml
    )))

    # Yamlium
    results.append(("Yamlium", size_name, *benchmark(
        "Yamlium",
        lambda d: yamlium.from_dict(d).to_yaml(),
        lambda s: yamlium.parse(s).to_dict(),
        sample_dict, sample_yaml
    )))

    # oyaml
    results.append(("oyaml", size_name, *benchmark(
        "oyaml",
        lambda d: oyaml.safe_dump(d, sort_keys=False),
        lambda s: oyaml.safe_load(s),
        sample_dict, sample_yaml
    )))

    # ryaml
    results.append(("ryaml", size_name, *benchmark(
        "ryaml",
        lambda d: ryaml.dumps(d),
        lambda s: ryaml.loads(s),
        sample_dict, sample_yaml
    )))

# ============================================================
# Save results to CSV
# ============================================================

with open("benchmark.csv", "w", newline="", encoding="utf-8") as f:
    writer = csv.writer(f)
    writer.writerow(["library", "data_size", "dump_time_ms", "load_time_ms"])
    writer.writerows(results)

print("\n✅ Benchmark completed successfully. Results saved to 'benchmark.csv'")
