import time
import csv
import random
import string
from io import StringIO

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
N = 200  # Number of iterations per test
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

def benchmark(library_name, dump_func, load_func, sample_dict, sample_yaml):
    """Run a benchmark for a given YAML library and return average times."""
    # Serialize benchmark
    start = time.perf_counter()
    for _ in range(N):
        dump_func(sample_dict)
    dump_time = (time.perf_counter() - start) / N

    # Deserialize benchmark
    start = time.perf_counter()
    for _ in range(N):
        load_func(sample_yaml)
    load_time = (time.perf_counter() - start) / N

    return dump_time * 1000, load_time * 1000  # convert to ms


# ============================================================
# Initialize YAML library objects
# ============================================================

yaml_ruamel = ruamel.yaml.YAML(typ="safe")
yaml_ruyaml = ruyaml.YAML(typ="safe")
from yamlium import parse, from_dict

# ============================================================
# Run benchmarks
# ============================================================

results = []

for size_name, num_items in SIZES.items():
    print(f"Running benchmarks for data size: {size_name} ({num_items} items)...")
    sample_dict = random_dict(num_items)
    sample_yaml = yaml_text_from_dict(sample_dict)

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
        lambda d: from_dict(d).to_yaml(),
        lambda s: parse(s).to_dict(),
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
