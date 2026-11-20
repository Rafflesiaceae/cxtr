/*
 * Ignoring a CXTR_MUST_USE result is diagnosed.
 *
 * mode: fail
 * expect: unused-result
 */
#include "cxtr.h"

CXTR_MUST_USE int try_open(void);

void caller(void);
void caller(void)
{
    try_open();
}
