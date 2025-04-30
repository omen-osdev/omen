#ifndef _PIT_H
#define _PIT_H

#include <omen/libraries/std/stdint.h>

#define ALARM_MIN_TICKS 5

struct pit_alarm {
    int alarm_id;
    int64_t remaining_ticks;
    uint8_t rearm;
    int64_t rearm_ticks;
    void (*callback)(int, uint64_t, int64_t);
    struct pit_alarm * next;
};

struct pit {
    uint64_t boot_epoch;
    uint64_t hertz;

    volatile uint64_t timer_ticks ;
    volatile uint8_t timer_subticks ;

    volatile uint64_t preemption_ticks;
    volatile uint8_t preemption_enabled;
    struct pit_alarm * alarms;
};

void init_pit();
void tick();

void set_preeption_ticks(uint64_t ticks);
void enable_preemption();
uint8_t requires_preemption();

uint64_t get_epoch();

void remove_alarm(int alarm_id);
int add_alarm(int64_t ticks, int rearm, void (*callback)(int, uint64_t, int64_t));
int64_t get_remaining_ticks(int alarm_id);

uint64_t ns_to_ticks(uint64_t ns);
uint64_t ticks_to_ns(uint64_t ticks);
uint64_t ticks_to_ms(uint64_t ticks);
uint64_t ms_to_ticks(uint64_t ms);

#endif