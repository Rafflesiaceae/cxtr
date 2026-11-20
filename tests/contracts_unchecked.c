/*
 * With NDEBUG, assumption checks default to off and the tier 2 macros
 * become pure optimizer hints. CXTR_ASSUME_ALIGNED keeps the pointer's
 * value and type (C++ would reject a void * result below).
 *
 * mode: run
 * flags: -DNDEBUG
 */
#include "cxtr.h"

#if CXTR_CHECK_ASSUMPTIONS
#  error "assumption checks must default to off with NDEBUG"
#endif

static double storage[4];

static int pick(int x)
{
    CXTR_ASSUME(x == 0 || x == 1);
    if (x == 0)
        return 10;
    if (x == 1)
        return 20;
    CXTR_UNREACHABLE();
    return -1;
}

int main(void)
{
    volatile int one = 1;
    double *p = storage;
    double *aligned = CXTR_ASSUME_ALIGNED(p, 8);
    if (aligned != p)
        return 1;
    return pick(one) == 20 ? 0 : 1;
}
