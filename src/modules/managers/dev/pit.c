#include <omen/managers/dev/pit.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/io.h>
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/managers/boot/bootloaders/bootloader.h>

struct pit pit;

#define PIT_A 0x40
#define PIT_B 0x41
#define PIT_C 0x42
#define PIT_CONTROL 0x43

#define PIT_MASK 0xFF
#define PIT_SET 0x36

#define TIMER_IRQ 0

void timer_phase(int hz) {
	int divisor = (7159090 + 6/2) / (6 * hz);
	outb(PIT_CONTROL, PIT_SET);
	outb(PIT_A, divisor & PIT_MASK);
	outb(PIT_A, (divisor >> 8) & PIT_MASK);
}

void init_pit(int hertz) {
    kprintf("### PIT STARTUP ###\n");

    pit.boot_epoch = get_boot_time();
    pit.timer_ticks = 0;
    pit.preemption_ticks = 0;
    pit.preemption_enabled = 0;
    pit.hertz = hertz;
    pit.alarms = 0x0;

    kprintf("Hertz: %d\n", hertz);
    
    timer_phase(hertz);
}

void tick() {
    pit.timer_ticks++;
    if (((pit.timer_ticks % ALARM_MIN_TICKS) == 0) && pit.alarms != 0x0) {
        struct pit_alarm * alarm = pit.alarms;
        while (alarm != 0x0) {
            if (alarm->remaining_ticks > 0) {
                alarm->remaining_ticks-=5;
            }
            if (alarm->remaining_ticks <= 0) {
                if (alarm->callback != 0x0) {
                    alarm->callback(alarm->alarm_id, pit.timer_ticks, alarm->rearm_ticks);
                }
                if (alarm->rearm) {
                    alarm->remaining_ticks = alarm->rearm_ticks;
                }
            }
            alarm = alarm->next;
        }
    }
}

int find_free_alarm_id() {
    int id = 0;
    struct pit_alarm * alarm = pit.alarms;
    while (alarm != 0x0) {
        if (alarm->alarm_id == id) {
            id++;
            alarm = pit.alarms;
        } else {
            alarm = alarm->next;
        }
    }
    return id;
}

int add_alarm(int64_t ticks, int rearm, void (*callback)(int, uint64_t, int64_t)) {
    if (ticks < ALARM_MIN_TICKS) {
        panic("Alarm ticks too low\n");
        return -1;
    }
    if (callback == 0x0) {
        panic("Alarm callback is null\n");
        return -1;
    }
    struct pit_alarm * alarm = (struct pit_alarm *)kmalloc(sizeof(struct pit_alarm));
    if (alarm == 0x0) {
        kprintf("Failed to allocate memory for alarm\n");
        return -1;
    }
    alarm->alarm_id = find_free_alarm_id();
    alarm->remaining_ticks = ticks;
    alarm->rearm = rearm;
    alarm->callback = callback;
    alarm->next = pit.alarms;
    pit.alarms = alarm;

    kprintf("Alarm %d added with %ld ticks\n", alarm->alarm_id, ticks);

    return alarm->alarm_id;
}

int64_t get_remaining_ticks(int alarm_id) {
    //Get the ticks until the alarm expires
    struct pit_alarm * alarm = pit.alarms;
    while (alarm != 0x0) {
        if (alarm->alarm_id == alarm_id) {
            return alarm->remaining_ticks;
        }
        alarm = alarm->next;
    }
    return -1;
}

void remove_alarm(int alarm_id) {
    struct pit_alarm * alarm = pit.alarms;
    struct pit_alarm * prev = 0x0;
    while (alarm != 0x0) {
        if (alarm->alarm_id == alarm_id) {
            if (prev == 0x0) {
                pit.alarms = alarm->next;
            } else {
                prev->next = alarm->next;
            }
            kfree(alarm);
            return;
        }
        prev = alarm;
        alarm = alarm->next;
    }
}

uint64_t seconds_to_ticks(uint64_t seconds) {
    return seconds * pit.hertz;
}

uint64_t ticks_to_seconds(uint64_t ticks) {
    if (pit.hertz == 0) return 0;
    if (ticks == 0) return 0;
    if (ticks < pit.hertz) return 1;
    return ticks / pit.hertz;
}

uint64_t ticks_to_ms(uint64_t ticks) {
    if (pit.hertz == 0) return 0;
    if (ticks == 0) return 0;
    if ((ticks * 1000)< pit.hertz) return 1000;
    return (ticks * 1000) / pit.hertz;
}

uint64_t ms_to_ticks(uint64_t ms) {
    //kprintf("MS: %d PITH: %ld\n", ms, pit.hertz);
    uint64_t cuak = ms * pit.hertz;
    if (cuak < 1000) return 1;
    return cuak / 1000; 
}

uint64_t ns_to_ticks(uint64_t ns) {
    //Check if we are requesting at least 1ms
    if (ns < 1000000) {
        panic("PIT hertz too low for ns resolution\n");
        return 0;
    }

    uint64_t millis = ns / 1000000;
    uint64_t ticks = ms_to_ticks(millis);
    if (ticks == 0) {
        panic("PIT hertz too low for ns resolution\n");
    }

    return ticks;
}

uint64_t get_resolution() {
    return 1000000;
    
}

uint64_t ticks_to_ns(uint64_t ticks) {
    if (pit.hertz == 0) return 0;
    if (ticks == 0) return 0;
    if (ticks < pit.hertz) return 1000000;
    uint64_t ns = (ticks * 1000000) / pit.hertz;
    if (ns == 0) panic("PIT hertz too low for ns resolution\n");
    return ns;
}

void set_preeption_ticks(uint64_t ticks) {
    pit.preemption_ticks = ticks;
}

void preempt_toggle() {
    pit.preemption_enabled = !pit.preemption_enabled;
}

uint8_t requires_preemption() {
    if (!pit.preemption_enabled || !pit.preemption_ticks) return 0;
    return pit.timer_ticks % pit.preemption_ticks == 0;
}

void enable_preemption() {
    pit.preemption_enabled = 1;
}

uint64_t get_ticks_since_boot() {
    return pit.timer_ticks;
}

uint64_t get_epoch() {
    return pit.boot_epoch + ticks_to_seconds(pit.timer_ticks);
}