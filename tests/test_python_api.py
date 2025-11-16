import io

import yyaml


def test_document_from_dict_roundtrip():
    payload = {
        "name": "demo",
        "count": 3,
        "enabled": True,
        "threshold": 1.25,
        "items": ["alpha", "beta"],
        "meta": {"owner": "tester", "empty": None},
    }

    doc = yyaml.Document.from_dict(payload)
    assert doc.root is not None
    assert doc.to_dict() == payload

    rendered = yyaml.dumps(payload)
    assert "demo" in rendered

    loaded = yyaml.loads(rendered)
    assert loaded == payload


def test_dump_and_load_file_objects():
    data = {"feature": "file-api", "values": [1, 2, 3]}

    buffer = io.StringIO()
    yyaml.dump(data, buffer)
    buffer.seek(0)

    parsed = yyaml.load(buffer)
    assert parsed == data
