#include <omen/vdso.h>

int vdso_set_data(vdso_t* vdso, uint8_t id, void* info, int64_t size) {
    vdso_entry_t* entry = vdso->entry;
    while (entry) {
        if (entry->id == id) {
            entry->info = info;
            entry->size = size;
            return 0;
        }
        entry = entry->next;
    }

    return -1;
}

int vdso_get_data(vdso_t* vdso, uint8_t id, void** info, int64_t* size) {
    vdso_entry_t* entry = vdso->entry;
    if (info == 0) {
        return -1;
    }
    while (entry) {
        if (entry->id == id) {
            *info = entry->info;
            if (size != 0) {
                *size = entry->size;
            }
            return 0;
        }
        entry = entry->next;
    }
    *info = 0;
    *size = 0;

    return -1;
}