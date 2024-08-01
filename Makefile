CC:=gcc

CFLAGS  = -std=gnu99
CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -Wmissing-declarations
CFLAGS += -I$(SRC_DIR)
CFLAGS += -I$(ROOT_DIR)

ROOT_DIR := .
SRC_DIR := ./src
BUILD_DIR := ./build
TEST_DIR := ./test
DEPENDENCIES_DIR := ./dependencies

UNITY_DIR := $(DEPENDENCIES_DIR)/Unity/src
TESTFLAGS = -DUINITY_SUPPORT_64 -DUNITY_OUTPUT_COLOR

setup:
	mkdir "$(BUILD_DIR)"
	mkdir "$(DEPENDENCIES_DIR)"

setup-test:
	git clone git@github.com:ThrowTheSwitch/Unity.git "$(DEPENDENCIES_DIR)/Unity" --branch v2.6.0

test: tests.out
	@"$(BUILD_DIR)/tests.out"

tests.out: "$(TEST_DIR)/modules/basics/linkedListLibrary/*.c"
	@echo Compiling $@
	@$(CC) $(CFLAGS) $(TESTFLAGS) $(UNITY_DIR)/unity.c ./*.c -o "$(BUILD_DIR)/tests.out"

clean:
	rm -rf "$(BUILD_DIR)"
	rm -rf "$(DEPENDENCIES_DIR)"
