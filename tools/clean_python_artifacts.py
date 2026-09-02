#!/usr/bin/env python3
"""Remove untracked Python bytecode/cache artifacts from the repository tree.

AIM Editor's validation scripts are intentionally executable with multiple Python
versions.  A direct invocation without PYTHONDONTWRITEBYTECODE may leave
``__pycache__`` directories in Source-adjacent tooling.  Those files are runtime
artifacts, not repository content.

This helper is conservative:
- tracked cache artifacts are treated as an error and are never silently removed;
- untracked cache artifacts outside known runtime/build roots are removed;
- .git, .deps, IDE metadata, and build directories are ignored.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
IGNORED_RUNTIME_ROOTS = {".git", ".deps", ".idea", ".vscode"}


def is_runtime_generated(path: Path) -> bool:
    try:
        relative = path.relative_to(ROOT)
    except ValueError:
        return True
    if not relative.parts:
        return False
    first = relative.parts[0]
    return (
        first in IGNORED_RUNTIME_ROOTS
        or first == "build"
        or first.startswith("build-")
        or first.startswith("cmake-build-")
    )


def is_cache_path(path: Path) -> bool:
    return path.suffix.lower() in {".pyc", ".pyo"} or "__pycache__" in path.parts


def tracked_paths() -> set[Path]:
    try:
        result = subprocess.run(
            ["git", "ls-files", "-z"],
            cwd=ROOT,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return set()

    tracked: set[Path] = set()
    for raw in result.stdout.split(b"\0"):
        if raw:
            tracked.add(ROOT / raw.decode("utf-8", errors="surrogateescape"))
    return tracked


def discover_cache_artifacts() -> list[Path]:
    return sorted(
        p
        for p in ROOT.rglob("*")
        if not is_runtime_generated(p) and is_cache_path(p)
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--check",
        action="store_true",
        help="report cache artifacts without deleting untracked runtime files",
    )
    args = parser.parse_args()

    artifacts = discover_cache_artifacts()
    tracked = tracked_paths()
    tracked_artifacts = [p for p in artifacts if p.is_file() and p in tracked]
    if tracked_artifacts:
        print("FAIL: Python cache artifacts are tracked by git:", file=sys.stderr)
        for path in tracked_artifacts:
            print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
        return 1

    if args.check:
        if artifacts:
            print("FAIL: Python cache artifacts are present in the repository working tree:", file=sys.stderr)
            for path in artifacts:
                print(f"  {path.relative_to(ROOT)}", file=sys.stderr)
            return 1
        print("PASS: no Python cache artifacts in repository working tree")
        return 0

    files_removed = 0
    directories_removed = 0

    # Remove cache directories first.  This also removes any contained .pyc files.
    cache_dirs = sorted(
        (p for p in artifacts if p.is_dir() and p.name == "__pycache__"),
        key=lambda p: len(p.parts),
        reverse=True,
    )
    for directory in cache_dirs:
        if directory.exists():
            shutil.rmtree(directory)
            directories_removed += 1

    for path in artifacts:
        if path.is_file() and path.exists():
            path.unlink()
            files_removed += 1

    if files_removed or directories_removed:
        print(
            "PASS: removed generated Python cache artifacts "
            f"({directories_removed} director{'y' if directories_removed == 1 else 'ies'}, "
            f"{files_removed} standalone file{'s' if files_removed != 1 else ''})"
        )
    else:
        print("PASS: no generated Python cache artifacts required cleanup")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
