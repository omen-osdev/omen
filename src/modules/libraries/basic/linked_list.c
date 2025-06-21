#include <omen/apps/debug/debug.h>
#include <omen/libraries/basic/linked_list.h>

void debug_list(struct list_head *head) {
    struct list_head *node;
    DBG_INFO("%p -> ", head);
    list_for_each(node, head) {
      DBG_INFO("%p -> ", node);
    }
    DBG_INFO("%p\n", head);
}
