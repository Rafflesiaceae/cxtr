/*
 * CXTR_VERSION is the documented YYYYMMPP number, usable both in #if and
 * in expressions.
 *
 * mode: run
 */
#include "cxtr.h"

#if !defined(CXTR_VERSION) || CXTR_VERSION != 20261100L
#  error "CXTR_VERSION must be 20261100L"
#endif

CXTR_STATIC_ASSERT(CXTR_VERSION / 100 % 100 >= 1, "month out of range");
CXTR_STATIC_ASSERT(CXTR_VERSION / 100 % 100 <= 12, "month out of range");

int main(void)
{
    long version = CXTR_VERSION;
    return version == 20261100L ? 0 : 1;
}
