/*
 * A format string that does not match a CXTR_PRINTF function's arguments
 * is diagnosed.
 *
 * mode: fail
 * expect: -W(error=)?format
 */
#include "cxtr.h"

CXTR_PRINTF(1, 2) void log_message(const char *fmt, ...);

void caller(void);
void caller(void)
{
    log_message("%d items", "three");
}
