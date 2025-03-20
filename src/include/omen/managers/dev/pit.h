#ifndef _PIT_H
#define _PIT_H

#include <omen/libraries/std/stdint.h>

struct pit {
    uint64_t boot_epoch;
    uint64_t hertz;

    volatile uint64_t timer_ticks ;
    volatile uint8_t timer_subticks ;

    volatile uint64_t preemption_ticks;
    volatile uint8_t preemption_enabled;

    void (*wakeup_handler)();
    volatile uint64_t wakeup_ticks;
};

void init_pit();

void sleep(uint64_t);
void sleep_ticks(uint64_t);

void tick();

void set_wakeup_call(void (* handler)(), uint64_t ticks);
void set_preeption_ticks(uint64_t ticks);
void enable_preemption();
void preempt_toggle();
uint8_t requires_preemption();
uint8_t requires_wakeup();
void wakeup();
uint64_t get_ticks_since_boot();
uint64_t get_epoch();

uint64_t ticks_to_ms(uint64_t ticks);
uint64_t ms_to_ticks(uint64_t ms);

#endif