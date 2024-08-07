#include <omen/libraries/basic/linked_list.h>

void debug_list(struct list_head *head) {
    struct list_head *node;
    printf("%p -> ", head);
    list_for_each(node, head) {
      printf("%p -> ", node);
    }
    printf("%p\n", head);
}
