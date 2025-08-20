include config.make

CFLAGS += -Wall -Wextra -Werror -Wunused -pedantic -std=c11

SOURCES := $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS := $(SOURCES:$(SOURCE_DIR)/%.c=$(BUILD_DIR)/%.o)

TEST_SOURCES := $(wildcard $(TEST_SOURCE_DIR)/*.c)
TEST_OBJECTS := $(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=$(TEST_BUILD_DIR)/%.o)

# Command-line flag to silence nested \$(MAKE).
$(VERBOSE)MAKESILENT := -s

##@ Default target
.PHONY: all
all: build ## Clean and build the library

##@ Build commands
.PHONY: clean build
build: $(LIBRARY_PATH) ## Clean and build the library

.PHONY: bear
bear: ## Generate compile_commands.json with BEAR
	bear -- $(MAKE) $(MAKESILENT) clean build

##@ Test commands
.PHONY: test
test: $(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=test_%) ## Run all tests

.PHONY: clean build_test
build_test: $(TEST_BUILD_DIR)/*.o ## Clean and build tests

$(LIBRARY_PATH): $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CLIBRARIES) -o $(LIBRARY_PATH) $(OBJECTS)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CINCLUDES) -o $@ -c $<

$(TEST_BUILD_DIR)/%.o: $(TEST_SOURCE_DIR)/%.c $(LIBRARY_PATH) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) $(CINCLUDES) -o $@ $< $(LIBRARY_PATH)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(TEST_BUILD_DIR):
	@mkdir -p $(TEST_BUILD_DIR)

$(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=build_test_%): build_test_%: $(TEST_BUILD_DIR)/%.o

$(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.c=test_%): test_%: $(TEST_BUILD_DIR)/%.o
	@echo "=== Run test: $* ==="
	-$^
# -$(MEMCHECK) $(MEMCHECK_ARGS) $^

##@ Clean commands
.PHONY: clean
clean: ## Clean built artifacts
	@rm -rf $(BUILD_DIR)

##@ Help commands
.PHONY: help
help: ## Display this help
	@awk 'BEGIN {FS = ":.*##";                                               \
		printf "Usage: make \033[36m<target>\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ \
		{ printf "  \033[36m%-10s\033[0m %s\n", $$1, $$2 } /^##@/            \
		{ printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } '                  \
		$(MAKEFILE_LIST)

# Suppress display of executed commands.
$(VERBOSE).SILENT:
