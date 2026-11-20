/*
 * cxtr.h - checked semantic annotations for C, practical subset.
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Single self-contained header: copy it into your project and include it.
 * There is nothing to build or link, and no attribution is required (see
 * the license at the end of this file). Usage, rules, placement and
 * configuration are documented in cxtr.md.
 *
 * Supported: any C compiler from C89 to C23 (no variadic macros are used),
 * with native mappings for GCC, Clang and MSVC. It also parses as C++.
 * Every macro expands to something a stock compiler checks, or to nothing.
 */

#ifndef CXTR_H
#define CXTR_H

#ifdef CXTR_USER_CONFIG
#  include CXTR_USER_CONFIG
#endif

/*
 * Attribute names below use the reserved __name__ spelling so that project
 * macros such as `#define malloc(n) debug_malloc(n, ...)`, `#define printf`
 * or `#define noreturn _Noreturn` (from <stdnoreturn.h>) cannot break them.
 *
 * Every public macro is wrapped in #ifndef so that CXTR_USER_CONFIG, or a
 * definition made before inclusion, can replace it.
 */

/* ---- detection --------------------------------------------------------- */

#ifdef __has_attribute
#  define CXTR_HAS_ATTR(x) __has_attribute(x)
#else
#  define CXTR_HAS_ATTR(x) 0
#endif

#ifdef __has_builtin
#  define CXTR_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define CXTR_HAS_BUILTIN(x) 0
#endif

#ifdef __has_feature
#  define CXTR_HAS_FEATURE(x) __has_feature(x)
#else
#  define CXTR_HAS_FEATURE(x) 0
#endif

/* GCC major version, or 0 for every other compiler (Clang defines
 * __GNUC__ too, but its attribute support follows its own versions). */
#if defined(__GNUC__) && !defined(__clang__)
#  define CXTR_GCC __GNUC__
#else
#  define CXTR_GCC 0
#endif

/* C11 or later (never true in C++, which has different keywords). */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define CXTR_C11 1
#else
#  define CXTR_C11 0
#endif

#define CXTR_CAT_(a, b) a##b
#define CXTR_CAT(a, b) CXTR_CAT_(a, b)

/* ---- tier 1: checked by the compiler, no optimizer effect -------------- */

#ifndef CXTR_MUST_USE
#  if CXTR_HAS_ATTR(__warn_unused_result__)
#    define CXTR_MUST_USE __attribute__((__warn_unused_result__))
#  elif defined(_MSC_VER)
#    include <sal.h>
#    define CXTR_MUST_USE _Check_return_
#  else
#    define CXTR_MUST_USE
#  endif
#endif

#ifndef CXTR_NORETURN
#  if CXTR_HAS_ATTR(__noreturn__)
#    define CXTR_NORETURN __attribute__((__noreturn__))
#  elif defined(_MSC_VER)
#    define CXTR_NORETURN __declspec(noreturn)
#  elif CXTR_C11
#    define CXTR_NORETURN _Noreturn
#  else
#    define CXTR_NORETURN
#  endif
#endif

/* MinGW's printf may be the MSVCRT one; gnu_printf checks the C99 formats
 * that MinGW's own stdio implements. */
#ifndef CXTR_PRINTF
#  if CXTR_HAS_ATTR(__format__) && defined(__MINGW32__)
#    define CXTR_PRINTF(fmt, va) \
         __attribute__((__format__(__gnu_printf__, fmt, va)))
#  elif CXTR_HAS_ATTR(__format__)
#    define CXTR_PRINTF(fmt, va) __attribute__((__format__(__printf__, fmt, va)))
#  else
#    define CXTR_PRINTF(fmt, va)
#  endif
#endif

#if CXTR_HAS_ATTR(__alloc_size__)
#  ifndef CXTR_ALLOC_SIZE
#    define CXTR_ALLOC_SIZE(n) __attribute__((__alloc_size__(n)))
#  endif
#  ifndef CXTR_ALLOC_SIZE2
#    define CXTR_ALLOC_SIZE2(n, m) __attribute__((__alloc_size__(n, m)))
#  endif
#else
#  ifndef CXTR_ALLOC_SIZE
#    define CXTR_ALLOC_SIZE(n)
#  endif
#  ifndef CXTR_ALLOC_SIZE2
#    define CXTR_ALLOC_SIZE2(n, m)
#  endif
#endif

/* Element count of a flexible array member or pointer member. Enabled only
 * where both forms are accepted (GCC >= 16, Clang >= 21), so every use
 * compiles everywhere. Apple Clang is excluded because its version numbers
 * differ from upstream Clang. */
