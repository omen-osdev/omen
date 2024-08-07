CC := gcc

INCLUDE_DIR := ./include
SRC_DIR := ./src
BUILD_DIR := ./build
TEST_DIR := ./test
DEPENDENCIES_DIR := ./dependencies

UNITY_DIR := $(DEPENDENCIES_DIR)/Unity/src
TESTFLAGS := -DUINITY_SUPPORT_64 -DUNITY_OUTPUT_COLOR

CFLAGS := -std=gnu11
CFLAGS += -g
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -Wmissing-declarations
CFLAGS += -I$(SRC_DIR)
CFLAGS += -I$(INCLUDE_DIR)
CFLAGS += -I$(UNITY_DIR)

SRCS := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

UNIT_TEST_SRCS := $(shell find $(TEST_DIR) -type f -name '*.unit.c')
UNIT_TEST_BINS := $(patsubst $(TEST_DIR)/%.unit.c, $(BUILD_DIR)/%.unit-test-out, $(UNIT_TEST_SRCS))
E2E_TEST_SRCS := $(shell find $(TEST_DIR) -type f -name '*.e2e.c')
E2E_TEST_BINS := $(patsubst $(TEST_DIR)/%.e2e.c, $(BUILD_DIR)/%.e2e-test-out, $(E2E_TEST_SRCS))

setup:
	mkdir -p "$(BUILD_DIR)"
	mkdir -p "$(DEPENDENCIES_DIR)"

clean:
	rm -rf "$(BUILD_DIR)"
	rm -rf "$(DEPENDENCIES_DIR)"

setup-test: setup
	@if [ ! -d $(DEPENDENCIES_DIR)/Unity ]; then \
		echo "Cloning Unity test framework"; \
		git clone https://github.com/ThrowTheSwitch/Unity.git "$(DEPENDENCIES_DIR)/Unity" --branch v2.6.0; \
	fi

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	echo "hello"
	@mkdir -p $$(dirname $@)  # Create the directory structure in BUILD_DIR
	$(CC) $(CFLAGS) -c $< -o $@

test: e2e-test unit-test 

unit-test: setup-test $(UNIT_TEST_BINS)
	@for testfile in $(UNIT_TEST_BINS); do                \
		echo -e "\033[35mRunning $$testfile\n\033[0m"; \
		"./$$testfile";                                \
	done

$(BUILD_DIR)/%.unit-test-out: $(TEST_DIR)/%.unit.c $(OBJS)
	@echo "In: $<"
	@echo "Out: $@"
	@mkdir -p $$(dirname $@)  # Create the directory structure in BUILD_DIR
	@$(CC) $(CFLAGS) $(TESTFLAGS) $(UNITY_DIR)/unity.c $< $(OBJS) -o $@

e2e-test: setup-test $(E2E_TEST_BINS)
	@for testfile in $(E2E_TEST_BINS); do                \
		echo -e "\033[35mRunning $$testfile\n\033[0m"; \
		"./$$testfile";                                \
	done

$(BUILD_DIR)/%.e2e-test-out: $(TEST_DIR)/%.e2e.c $(OBJS)
	@echo "In: $<"
	@echo "Out: $@"
	@mkdir -p $$(dirname $@)  # Create the directory structure in BUILD_DIR
	@$(CC) $(CFLAGS) $(TESTFLAGS) $< $(OBJS) -o $@

clean-test:
	@for testfile in $(UNIT_TEST_BINS); do \
		echo "Removing $$testfile";     \
		rm -f "$$testfile";             \
	done
	@for testfile in $(E2E_TEST_BINS); do \
		echo "Removing $$testfile";		  \
		rm -f "$$testfile";				  \
	done
