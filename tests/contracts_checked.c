/*
 * With assumption checks on (the default without NDEBUG), every tier 2
 * runtime check reports its text through CXTR_CONTRACT_FAILED, and checks
 * that hold do not.
 *
 * mode: run
 */
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

/* The handler must not return, so it leaves via longjmp. */
static void on_contract_failed(const char *text);
#define CXTR_CONTRACT_FAILED(text) on_contract_failed(text)
#include "cxtr.h"

#if !CXTR_CHECK_ASSUMPTIONS
#  error "assumption checks must default to on without NDEBUG"
#endif

static jmp_buf escape;
static const char *failure;
static double storage[4];

static void on_contract_failed(const char *text)
{
    failure = text;
    longjmp(escape, 1);
}

static void assume_true(void)
{
    volatile int x = 1;
    CXTR_ASSUME(x == 1);
}

static void assume_false(void)
{
    volatile int x = 1;
    CXTR_ASSUME(x == 2);
}

static void unreachable(void)
{
    CXTR_UNREACHABLE();
}

static void aligned_ok(void)
{
    char *p = (char *)storage;
    if (CXTR_ASSUME_ALIGNED(p, 8) != p)
        failure = "ASSUME_ALIGNED changed the pointer";
}

static void misaligned(void)
{
    char *p = (char *)storage + 1;
    (void)CXTR_ASSUME_ALIGNED(p, 4);
}

static void not_power_of_two(void)
{
    char *p = (char *)storage;
    (void)CXTR_ASSUME_ALIGNED(p, 3);
}

/* Runs fn and checks which failure text, if any, it reported. */
static int expect(void (*fn)(void), const char *name, const char *text)
{
    failure = NULL;
    if (setjmp(escape) == 0)
        fn();
    if (text ? (failure && strcmp(failure, text) == 0) : !failure)
        return 0;
    fprintf(stderr, "%s: expected \"%s\", got \"%s\"\n", name,
            text ? text : "(no failure)", failure ? failure : "(no failure)");
    return 1;
}

int main(void)
{
    int errors = 0;
    errors += expect(assume_true, "assume_true", NULL);
    errors += expect(assume_false, "assume_false",
                     "assumption failed: x == 2");
    errors += expect(unreachable, "unreachable", "unreachable code reached");
    errors += expect(aligned_ok, "aligned_ok", NULL);
    errors += expect(misaligned, "misaligned", "assumed alignment failed: p");
    errors += expect(not_power_of_two, "not_power_of_two",
                     "assumed alignment failed: p");
    return errors != 0;
}
