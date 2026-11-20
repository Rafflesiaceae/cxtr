#!/usr/bin/env python3
"""Test suite for cxtr.h.

Every tests/*.c file is one test case, compiled with each compiler and
language standard it applies to. Its leading block comment configures it:

    mode: compile | run | fail
        compile  must compile warning-free (-Werror)
        run      must compile warning-free, then exit with status 0
        fail     must not compile, and the diagnostics must match `expect`
    expect: REGEX         required for fail tests, searched in stderr
    compilers: NAME...    default: all of COMPILERS
    std: STD...           default: all of C_STDS and CXX_STDS; a token may
                          also be `c`, `c++` (all of one language) or
                          `cNN+` (that C standard and later ones)
    flags: FLAGS...       extra compiler flags, split like a shell would

Use the compilers from pixi.toml by running this through `./dev.py test`.
"""

import argparse
import concurrent.futures
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

TESTS = Path(__file__).resolve().parent
ROOT = TESTS.parent

C_STDS = ["c89", "c99", "c11", "c17", "c23"]
CXX_STDS = ["c++98", "c++11", "c++17", "c++20"]

# Flags shared by every compilation. -pedantic keeps the header honest about
# the standard it is compiled as; -I TESTS lets tests use helper headers.
BASE_FLAGS = ["-Wall", "-Wextra", "-Werror", "-pedantic", "-O2",
              "-I", str(ROOT), "-I", str(TESTS)]

# Per-compiler flags: clang's analyses are what the Clang-only macros map
# to, so they are switched on for every test.
COMPILERS = {
    "gcc": [],
    "clang": ["-Wthread-safety", "-Wfunction-effects",
              "-Wno-nullability-extension"],
}

DIRECTIVE = re.compile(r"^\s*\*?\s*(mode|expect|compilers|std|flags):\s*(.*?)\s*$")
MODES = ("compile", "run", "fail")


@dataclass
class Case:
    """One test file and the configuration from its leading comment."""
    path: Path
    mode: str = ""
    expect: str = ""
    compilers: list = field(default_factory=lambda: list(COMPILERS))
    stds: list = field(default_factory=lambda: C_STDS + CXX_STDS)
    flags: list = field(default_factory=list)

    @property
    def name(self):
        return self.path.stem


@dataclass
class Job:
    case: Case
    compiler: str
    std: str

    @property
    def label(self):
        return f"{self.case.name} [{self.compiler} -std={self.std}]"


@dataclass
class Result:
    job: Job
    ok: bool
    log: str


def expand_stds(tokens):
    """Resolve the `std:` directive tokens to a list of standards."""
    stds = []
    for token in tokens:
        if token == "c":
            stds += C_STDS
        elif token == "c++":
            stds += CXX_STDS
        elif token.endswith("+") and token[:-1] in C_STDS:
            stds += C_STDS[C_STDS.index(token[:-1]):]
        elif token in C_STDS or token in CXX_STDS:
            stds.append(token)
        else:
            raise ValueError(f"unknown std '{token}'")
    return stds


def parse_case(path):
    """Read the directives from the first block comment of a test file."""
    case = Case(path)
    text = path.read_text()
    match = re.match(r"\s*/\*(.*?)\*/", text, re.S)
    if not match:
        raise ValueError("missing leading block comment with directives")
    for line in match.group(1).splitlines():
        directive = DIRECTIVE.match(line)
        if not directive:
            continue
        key, value = directive.groups()
        if key == "mode":
            case.mode = value
        elif key == "expect":
            case.expect = value
        elif key == "compilers":
            case.compilers = value.split()
        elif key == "std":
            case.stds = expand_stds(value.split())
        elif key == "flags":
            case.flags = shlex.split(value)
    if case.mode not in MODES:
        raise ValueError(f"mode must be one of {', '.join(MODES)}")
    if case.mode == "fail" and not case.expect:
        raise ValueError("fail tests need an `expect` regex")
    unknown = set(case.compilers) - set(COMPILERS)
    if unknown:
        raise ValueError(f"unknown compilers: {', '.join(sorted(unknown))}")
    return case