#ifndef CXTR_COUNTED_BY
#  if CXTR_HAS_ATTR(__counted_by__) && \
      (CXTR_GCC >= 16 || \
       (defined(__clang__) && !defined(__apple_build_version__) && \
        __clang_major__ >= 21))
#    define CXTR_COUNTED_BY(count) __attribute__((__counted_by__(count)))
#  else
#    define CXTR_COUNTED_BY(count)
#  endif
#endif

/* Ownership: ties an allocator to its deallocator (GCC >= 11 analyzer and
 * -Wmismatched-dealloc). Deliberately not mapped to Clang's plain `malloc`
 * attribute, which would also claim the result is non-aliasing. */
#ifndef CXTR_RETURNS_OWNED
#  if CXTR_GCC >= 11
#    define CXTR_RETURNS_OWNED(dealloc, argidx) \
         __attribute__((__malloc__(dealloc, argidx)))
#  else
#    define CXTR_RETURNS_OWNED(dealloc, argidx)
#  endif
#endif

/* Nullability: Clang type qualifiers, opt-in per region (R4). Never mapped
 * to GCC's nonnull/returns_nonnull, which GCC uses to delete null checks. */
#if defined(__clang__) && CXTR_HAS_FEATURE(nullability)
#  ifndef CXTR_NULLABLE
#    define CXTR_NULLABLE _Nullable
#  endif
#  ifndef CXTR_NONNULL
#    define CXTR_NONNULL _Nonnull
#  endif
#  ifndef CXTR_ASSUME_NONNULL_BEGIN
#    define CXTR_ASSUME_NONNULL_BEGIN _Pragma("clang assume_nonnull begin")
#  endif
#  ifndef CXTR_ASSUME_NONNULL_END
#    define CXTR_ASSUME_NONNULL_END _Pragma("clang assume_nonnull end")
#  endif
#else
#  ifndef CXTR_NULLABLE
#    define CXTR_NULLABLE
#  endif
#  ifndef CXTR_NONNULL
#    define CXTR_NONNULL
#  endif
#  ifndef CXTR_ASSUME_NONNULL_BEGIN
#    define CXTR_ASSUME_NONNULL_BEGIN
#  endif
#  ifndef CXTR_ASSUME_NONNULL_END
#    define CXTR_ASSUME_NONNULL_END
#  endif
#endif

/* Before C11 a typedef of an array with negative size fails to compile.
 * The typedef name includes __LINE__, so use at most one per line. */
#ifndef CXTR_STATIC_ASSERT
#  if CXTR_C11
#    define CXTR_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#  else
#    define CXTR_STATIC_ASSERT(cond, msg) \
         typedef char CXTR_CAT(cxtr_static_assert_, __LINE__)[(cond) ? 1 : -1]
#  endif
#endif

#if CXTR_HAS_BUILTIN(__builtin_expect) || defined(__GNUC__)
#  ifndef CXTR_LIKELY
#    define CXTR_LIKELY(e) __builtin_expect(!!(e), 1)
#  endif
#  ifndef CXTR_UNLIKELY
#    define CXTR_UNLIKELY(e) __builtin_expect(!!(e), 0)
#  endif
#else
#  ifndef CXTR_LIKELY
#    define CXTR_LIKELY(e) (e)
#  endif
#  ifndef CXTR_UNLIKELY
#    define CXTR_UNLIKELY(e) (e)
#  endif
#endif

/* Rarely executed: placed out of line, calls predicted unlikely. */
#ifndef CXTR_COLD
#  if CXTR_HAS_ATTR(__cold__)
#    define CXTR_COLD __attribute__((__cold__))
#  else
#    define CXTR_COLD
#  endif
#endif

#ifndef CXTR_NOINLINE
#  if CXTR_HAS_ATTR(__noinline__)
#    define CXTR_NOINLINE __attribute__((__noinline__))
#  elif defined(_MSC_VER)
#    define CXTR_NOINLINE __declspec(noinline)
#  else
#    define CXTR_NOINLINE
#  endif
#endif

/* Includes `static`: write `CXTR_FORCE_INLINE int f(void) { ... }`. */
#ifndef CXTR_FORCE_INLINE
#  if CXTR_HAS_ATTR(__always_inline__)
#    define CXTR_FORCE_INLINE static __inline__ __attribute__((__always_inline__))
#  elif defined(_MSC_VER)
#    define CXTR_FORCE_INLINE static __forceinline
#  else
#    define CXTR_FORCE_INLINE static
#  endif
#endif

