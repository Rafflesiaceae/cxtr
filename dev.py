#!/usr/bin/env python3
"""Developer entry point for cxtr.

    ./dev.py test [ARGS...]   run tests/runner.py, forwarding ARGS to it
                              (see ./dev.py test --help)

Commands run inside the pixi environment from pixi.toml, so they always use
its pinned compilers rather than whatever is on the system PATH. When called
from outside that environment, this script re-executes itself via
`pixi run`.
"""

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent

# Set when re-executing inside pixi, so a broken environment fails loudly
# instead of re-executing forever.
PIXI_GUARD = "CXTR_DEV_INSIDE_PIXI"

# Each command is a script that receives all remaining arguments unchanged.
COMMANDS = {
    "test": ROOT / "tests" / "runner.py",
}


def in_pixi_env():
    """True when the active environment is this project's pixi environment."""
    prefix = os.environ.get("CONDA_PREFIX")
    if not prefix:
        return False
    try:
        Path(prefix).resolve().relative_to(ROOT / ".pixi" / "envs")
    except ValueError:
        return False
    return True


def ensure_pixi_env():
    """Re-execute this script inside `pixi run` unless already there."""
    if in_pixi_env():
        return
    if os.environ.get(PIXI_GUARD):
        sys.exit("dev.py: `pixi run` did not activate the project environment")
    env = dict(os.environ, **{PIXI_GUARD: "1"})
    command = ["pixi", "run", "--manifest-path", str(ROOT / "pixi.toml"),
               "--", sys.executable, str(Path(__file__).resolve()),
               *sys.argv[1:]]
    try:
        sys.exit(subprocess.call(command, env=env))
    except FileNotFoundError:
        sys.exit("dev.py: pixi not found on PATH (https://pixi.sh)")
    except KeyboardInterrupt:
        sys.exit(130)


def usage(stream):
    print(__doc__.strip(), file=stream)


def main():
    # Dispatch by hand instead of argparse so that every argument after the
    # command, including -h/--help, reaches the forwarded script untouched.
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        usage(sys.stdout if len(sys.argv) >= 2 else sys.stderr)
        return 0 if len(sys.argv) >= 2 else 2
    command = sys.argv[1]
    if command not in COMMANDS:
        print(f"dev.py: unknown command '{command}'\n", file=sys.stderr)
        usage(sys.stderr)
        return 2

    ensure_pixi_env()
    try:
        return subprocess.call([sys.executable, str(COMMANDS[command]),
                                *sys.argv[2:]])
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    sys.exit(main())
