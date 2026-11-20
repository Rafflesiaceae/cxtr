/*
 * Releasing a CXTR_RETURNS_OWNED result with the wrong deallocator is
 * diagnosed (GCC only).
 *
 * mode: fail
 * compilers: gcc
 * expect: mismatched-dealloc
 */
#include <stdlib.h>
#include "cxtr.h"

typedef struct Buffer { size_t size; } Buffer;

void buffer_destroy(Buffer *buffer);
CXTR_MUST_USE CXTR_RETURNS_OWNED(buffer_destroy, 1)
Buffer *buffer_create(size_t size);

void caller(void);
void caller(void)
{
    Buffer *buffer = buffer_create(16);
    free(buffer);
}
