/*
 * A false CXTR_STATIC_ASSERT stops compilation, both as _Static_assert
 * and through the negative-array fallback used before C11 and in C++.
 *
 * mode: fail
 * expect: static.assert|negative
 */
#include "cxtr.h"

CXTR_STATIC_ASSERT(sizeof(char) == 2, "char is one byte");
