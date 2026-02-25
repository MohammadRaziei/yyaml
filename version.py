#!/usr/bin/env python3
"""
YYAML Version & Tag Manager

Manage semantic version macros inside:
    yyaml/yyaml.h

Supports:
  - Showing current header version
  - Reading latest git tag
  - Setting version from tag
  - Creating releases (commit + tag)
  - Semantic version bumping

Repository:
    https://github.com/mohammadraziei/yyaml

Author:
    Mohammad Raziei

License:
    MIT
    SPDX-License-Identifier: MIT

Run:
    python version.py --help
"""

import re
import sys
import argparse
import subprocess
from pathlib import Path


HEADER = Path(__file__).parent / "yyaml/yyaml.h"
VERSION_PATTERN = r"#define YYAML_VERSION_(MAJOR|MINOR|PATCH) (\d+)"


# ============================================================
# Utilities
# ============================================================

def run(cmd):
    subprocess.run(cmd, check=True)


def git_last_tag():
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0"],
            capture_output=True,
            text=True,
            check=True,
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None


def read_version():
    if not HEADER.exists():
        sys.exit("yyaml/yyaml.h not found")

    text = HEADER.read_text()
    matches = re.findall(VERSION_PATTERN, text)
    version = {name: int(value) for name, value in matches}
    return version, text


def write_version(version, text):
    def repl(match):
        name = match.group(1)
        return f"#define YYAML_VERSION_{name} {version[name]}"

    new_text = re.sub(VERSION_PATTERN, repl, text)
    HEADER.write_text(new_text)


def version_str(v):
    return f"{v['MAJOR']}.{v['MINOR']}.{v['PATCH']}"


def bump(version, part):
    if part == "major":
        version["MAJOR"] += 1
        version["MINOR"] = 0
        version["PATCH"] = 0
    elif part == "minor":
        version["MINOR"] += 1
        version["PATCH"] = 0
    elif part == "patch":
        version["PATCH"] += 1


# ============================================================
# Commands
# ============================================================

def cmd_show(_):
    version, _ = read_version()
    print(version_str(version))


def cmd_tag(args):

    # --------------------------------------------------------
    # python version.py tag
    # --------------------------------------------------------
    if args.subcommand is None:
        tag = git_last_tag()
        print(tag if tag else "No git tags found")
        return

    # --------------------------------------------------------
    # python version.py tag set
    # --------------------------------------------------------
    if args.subcommand == "set":
        tag = git_last_tag()
        if not tag:
            sys.exit("No git tags found")

        value = tag.lstrip("v")
        parts = value.split(".")
        if len(parts) != 3 or not all(p.isdigit() for p in parts):
            sys.exit("Invalid tag format")

        version, text = read_version()
        version["MAJOR"], version["MINOR"], version["PATCH"] = map(int, parts)

        write_version(version, text)
        print(f"Version set from tag → {value}")
        return

    # --------------------------------------------------------
    # python version.py tag create [...]
    # --------------------------------------------------------
    if args.subcommand == "create":

        version, text = read_version()

        if args.value is None:
            # use current header version
            pass

        elif args.value in ("major", "minor", "patch"):
            bump(version, args.value)

        else:
            value = args.value.lstrip("v")
            parts = value.split(".")
            if len(parts) != 3 or not all(p.isdigit() for p in parts):
                sys.exit("Invalid version format")
            version["MAJOR"], version["MINOR"], version["PATCH"] = map(int, parts)

        write_version(version, text)

        new_version = version_str(version)
        tag_name = f"v{new_version}"

        run(["git", "add", str(HEADER)])
        run(["git", "commit", "-m", f"Release {new_version}"])
        run(["git", "tag", tag_name])

        print(f"Committed and tagged {tag_name}")
        return


# ============================================================
# CLI
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        prog="version.py",
        description="YYAML version and git tag manager",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog="""
Examples:

  python version.py
      Show header version

  python version.py tag
      Show latest git tag

  python version.py tag set
      Set header version from latest tag

  python version.py tag create
      Commit and tag using current header version

  python version.py tag create minor
      Bump minor, commit and tag

  python version.py tag create v1.2.3
      Set version, commit and tag
"""
    )

    subparsers = parser.add_subparsers(dest="command")

    # default show
    parser.set_defaults(func=cmd_show)

    # tag command
    tag_parser = subparsers.add_parser("tag", help="Manage git tags")
    tag_sub = tag_parser.add_subparsers(dest="subcommand")

    tag_parser.set_defaults(func=cmd_tag)

    tag_sub.add_parser("set", help="Set version from latest tag")

    create_parser = tag_sub.add_parser("create", help="Create release tag")
    create_parser.add_argument(
        "value",
        nargs="?",
        help="major | minor | patch | X.Y.Z"
    )

    args = parser.parse_args()

    if hasattr(args, "func"):
        args.func(args)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()