/* ---- tier 1: Clang analyses (postfix placement) ------------------------ */

#if defined(__clang__) && CXTR_HAS_ATTR(__capability__)
#  ifndef CXTR_CAPABILITY
#    define CXTR_CAPABILITY(kind) __attribute__((__capability__(kind)))
#  endif
#  ifndef CXTR_REQUIRES
#    define CXTR_REQUIRES(x) __attribute__((__requires_capability__(x)))
#  endif
#  ifndef CXTR_ACQUIRES
#    define CXTR_ACQUIRES(x) __attribute__((__acquire_capability__(x)))
#  endif
#  ifndef CXTR_RELEASES
#    define CXTR_RELEASES(x) __attribute__((__release_capability__(x)))
#  endif
#  ifndef CXTR_EXCLUDES
#    define CXTR_EXCLUDES(x) __attribute__((__locks_excluded__(x)))
#  endif
#  ifndef CXTR_ASSERTS
#    define CXTR_ASSERTS(x) __attribute__((__assert_capability__(x)))
#  endif
#  ifndef CXTR_GUARDED_BY
#    define CXTR_GUARDED_BY(x) __attribute__((__guarded_by__(x)))
#  endif
#  ifndef CXTR_NO_CAPABILITY_ANALYSIS
#    define CXTR_NO_CAPABILITY_ANALYSIS \
         __attribute__((__no_thread_safety_analysis__))
#  endif
#else
#  ifndef CXTR_CAPABILITY
#    define CXTR_CAPABILITY(kind)
#  endif
#  ifndef CXTR_REQUIRES
#    define CXTR_REQUIRES(x)
#  endif
#  ifndef CXTR_ACQUIRES
#    define CXTR_ACQUIRES(x)
#  endif
#  ifndef CXTR_RELEASES
#    define CXTR_RELEASES(x)
#  endif
#  ifndef CXTR_EXCLUDES
#    define CXTR_EXCLUDES(x)
#  endif
#  ifndef CXTR_ASSERTS
#    define CXTR_ASSERTS(x)
#  endif
#  ifndef CXTR_GUARDED_BY
#    define CXTR_GUARDED_BY(x)
#  endif
#  ifndef CXTR_NO_CAPABILITY_ANALYSIS
#    define CXTR_NO_CAPABILITY_ANALYSIS
#  endif
#endif

/* A role is a capability with no runtime state (thread, phase, context).
 * Role objects are only named inside attributes, so they need an extern
 * declaration but no definition. */
typedef struct CXTR_CAPABILITY("role") cxtr_role { int unused_; } cxtr_role;

/* Function effects. These are type attributes and must follow the
 * parameter list. NONBLOCKING implies NONALLOCATING. */
#if defined(__clang__) && CXTR_HAS_ATTR(__nonblocking__)
#  ifndef CXTR_NONBLOCKING
#    define CXTR_NONBLOCKING __attribute__((__nonblocking__))
#  endif
#  ifndef CXTR_NONALLOCATING
#    define CXTR_NONALLOCATING __attribute__((__nonallocating__))
#  endif
#else
#  ifndef CXTR_NONBLOCKING
#    define CXTR_NONBLOCKING
#  endif
#  ifndef CXTR_NONALLOCATING
#    define CXTR_NONALLOCATING
#  endif
#endif

/* ---- tier 2: optimizer contracts --------------------------------------- */

#ifndef CXTR_RESTRICT
#  if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#    define CXTR_RESTRICT restrict
#  elif defined(__GNUC__) || defined(_MSC_VER)
#    define CXTR_RESTRICT __restrict
#  else
#    define CXTR_RESTRICT
#  endif
#endif

/* Object alignment. Deliberately undefined when unsupported (R7): silently
 * dropping it could make code that relies on it incorrect. */
#ifndef CXTR_ALIGN
#  if CXTR_C11
#    define CXTR_ALIGN(n) _Alignas(n)
#  elif defined(__GNUC__)
#    define CXTR_ALIGN(n) __attribute__((__aligned__(n)))
#  elif defined(_MSC_VER)
#    define CXTR_ALIGN(n) __declspec(align(n))
#  endif
#endif

/* ISA precondition. Deliberately undefined when unsupported (R7). MSVC
 * needs no attribute: it accepts any intrinsic in any function. */
