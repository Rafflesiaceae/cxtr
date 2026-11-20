# cxtr.h

Single-header C annotation vocabulary (C89-C23, C++; GCC, Clang, MSVC). Each `CXTR_*` macro expands to a native, checked feature or to nothing. Annotations are checked contracts, not comments.

## Rules

- **R1** Public names are `CXTR_*` / `cxtr_*`. The header never defines short aliases; a project may in its own header.
- **R2** Prefer real C first: strong ID structs, span structs, `const`, `static`, `CXTR_STATIC_ASSERT`.
- **R3** Every annotation must be checked by something (compiler, analyzer, sanitizer, debug runtime check) or be free.
- **R4** Nullability is unspecified outside `CXTR_ASSUME_NONNULL_BEGIN/END`. Open a region only over audited code.
- **R5** Tier 2 macros change codegen; a false promise is UB. Add one only when the fact is true by construction, never because an optimization report asked for it.
- **R6** Tier 2 assumptions are checked in debug builds. Arguments to `CXTR_ASSUME` and `CXTR_ASSUME_ALIGNED` must have no side effects.
- **R7** Unsupported expands to nothing, except `CXTR_ALIGN` and `CXTR_TARGET`, which stay undefined (test with `#ifdef`). On MSVC `CXTR_TARGET` is empty.
- **R8** Never remove or weaken an annotation to compile or silence a diagnostic. Fix the code, or suppress locally with a comment explaining why the diagnostic is wrong. Never work around a poison pragma.
- **R9** Policy (forbidden APIs, layer rules, capability names, allocator pairs) belongs to the project, not the header.
- **R10** Add vocabulary only for repeated, measured problems.

## Tier 1: checked, no codegen effect

| Macro | Meaning |
|---|---|
| `CXTR_MUST_USE` | result must be used |
| `CXTR_NORETURN` | does not return |
| `CXTR_PRINTF(fmt, va)` | printf-like; 1-based indexes, `va` = 0 for `va_list` functions |
| `CXTR_ALLOC_SIZE(n)` / `CXTR_ALLOC_SIZE2(n, m)` | allocation size is param `n` (or `n * m`) |
| `CXTR_COUNTED_BY(count)` | member `count` = allocated element count of this flexible array / pointer member |
| `CXTR_RETURNS_OWNED(d, i)` | result must be released with `d` (as its param `i`) and nothing else |
| `CXTR_NULLABLE`, `CXTR_NONNULL` | pointer nullability qualifiers |
| `CXTR_ASSUME_NONNULL_BEGIN/END` | region where unannotated pointers are non-null |
| `CXTR_CAPABILITY(kind)` | type is a capability (lock or role) |
| `CXTR_REQUIRES/ACQUIRES/RELEASES/EXCLUDES/ASSERTS(x)` | function requires/acquires/releases/must not hold/asserts capability `x`; one capability per macro, repeat for several |
| `CXTR_GUARDED_BY(x)` | member accessed only while holding `x` |
| `CXTR_NO_CAPABILITY_ANALYSIS` | opt function out of capability analysis |
| `CXTR_NONBLOCKING`, `CXTR_NONALLOCATING` | transitive: every callee must carry the same attribute; NONBLOCKING implies NONALLOCATING |
| `CXTR_STATIC_ASSERT(cond, msg)` | compile-time assert; at most one per line |
| `CXTR_LIKELY(e)`, `CXTR_UNLIKELY(e)`, `CXTR_COLD` | hints, unchecked but harmless |
| `CXTR_NOINLINE` | never inline |
| `CXTR_FORCE_INLINE` | always inline; includes `static` |

## Tier 2: optimizer contracts (false = UB)

| Macro | Meaning |
|---|---|
| `CXTR_RESTRICT` | object accessed only through this pointer in its scope |
| `CXTR_ASSUME(e)` | `e` is true here |
| `CXTR_UNREACHABLE()` | control never reaches here |
| `CXTR_ALIGN(n)` | object is `n`-byte aligned |
| `CXTR_ASSUME_ALIGNED(p, n)` | returns `p`, which is `n`-byte aligned; `n` a constant power of two |
| `CXTR_ALLOC_ALIGN(i)` | allocator result aligned to param `i` |
| `CXTR_TARGET(features)` | runs only on CPUs with `features`, e.g. `"avx2,fma"` |

## Placement

1. Before the return type: `MUST_USE`, `NORETURN`, `PRINTF`, `ALLOC_SIZE*`, `ALLOC_ALIGN`, `RETURNS_OWNED`, `NOINLINE`, `FORCE_INLINE`, `TARGET`, `COLD`.
2. After the parameter list: `REQUIRES`, `ACQUIRES`, `RELEASES`, `EXCLUDES`, `ASSERTS`, `NONBLOCKING`, `NONALLOCATING`, `NO_CAPABILITY_ANALYSIS`.
3. After the `*` they qualify: `NULLABLE`, `NONNULL`, `RESTRICT`.
4. First in an object declaration: `ALIGN`. After the member name: `GUARDED_BY`, `COUNTED_BY`.

Do not mix C23 `[[...]]` attributes with these macros in one declaration.

```c
void buffer_destroy(Buffer *buffer);
CXTR_MUST_USE CXTR_RETURNS_OWNED(buffer_destroy, 1)
Buffer *buffer_create(size_t size);

CXTR_COLD CXTR_NORETURN CXTR_PRINTF(1, 2)
void fatal(const char *fmt, ...);

extern cxtr_role main_thread;            /* declared, never defined */
void assert_main_thread(void) CXTR_ASSERTS(main_thread);
void gui_update(Gui *gui) CXTR_REQUIRES(main_thread);

void mix(float *CXTR_RESTRICT dst, const float *CXTR_RESTRICT src,
         size_t n) CXTR_NONBLOCKING;

CXTR_ALIGN(16) float simd_scratch[4];

typedef struct {
    size_t count;
    Entity items[] CXTR_COUNTED_BY(count);
} EntityList;
```

## Usage notes

- **Nullability**: open the region after all `#include`s, close it in the same file. Use `CXTR_NULLABLE`/`CXTR_NONNULL` only inside regions. Under `-pedantic` add `-Wno-nullability-extension`.
- **Capabilities**: a mutex or a role (thread, phase, callback context). Roles are `cxtr_role` objects. Do not call a `REQUIRES(x)` function unless `x` is held.
- **Counted arrays**: count by capacity, not length. Count must never exceed the allocation, must be declared before the member, must be set before any element access (also after reallocation), and must be 0 while the pointer is NULL. Struct members only, never `void *`.
- **Ownership**: the deallocator must be declared first. Not for arena/pool/frame allocators released in bulk (`CXTR_ALLOC_SIZE` is still fine there).
- **SIMD**: make `CXTR_TARGET` functions `static`, call them only via a dispatcher that checked the CPU at runtime. Never build files with `-m<isa>` to make intrinsics compile.

## Configuration

Define before including (or in the `CXTR_USER_CONFIG` header):

- `CXTR_USER_CONFIG`: header included first, e.g. `-DCXTR_USER_CONFIG='"my_config.h"'`. Any `CXTR_*` macro it defines replaces the built-in one.
- `CXTR_CHECK_ASSUMPTIONS`: 1 checks `ASSUME`, `UNREACHABLE`, `ASSUME_ALIGNED` at runtime; 0 trusts them. Default 1 unless `NDEBUG`.
- `CXTR_CONTRACT_FAILED(text)`: called with a string literal on a failed check; default `abort()`. Must not return.
