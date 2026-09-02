#!/usr/bin/env python3
"""Create/update AIM Editor's repository-local Python validation environment.

The application build must not depend on distro-provided jsonschema versions.  This
helper owns .deps/python-tools, installs the fully pinned tooling requirements, and
never writes to the user's/system Python environment.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import venv

ROOT = Path(__file__).resolve().parents[1]
VENV = ROOT / ".deps" / "python-tools"
REQUIREMENTS = ROOT / "tools" / "requirements-tools.txt"
STAMP = VENV / ".aim-editor-requirements.sha256"
MIN_PYTHON = (3, 10)


def venv_python() -> Path:
    if os.name == "nt":
        return VENV / "Scripts" / "python.exe"
    return VENV / "bin" / "python"


def requirements_digest() -> str:
    return hashlib.sha256(REQUIREMENTS.read_bytes()).hexdigest()


def environment_is_current(python: Path, digest: str) -> bool:
    if not python.is_file() or not STAMP.is_file():
        return False
    if STAMP.read_text(encoding="utf-8", errors="replace").strip() != digest:
        return False
    probe = (
        "from jsonschema import Draft202012Validator; "
        "from referencing import Registry, Resource; "
        "import jsonschema; print('ok')"
    )
    result = subprocess.run(
        [str(python), "-c", probe],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return result.returncode == 0


def create_environment() -> None:
    try:
        builder = venv.EnvBuilder(with_pip=True, clear=True)
        builder.create(VENV)
    except Exception as exc:
        print(f"ERROR: could not create project-local Python environment: {exc}", file=sys.stderr)
        if sys.platform.startswith("linux"):
            print("Debian/Ubuntu hint: sudo apt-get install python3-venv", file=sys.stderr)
        raise SystemExit(2) from exc


def install_requirements(python: Path) -> None:
    command = [
        str(python),
        "-m",
        "pip",
        "install",
        "--disable-pip-version-check",
        "--requirement",
        str(REQUIREMENTS),
    ]
    print("+ " + " ".join(command), flush=True)
    try:
        subprocess.run(command, cwd=ROOT, check=True)
    except subprocess.CalledProcessError as exc:
        print(
            "ERROR: failed to install the pinned repository-validation Python packages. "
            "The host/system Python was not modified.",
            file=sys.stderr,
        )
        raise SystemExit(exc.returncode) from exc


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--force", action="store_true", help="recreate the local validation environment")
    args = parser.parse_args()

    if sys.version_info < MIN_PYTHON:
        print(
            f"ERROR: Python {MIN_PYTHON[0]}.{MIN_PYTHON[1]}+ is required for repository tooling; "
            f"found {sys.version.split()[0]}",
            file=sys.stderr,
        )
        return 2
    if not REQUIREMENTS.is_file():
        print(f"ERROR: missing {REQUIREMENTS.relative_to(ROOT)}", file=sys.stderr)
        return 2

    digest = requirements_digest()
    python = venv_python()
    if not args.force and environment_is_current(python, digest):
        print(f"PASS: pinned Python validation environment ready at {VENV}")
        return 0

    print("== Bootstrap pinned Python validation tools ==")
    print(f"Host Python : {sys.version.split()[0]}")
    print(f"Target      : {VENV}")
    create_environment()
    python = venv_python()
    if not python.is_file():
        print(f"ERROR: virtual-environment interpreter was not created at {python}", file=sys.stderr)
        return 2

    install_requirements(python)
    STAMP.write_text(digest + "\n", encoding="utf-8")
    if not environment_is_current(python, digest):
        print("ERROR: pinned Python validation environment failed its import self-test", file=sys.stderr)
        return 2

    print(f"PASS: pinned Python validation environment ready at {VENV}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
