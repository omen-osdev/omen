CC := gcc

SRC_DIR := ./src
BUILD_DIR := ./build
TEST_DIR := ./test
DEPENDENCIES_DIR := ./dependencies

UNITY_DIR := $(DEPENDENCIES_DIR)/Unity/src
TESTFLAGS := -DUINITY_SUPPORT_64 -DUNITY_OUTPUT_COLOR

CFLAGS := -std=gnu99
CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -Wmissing-declarations
CFLAGS += -I$(SRC_DIR)
CFLAGS += -I$(UNITY_DIR)

TEST_SRCS := $(shell find $(TEST_DIR) -type f -name '*.c')
TEST_BINS := $(patsubst %.c, %.test-out, $(TEST_SRCS))

setup:
	mkdir -p "$(BUILD_DIR)"
	mkdir -p "$(DEPENDENCIES_DIR)"

clean:
	rm -rf "$(BUILD_DIR)"
	rm -rf "$(DEPENDENCIES_DIR)"

setup-test: setup
	@if [ ! -d $(DEPENDENCIES_DIR)/Unity ]; then \
		echo "Cloning Unity test framework"; \
		git clone git@github.com:ThrowTheSwitch/Unity.git "$(DEPENDENCIES_DIR)/Unity" --branch v2.6.0; \
	fi

.PHONY: test
test: setup-test $(TEST_SRCS:.c=.test-out)
	@for testfile in $(TEST_BINS); do                \
		echo -e "\033[35mRunning $$testfile\n\033[0m"; \
		"./$$testfile";                                \
	done

%.test-out: %.c
	@$(CC) $(CFLAGS) $(TESTFLAGS) $(UNITY_DIR)/unity.c $< -o $@

clean-test:
	@for testfile in $(TEST_BINS); do \
		echo "Removing $$testfile";     \
		rm -f "$$testfile";             \
	done
