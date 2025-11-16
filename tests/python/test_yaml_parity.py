import datetime as dt
import io
import math
from pathlib import Path

import pytest
import yaml

import yyaml

DATA_DIR = Path(__file__).resolve().parents[1] / "data"

SAMPLE_PAYLOADS = [
    {"service": "alpha", "enabled": True, "threshold": 0.75},
    {
        "name": "nested",
        "values": [1, 2, 3],
        "meta": {"owner": "tester", "tags": ["fast", "yaml"]},
        "empty": None,
    },
    {
        "users": [
            {"id": 1, "name": "Leila", "active": True},
            {"id": 2, "name": "Omid", "active": False},
        ],
        "config": {"retries": 3, "timeout": 1.5},
    },
]


def _normalize_scalar(value):
    if isinstance(value, (dt.datetime, dt.date, dt.time)):
        return value.isoformat()
    if isinstance(value, float):
        if math.isnan(value):
            return math.nan
        return value
    return value


def normalize_structure(value):
    if isinstance(value, dict):
        return {k: normalize_structure(v) for k, v in value.items()}
    if isinstance(value, list):
        return [normalize_structure(v) for v in value]
    if isinstance(value, tuple):
        return [normalize_structure(v) for v in value]
    return _normalize_scalar(value)


def assert_yaml_equivalent(lhs, rhs):
    if isinstance(lhs, float) and isinstance(rhs, float):
        if math.isnan(lhs) and math.isnan(rhs):
            return
        assert lhs == pytest.approx(rhs)
        return
    if isinstance(lhs, list) and isinstance(rhs, list):
        assert len(lhs) == len(rhs)
        for left, right in zip(lhs, rhs):
            assert_yaml_equivalent(left, right)
        return
    if isinstance(lhs, dict) and isinstance(rhs, dict):
        assert set(lhs.keys()) == set(rhs.keys())
        for key in lhs:
            assert_yaml_equivalent(lhs[key], rhs[key])
        return
    assert lhs == rhs


@pytest.mark.parametrize(
    "data_file",
    sorted(DATA_DIR.glob("*.yaml")),
    ids=lambda path: path.name,
)
def test_loads_matches_pyyaml_for_sample_files(data_file):
    text = data_file.read_text()
    expected = normalize_structure(yaml.safe_load(text))
    actual = normalize_structure(yyaml.loads(text))
    assert_yaml_equivalent(actual, expected)


@pytest.mark.parametrize("payload", SAMPLE_PAYLOADS, ids=["simple", "nested", "users"])
def test_dumps_roundtrip_matches_original_and_pyyaml(payload):
    yaml_text = yyaml.dumps(payload)
    yyaml_loaded = normalize_structure(yyaml.loads(yaml_text))
    pyyaml_loaded = normalize_structure(yaml.safe_load(yaml_text))

    assert_yaml_equivalent(yyaml_loaded, normalize_structure(payload))
    assert_yaml_equivalent(yyaml_loaded, pyyaml_loaded)


@pytest.mark.parametrize("payload", SAMPLE_PAYLOADS, ids=["simple", "nested", "users"])
def test_parses_pyyaml_dump(payload):
    pyyaml_text = yaml.safe_dump(payload, sort_keys=False)
    parsed = normalize_structure(yyaml.loads(pyyaml_text))
    assert_yaml_equivalent(parsed, normalize_structure(payload))


def test_file_object_load_and_dump_retain_content():
    file_path = DATA_DIR / "special_cases.yaml"
    with file_path.open("r", encoding="utf-8") as handle:
        text = handle.read()

    expected = normalize_structure(yaml.safe_load(text))
    buffer = io.StringIO(text)
    parsed = normalize_structure(yyaml.load(buffer))
    assert_yaml_equivalent(parsed, expected)

    out_buffer = io.StringIO()
    yyaml.dump(expected, out_buffer, opts={"indent": 4, "final_newline": False})
    dumped_text = out_buffer.getvalue()
    assert not dumped_text.endswith("\n")

    reloaded = normalize_structure(yyaml.loads(dumped_text))
    assert_yaml_equivalent(reloaded, expected)


def test_scalar_roundtrip_matches_between_libraries():
    payloads = [
        None,
        True,
        False,
        42,
        -7,
        3.14159,
        "سلام دنیا",
        "multi\nline",
    ]

    for payload in payloads:
        serialized = yyaml.dumps(payload)
        yyaml_value = normalize_structure(yyaml.loads(serialized))
        pyyaml_value = normalize_structure(yaml.safe_load(serialized))
        assert_yaml_equivalent(yyaml_value, normalize_structure(payload))
        assert_yaml_equivalent(yyaml_value, pyyaml_value)
