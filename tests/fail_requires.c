/*
 * Calling a CXTR_REQUIRES function without holding its role is diagnosed
 * by Clang's thread safety analysis.
 *
 * mode: fail
 * compilers: clang
 * expect: -Wthread-safety
 */
#include "cxtr.h"

extern cxtr_role main_thread;
void gui_update(void) CXTR_REQUIRES(main_thread);

void worker(void);
void worker(void)
{
    gui_update();
}
