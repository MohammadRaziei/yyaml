#!/usr/bin/env python3

import re
import argparse
from pathlib import Path

HEADER = Path(__file__).parent / "yyaml/yyaml.h"


def main():
    parser = argparse.ArgumentParser(description="Manage YYAML version")
    parser.add_argument("part", nargs="?", help="major | minor | patch | X.Y.Z")
    parser.add_argument("value", nargs="?", help="number | +N | -N")
    args = parser.parse_args()

    if not HEADER.exists():
        print("yyaml/yyaml.h not found")
        return

    text = HEADER.read_text()

    version = {
        name: int(re.search(
            rf"#define YYAML_VERSION_{name} (\d+)", text
        ).group(1))
        for name in ("MAJOR", "MINOR", "PATCH")
    }

    # No args → print full version
    if not args.part:
        print(f"{version['MAJOR']}.{version['MINOR']}.{version['PATCH']}")
        return

    # Full version set
    if "." in args.part:
        value = args.part.lstrip("v")
        parts = value.split(".")
        if len(parts) != 3 or not all(p.isdigit() for p in parts):
            parser.error("Use: X.Y.Z")
        version["MAJOR"], version["MINOR"], version["PATCH"] = map(int, parts)

    # Single part handling
    elif args.part.lower() in ("major", "minor", "patch"):
        key = args.part.upper()

        # Only print that part
        if not args.value:
            print(version[key])
            return

        if args.value.startswith(("+", "-")):
            delta = int(args.value)
            version[key] += delta
        else:
            version[key] = int(args.value)

        if version[key] < 0:
            version[key] = 0

    else:
        parser.error("Invalid argument")

    # Replace in file
    def replacer(match):
        name = match.group(1)
        return f"#define YYAML_VERSION_{name} {version[name]}"

    text = re.sub(
        r"#define YYAML_VERSION_(MAJOR|MINOR|PATCH) \d+",
        replacer,
        text,
    )

    HEADER.write_text(text)

    print(
        f"Version updated to "
        f"{version['MAJOR']}."
        f"{version['MINOR']}."
        f"{version['PATCH']}"
    )


if __name__ == "__main__":
    main()