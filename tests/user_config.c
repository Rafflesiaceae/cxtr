/*
 * A CXTR_USER_CONFIG header is included first and its definitions replace
 * the built-in ones.
 *
 * mode: run
 * flags: -DNDEBUG -DCXTR_USER_CONFIG='"user_config.h"'
 */
#include <stdlib.h>
#include <string.h>
#include "cxtr.h"

void project_contract_failed(const char *text)
{
    /* Exiting successfully proves this handler, not abort(), was used. */
    exit(strcmp(text, "assumption failed: x == 2") == 0 ? 0 : 2);
}

/* CXTR_MUST_USE is overridden to nothing, so ignoring this is fine. */
CXTR_MUST_USE static int answer(void) { return 42; }

int main(void)
{
    volatile int x = 1;
    answer();
    CXTR_ASSUME(x == 2);
    return 1;
}
