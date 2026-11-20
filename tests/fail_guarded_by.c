/*
 * Writing a CXTR_GUARDED_BY variable without holding its lock is
 * diagnosed by Clang's thread safety analysis.
 *
 * mode: fail
 * compilers: clang
 * expect: -Wthread-safety
 */
#include "cxtr.h"

typedef struct CXTR_CAPABILITY("mutex") Mutex { int locked; } Mutex;

extern Mutex counter_lock;
extern int counter CXTR_GUARDED_BY(counter_lock);

void counter_bump(void);
void counter_bump(void)
{
    counter++;
}
