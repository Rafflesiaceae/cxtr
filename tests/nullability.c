/*
 * Nullability regions and qualifiers compile warning-free.
 *
 * mode: compile
 */
#include <stddef.h>
#include "cxtr.h"

CXTR_ASSUME_NONNULL_BEGIN

size_t name_length(const char *name);
const char *CXTR_NULLABLE find_name(const char *names, int id);
void set_name(const char *CXTR_NONNULL name);

size_t safe_length(const char *CXTR_NULLABLE name);
size_t safe_length(const char *CXTR_NULLABLE name)
{
    return name ? name_length(name) : 0;
}

CXTR_ASSUME_NONNULL_END
