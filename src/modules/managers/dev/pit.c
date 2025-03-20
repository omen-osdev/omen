#include <omen/managers/dev/pit.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/hal/arch/x86/int.h>
#include <omen/hal/arch/x86/io.h>
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
    pit.wakeup_handler = (void*)0x0;
    pit.wakeup_ticks = 0;

    kprintf("Hertz: %d\n", hertz);
    
    timer_phase(hertz);
}

void tick() {
    pit.timer_ticks++;
}

void wakeup() {
    pit.wakeup_handler();
}

uint8_t requires_wakeup() {
    return (pit.wakeup_handler != 0x0 && pit.wakeup_ticks != 0 && pit.timer_ticks % pit.wakeup_ticks);
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
    //printf("MS: %d PITH: %ld\n", ms, pit.hertz);
    uint64_t cuak = ms * pit.hertz;
    if (cuak < 1000) return 1;
    return cuak / 1000; 
}

void set_wakeup_call(void (* handler)(), uint64_t ticks) {
    if (handler == 0x0) {
        pit.wakeup_handler = (void*)0x0;
        pit.wakeup_ticks = 0;
    } else {
        pit.wakeup_handler = handler;
        pit.wakeup_ticks = ticks;
    }
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

void sleep_ticks(uint64_t ticks) {
    uint64_t start_time = pit.timer_ticks;
    while (pit.timer_ticks - start_time < ticks) {
        //yield();
        panic("Not implemented\n");
    }
}

void sleep(uint64_t seconds) {
    sleep_ticks(seconds_to_ticks(seconds));
}

uint64_t get_ticks_since_boot() {
    return pit.timer_ticks;
}


uint64_t get_epoch() {
    return pit.boot_epoch + ticks_to_seconds(pit.timer_ticks);
}