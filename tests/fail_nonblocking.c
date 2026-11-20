/*
 * A CXTR_NONBLOCKING function calling something not known to be
 * non-blocking is diagnosed by Clang's function effects analysis.
 *
 * mode: fail
 * compilers: clang
 * expect: -Wfunction-effects
 */
#include "cxtr.h"

void *allocate(unsigned long n);

void *audio_callback(void) CXTR_NONBLOCKING;
void *audio_callback(void) CXTR_NONBLOCKING
{
    return allocate(64);
}
