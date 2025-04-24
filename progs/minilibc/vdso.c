#include <vdso.h>

void vdso_set_data(vdso_t* vdso, uint8_t id, void* info, uint64_t size) {
    vdso_entry_t* entry = vdso->entry;
    while (entry) {
        if (entry->id == id) {
            entry->info = info;
            entry->size = size;
            return;
        }
        entry = entry->next;
    }
}

void vdso_get_data(vdso_t* vdso, uint8_t id, void** info, uint64_t* size) {
    vdso_entry_t* entry = vdso->entry;
    if (info == 0) {
        return;
    }
    while (entry) {
        if (entry->id == id) {
            *info = entry->info;
            if (size != 0) {
                *size = entry->size;
            }
            return;
        }
        entry = entry->next;
    }
    *info = 0;
    *size = 0;
}