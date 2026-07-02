#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../../include/types.h"

void keyboard_init(void);

/* Returns the next pending character, or 0 if the buffer is empty. */
char keyboard_get_char(void);

#endif
