CFLAGS  = -std=gnu99
CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -Wmissing-declarations
CFLAGS += -I../../../../src
CFLAGS += -I../../..


UNITY_DIR = ../../..
TESTFLAGS = -DUNITY_SUPPORT_64 -DUNITY_OUTPUT_COLOR

.PHONY: test
test: tests.out
	@./tests.out

.PHONY: clean
clean:
	rm -rf *.o *.out

tests.out: *.c
	@echo Compiling $@
	@$(CC) $(CFLAGS) $(TESTFLAGS) $(UNITY_DIR)/unity.c ./*.c -o tests.out
