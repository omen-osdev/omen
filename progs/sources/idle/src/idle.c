#include <minilibc.h>

int main(int argc, char* argv[]) {
    while(1) {
        sys_sched_yield();
    }
}