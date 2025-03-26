#include <omen/apps/debug/debug.h>
#include <omen/libraries/basic/linked_list.h>

void debug_list(struct list_head *head) {
    struct list_head *node;
    kprintf("%p -> ", head);
    list_for_each(node, head) {
      kprintf("%p -> ", node);
    }
    kprintf("%p\n", head);
}