#ifndef CXTR_TARGET
#  if CXTR_HAS_ATTR(__target__)
#    define CXTR_TARGET(features) __attribute__((__target__(features)))
#  elif defined(_MSC_VER)
#    define CXTR_TARGET(features)
#  endif
#endif

/* Parameter argidx of an allocator is the alignment of its result. */
#ifndef CXTR_ALLOC_ALIGN
#  if CXTR_HAS_ATTR(__alloc_align__)
#    define CXTR_ALLOC_ALIGN(argidx) __attribute__((__alloc_align__(argidx)))
#  else
#    define CXTR_ALLOC_ALIGN(argidx)
#  endif
#endif

/* Runtime verification of tier 2 assumptions (R6). */
#ifndef CXTR_CHECK_ASSUMPTIONS
#  ifdef NDEBUG
#    define CXTR_CHECK_ASSUMPTIONS 0
#  else
#    define CXTR_CHECK_ASSUMPTIONS 1
#  endif
#endif

#ifndef CXTR_CONTRACT_FAILED
#  include <stdlib.h>
#  define CXTR_CONTRACT_FAILED(text) abort()
#endif

/* Internal: optimizer hints without runtime checks. */
#if CXTR_HAS_BUILTIN(__builtin_unreachable) || defined(__GNUC__)
#  define CXTR_UNREACHABLE_HINT_() __builtin_unreachable()
#elif defined(_MSC_VER)
#  define CXTR_UNREACHABLE_HINT_() __assume(0)
#else
#  define CXTR_UNREACHABLE_HINT_() ((void)0)
#endif

#if CXTR_HAS_BUILTIN(__builtin_assume)
#  define CXTR_ASSUME_HINT_(e) __builtin_assume(e)
#elif defined(_MSC_VER)
#  define CXTR_ASSUME_HINT_(e) __assume(e)
#elif defined(__GNUC__)
#  define CXTR_ASSUME_HINT_(e) ((e) ? (void)0 : __builtin_unreachable())
#else
#  define CXTR_ASSUME_HINT_(e) ((void)0)
#endif

/* Checked builds report through CXTR_CONTRACT_FAILED; unchecked builds
 * hand the fact to the optimizer. */
#if CXTR_CHECK_ASSUMPTIONS
#  ifndef CXTR_ASSUME
#    define CXTR_ASSUME(e) \
         ((e) ? (void)0 \
              : (CXTR_CONTRACT_FAILED("assumption failed: " #e), (void)0))
#  endif
#  ifndef CXTR_UNREACHABLE
#    define CXTR_UNREACHABLE() \
         (CXTR_CONTRACT_FAILED("unreachable code reached"), \
          CXTR_UNREACHABLE_HINT_())
#  endif
#else
#  ifndef CXTR_ASSUME
#    define CXTR_ASSUME(e) CXTR_ASSUME_HINT_(e)
#  endif
#  ifndef CXTR_UNREACHABLE
#    define CXTR_UNREACHABLE() CXTR_UNREACHABLE_HINT_()
#  endif
#endif

/* Returns p, promising it is n-byte aligned. p is evaluated more than once
 * in checked builds and must be a side-effect-free pointer. The checked
 * branch also rejects an n that is not a power of two. The GNU branch casts
 * back so the result keeps the type of p. */
#ifndef CXTR_ASSUME_ALIGNED
#  if CXTR_CHECK_ASSUMPTIONS
#    if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || \
        (defined(_MSC_VER) && _MSC_VER >= 1600)
#      include <stdint.h>
#      define CXTR_UINTPTR_ uintptr_t
#    else
#      include <stddef.h>
#      define CXTR_UINTPTR_ size_t
#    endif
#    define CXTR_ASSUME_ALIGNED(p, n) \
         (((n) & ((n) - 1)) == 0 && (CXTR_UINTPTR_)(p) % (n) == 0 \
              ? (p) \
              : (CXTR_CONTRACT_FAILED("assumed alignment failed: " #p), (p)))
#  elif (CXTR_HAS_BUILTIN(__builtin_assume_aligned) || CXTR_GCC >= 5) && \
        defined(__GNUC__)
#    define CXTR_ASSUME_ALIGNED(p, n) \
         ((__typeof__(p))__builtin_assume_aligned((p), (n)))
#  else
#    define CXTR_ASSUME_ALIGNED(p, n) (p)
#  endif
#endif

#endif /* CXTR_H */

/*
 * ==== License ==============================================================
 *
 * MIT No Attribution
 *
 * Copyright 2026 the cxtr authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
