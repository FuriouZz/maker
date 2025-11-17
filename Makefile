include config.make

CFLAGS += -Wall -Wextra -Werror -Wunused -pedantic -std=c99 -DMK_DEBUG

SOURCES := $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS := $(SOURCES:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)

TEST_SOURCES := $(wildcard $(TEST_SOURCE_DIR)/*.c)
TEST_OBJECTS := $(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=$(TEST_BUILD_DIR)/%.o)

# Command-line flag to silence nested \$(MAKE).
$(VERBOSE)MAKESILENT := -s

##@ Help commands
help: ## Display this help
	@awk 'BEGIN {FS = ":.*##";                                               \
		printf "Usage: make \033[36m<target>\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ \
		{ printf "  \033[36m%-10s\033[0m %s\n", $$1, $$2 } /^##@/            \
		{ printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } '                  \
		$(MAKEFILE_LIST)

##@ Build commands

build: $(LIBRARY_PATH) ## Clean and build the library

clean: ## Clean built artifacts
	@rm -rf $(BUILD_DIR)

create_target_dir:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(TEST_BUILD_DIR)

bear: ## Generate compile_commands.json with BEAR
	bear -- $(MAKE) $(MAKESILENT) clean build

cscope: ## Navigate through files
	echo "$(wildcard $(SOURCE_DIR)/*)" > cscope.files
	cscope -X -i cscope.files

##@ Library commands

$(LIBRARY_PATH): $(OBJECTS) | create_target_dir
	$(CC) $(CFLAGS) $(LDFLAGS) $(CLIBRARIES) -o $(LIBRARY_PATH) $(OBJECTS)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c | create_target_dir
	$(CC) $(CFLAGS) $(CINCLUDES) -o $@ -c $<

##@ Test commands

test: $(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=test_%) ## Run all tests

build_test: $(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=build_test_%) ## Clean and build tests

$(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=build_test_%): build_test_%: $(LIBRARY_PATH) | create_target_dir
	$(CC) $(CFLAGS) $(CINCLUDES) -o $(TEST_BUILD_DIR)/$*.o $(TEST_SOURCE_DIR)/$*.c $(LIBRARY_PATH)

$(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=test_%): test_%: build_test_%
	@echo "=== Run test: $* ==="
	-$(TEST_BUILD_DIR)/$*.o
# -$(MEMCHECK) $(MEMCHECK_ARGS) $^

.PHONY: help clean build test build_test bear cscope create_target_dir

# Suppress display of executed commands.
$(VERBOSE).SILENT:
