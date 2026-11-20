/*
 * Every macro in its documented placement compiles warning-free in every
 * standard, including the Clang analyses being satisfied.
 *
 * mode: compile
 */
#include <stddef.h>
#include <stdio.h>
#include "cxtr.h"

/* ---- ownership and allocation ---- */

typedef struct Buffer { size_t size; } Buffer;

void buffer_destroy(Buffer *buffer);
CXTR_MUST_USE CXTR_RETURNS_OWNED(buffer_destroy, 1)
Buffer *buffer_create(size_t size);

CXTR_MUST_USE CXTR_ALLOC_SIZE(1) void *arena_alloc(size_t n);
CXTR_MUST_USE CXTR_ALLOC_SIZE2(1, 2) void *arena_calloc(size_t n, size_t m);
CXTR_MUST_USE CXTR_ALLOC_ALIGN(1) CXTR_ALLOC_SIZE(2)
void *arena_aligned(size_t align, size_t n);

size_t buffer_roundtrip(void);
size_t buffer_roundtrip(void)
{
    Buffer *buffer = buffer_create(16);
    size_t size = buffer ? buffer->size : 0;
    buffer_destroy(buffer);
    return size;
}

/* ---- function attributes ---- */

CXTR_COLD CXTR_NORETURN CXTR_PRINTF(1, 2)
void fatal(const char *fmt, ...);

CXTR_NOINLINE int checked_div(int a, int b);
int checked_div(int a, int b)
{
    if (CXTR_UNLIKELY(b == 0))
        fatal("division of %d by zero", a);
    return CXTR_LIKELY(b == 1) ? a : a / b;
}

CXTR_FORCE_INLINE int twice(int x) { return 2 * x; }

/* ---- capabilities: a lock and a role ---- */

typedef struct CXTR_CAPABILITY("mutex") Mutex { int locked; } Mutex;
void mutex_lock(Mutex *m) CXTR_ACQUIRES(m);
void mutex_unlock(Mutex *m) CXTR_RELEASES(m);

extern Mutex counter_lock;
extern int counter CXTR_GUARDED_BY(counter_lock);

void counter_bump(void) CXTR_EXCLUDES(counter_lock);
void counter_bump(void)
{
    mutex_lock(&counter_lock);
    counter += twice(1);
    mutex_unlock(&counter_lock);
}

int counter_peek(void) CXTR_NO_CAPABILITY_ANALYSIS;
int counter_peek(void) { return counter; }

extern cxtr_role main_thread;
void assert_main_thread(void) CXTR_ASSERTS(main_thread);
void gui_update(int *state) CXTR_REQUIRES(main_thread);

void frame(int *state);
void frame(int *state)
{
    assert_main_thread();
    gui_update(state);
}

/* ---- function effects and restrict ---- */

void mix(float *CXTR_RESTRICT dst, const float *CXTR_RESTRICT src,
         size_t n) CXTR_NONBLOCKING;
void mix(float *CXTR_RESTRICT dst, const float *CXTR_RESTRICT src,
         size_t n) CXTR_NONBLOCKING
{
    size_t i;
    for (i = 0; i < n; ++i)
        dst[i] += src[i];
}

int clamp_index(int i) CXTR_NONALLOCATING;
int clamp_index(int i) CXTR_NONALLOCATING { return i < 0 ? 0 : i; }

/* ---- tier 2 contracts ---- */

#ifdef CXTR_ALIGN
CXTR_ALIGN(16) float simd_scratch[4];
#endif

int parity(int x);
int parity(int x)
{
    CXTR_ASSUME(x >= 0);
    switch (x % 2) {
    case 0: return 0;
    case 1: return 1;
    }
    CXTR_UNREACHABLE();
    return -1;
}

float first_aligned(const float *p);
float first_aligned(const float *p)
{
    const float *aligned = CXTR_ASSUME_ALIGNED(p, 16);
    return aligned[0];
}

#if defined(CXTR_TARGET) && (defined(__x86_64__) || defined(__i386__))
CXTR_TARGET("avx2") static int sum_avx2(const int *v, int n)
{
    int i, s = 0;
    for (i = 0; i < n; ++i)
        s += v[i];
    return s;
}

int sum(const int *v, int n);
int sum(const int *v, int n)
{
    int i, s = 0;
    /* Dispatch: only reach the AVX2 kernel after a runtime CPU check. */
    if (__builtin_cpu_supports("avx2"))
        return sum_avx2(v, n);
    for (i = 0; i < n; ++i)
        s += v[i];
    return s;
}
#endif

CXTR_STATIC_ASSERT(sizeof(Buffer) >= sizeof(size_t), "Buffer holds a size");

void print_line(const char *line);
void print_line(const char *line) { printf("%s\n", line); }
