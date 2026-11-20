/*
 * Passing NULL to a pointer that is non-null by default inside a
 * nullability region is diagnosed (Clang only).
 *
 * mode: fail
 * compilers: clang
 * expect: -Wnonnull
 */
#include <stddef.h>
#include "cxtr.h"

CXTR_ASSUME_NONNULL_BEGIN

void set_name(const char *name);

void caller(void);
void caller(void)
{
    set_name(NULL);
}

CXTR_ASSUME_NONNULL_END
