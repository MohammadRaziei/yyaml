#!/usr/bin/env python3

import re
import argparse
from pathlib import Path

HEADER = Path(__file__).parent / "yyaml/yyaml.h"


def main():
    parser = argparse.ArgumentParser(
        description="Manage YYAML version (like hatch version)"
    )
    parser.add_argument("action", nargs="?",
                        help="major | minor | patch | X.Y.Z")
    args = parser.parse_args()

    if not HEADER.exists():
        print("yyaml/yyaml.h not found")
        return

    text = HEADER.read_text()

    # Read current version
    version = {
        name: int(re.search(
            rf"#define YYAML_VERSION_{name} (\d+)", text
        ).group(1))
        for name in ("MAJOR", "MINOR", "PATCH")
    }

    if not args.action:
        print(f"{version['MAJOR']}.{version['MINOR']}.{version['PATCH']}")
        return

    if args.action == "major":
        version["MAJOR"] += 1
        version["MINOR"] = 0
        version["PATCH"] = 0

    elif args.action == "minor":
        version["MINOR"] += 1
        version["PATCH"] = 0

    elif args.action == "patch":
        version["PATCH"] += 1

    else:
        parts = args.action.split(".")
        if len(parts) != 3 or not all(p.isdigit() for p in parts):
            parser.error("Use: major | minor | patch | X.Y.Z")
        version["MAJOR"], version["MINOR"], version["PATCH"] = map(int, parts)

    # Replace all in one pass
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