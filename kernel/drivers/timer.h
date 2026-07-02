#ifndef TIMER_H
#define TIMER_H

#include "../../include/types.h"

void timer_init(uint32_t frequency_hz);
uint32_t timer_get_ticks(void);
uint32_t timer_get_seconds(void);

#endif
