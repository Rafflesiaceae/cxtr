# AGENTS.md

cxtr is a single drop-in C header of checked semantic annotations.

- `cxtr.h`: the header. Self-contained, MIT-0, nothing to build or link.
- `cxtr.md`: the usage reference, written for LLMs and humans alike.
- `tests/`: one `*.c` file per test case, driven by `tests/runner.py`.
- `dev.py`: entry point; `./dev.py test [ARGS]` runs the suite inside the pixi environment.

## Goals

1. **Correct.** Every macro maps to a native feature that a stock compiler, analyzer, sanitizer or debug check actually enforces, or expands to nothing. A wrong expansion is worse than an empty one; tier 2 macros change codegen, so a wrong one is UB.
2. **Robust.** The header must compile warning-free under `-Wall -Wextra -Werror -pedantic` in every supported language mode, and survive hostile project macros (use `__name__` attribute spellings, wrap every public macro except `CXTR_VERSION` in `#ifndef` so it can be overridden).
3. **Concise.** `cxtr.h` and `cxtr.md` stay as short as correctness allows. Add vocabulary only for repeated, measured problems (R10). Prefer removing text over adding it; no rationale essays, no history.

## Support matrix

- Languages: C89, C99, C11, C17, C23, and parsing as C++ (C++98 to C++20). No variadic macros.
- Compilers: GCC and Clang (tested), MSVC (supported by mapping, not tested in CI). Version-gate features whose spelling or semantics differ between versions; exclude Apple Clang where its version numbers would mislead.
- Platforms: the `platforms` list in `pixi.toml` (linux-64, linux-aarch64, osx-64, osx-arm64, win-64). CI runs the suite on each of them natively.
- Toolchain: the gcc and clang pinned in `pixi.toml`. Never test against system compilers; the runner refuses them.

## Working rules

- Keep `cxtr.h` and `cxtr.md` in sync. Any change to a macro's meaning, placement, gating or configuration updates both in the same commit. Rule numbers R1 to R10 are referenced from header comments; do not renumber.
- Every macro change comes with tests: a `compile` or `run` case for correct use, and a `fail` case where the annotation should produce a diagnostic. A `fail` case must compile once its macro is disabled, so it proves the annotation, not an unrelated error.
- Behaviour must not differ between checked and `NDEBUG` builds except as documented (`CXTR_CHECK_ASSUMPTIONS`).
- Versioning: `YYYYMMPP` (year, month, two-digit patch). Bump it in the `cxtr.h` preamble, `CXTR_VERSION`, and `cxtr.md`, and update `tests/version.c`.
- Public names are `CXTR_*` / `cxtr_*` (R1); the header never defines unprefixed names.
- Run `./dev.py test` before every commit; it must pass with zero failures.
