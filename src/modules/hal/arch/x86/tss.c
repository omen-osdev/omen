#include <omen/hal/arch/x86/tss.h>
#include <omen/libraries/std/string.h>

#define DLP_CONVERT(dpl) (dpl == 0 ? 0 : (dpl-1))

void tss_set_stack(struct tss* tss, void* stack, uint8_t dpl) {
    tss->rsp[DLP_CONVERT(dpl)] = (uint64_t) stack;
}

uint64_t tss_get_stack(struct tss* tss, uint8_t dpl) {
    return tss->rsp[DLP_CONVERT(dpl)];
}

void tss_set_ist(struct tss* tss, uint8_t ist, uint64_t stack) {
    tss->ist[ist] = stack;
}

uint64_t tss_get_ist(struct tss* tss, uint8_t ist) {
    return tss->ist[ist];
}