def check_toolchain(compilers):
    """Make sure each compiler comes from the pixi environment.

    Returns the set of compilers that also have a working C++ frontend:
    conda-forge's gcc package ships only the C compiler, so C++ is checked
    with whichever compilers can actually do it.
    """
    env_root = ROOT / ".pixi" / "envs"
    cxx = set()
    for compiler in compilers:
        found = shutil.which(compiler)
        if not found:
            sys.exit(f"runner.py: {compiler} not found; run via ./dev.py test")
        try:
            Path(found).resolve().relative_to(env_root.resolve())
        except ValueError:
            sys.exit(f"runner.py: {compiler} is {found}, not from the pixi "
                     f"environment; run via ./dev.py test")
        probe = subprocess.run([compiler, "-x", "c++", "-fsyntax-only", "-"],
                               input="", capture_output=True, text=True)
        if probe.returncode == 0:
            cxx.add(compiler)
    return cxx


def run_job(job, workdir, verbose):
    """Compile (and for run tests execute) one test case."""
    case = job.case
    is_cxx = job.std.startswith("c++")
    out = Path(workdir) / f"{case.name}-{job.compiler}-{job.std}"
    command = [job.compiler, f"-std={job.std}", *BASE_FLAGS,
               *COMPILERS[job.compiler], *case.flags]
    if is_cxx:
        command += ["-x", "c++"]
    command.append(str(case.path))
    if case.mode == "run":
        command += ["-o", str(out)]
    else:
        # Compile to an object so diagnostics from later passes (e.g. GCC's
        # -Wmismatched-dealloc) are produced too.
        command += ["-c", "-o", str(out) + ".o"]

    log = [f"$ {shlex.join(command)}"]
    compiled = subprocess.run(command, capture_output=True, text=True)
    if compiled.stdout or compiled.stderr:
        log.append((compiled.stdout + compiled.stderr).rstrip())

    if case.mode == "fail":
        if compiled.returncode == 0:
            log.append("error: compiled, but was expected to fail")
            return Result(job, False, "\n".join(log))
        if not re.search(case.expect, compiled.stderr):
            log.append(f"error: diagnostics do not match /{case.expect}/")
            return Result(job, False, "\n".join(log))
        return Result(job, True, "\n".join(log))

    if compiled.returncode != 0:
        return Result(job, False, "\n".join(log))
    if case.mode == "run":
        log.append(f"$ {out}")
        ran = subprocess.run([str(out)], capture_output=True, text=True)
        if ran.stdout or ran.stderr:
            log.append((ran.stdout + ran.stderr).rstrip())
        if ran.returncode != 0:
            log.append(f"error: exited with status {ran.returncode}")
            return Result(job, False, "\n".join(log))
    return Result(job, True, "\n".join(log) if verbose else "")


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-k", "--filter", default="",
                        help="only run tests whose name contains this text")
    parser.add_argument("--compiler", action="append", choices=list(COMPILERS),
                        help="only use this compiler (repeatable)")
    parser.add_argument("--std", action="append",
                        help="only use this standard, e.g. c11 (repeatable)")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 1,
                        help="parallel compilations (default: %(default)s)")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print the commands and output of passing jobs too")
    args = parser.parse_args()

    compilers = args.compiler or list(COMPILERS)
    cxx_capable = check_toolchain(compilers)
    for compiler in compilers:
        if compiler not in cxx_capable:
            print(f"note: {compiler} has no C++ frontend, skipping C++ for it",
                  file=sys.stderr)

    cases = []
    for path in sorted(TESTS.glob("*.c")):
        if args.filter not in path.stem:
            continue
        try:
            cases.append(parse_case(path))
        except ValueError as error:
            sys.exit(f"runner.py: {path.name}: {error}")

    jobs = [Job(case, compiler, std)
            for case in cases
            for compiler in case.compilers if compiler in compilers
            for std in case.stds
            if (not args.std or std in args.std)
            and (not std.startswith("c++") or compiler in cxx_capable)]
    # An empty selection must not look like a passing test run.
    if not jobs:
        print("runner.py: no tests selected", file=sys.stderr)
        return 1

    failed = 0
    with tempfile.TemporaryDirectory(prefix="cxtr-tests-") as workdir, \
            concurrent.futures.ThreadPoolExecutor(max(1, args.jobs)) as pool:
        futures = [pool.submit(run_job, job, workdir, args.verbose)
                   for job in jobs]
        # Report in submission order so the output is deterministic.
        for future in futures:
            result = future.result()
            failed += not result.ok
            # Passing jobs are only listed with --verbose to keep the
            # output focused on failures.
            if result.ok and not args.verbose:
                continue
            print(f"{'ok  ' if result.ok else 'FAIL'} {result.job.label}")
            if result.log:
                print("    " + result.log.replace("\n", "\n    "))

    print(f"{len(jobs) - failed} passed, {failed} failed "
          f"({len(cases)} tests, {len(jobs)} jobs)")
    return 1 if failed else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
