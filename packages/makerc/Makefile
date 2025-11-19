PROFILE ?= debug
CFLAGS ?= -g

ifeq ($(PROFILE),release)
CFLAGS += -O2
else
CFLAGS += -O0 -fsanitize=address
endif

CLIBS = \
	-Lvendors/ffmpeg/build/lib \
	-Wl,-rpath,/vendors/ffmpeg/build/lib \
	-lavutil -lswresample -lavcodec -lavformat -lswscale -lavfilter -lavdevice

BUILD_CFLAGS = $(CFLAGS) \
	-std=c99 -Wall -Wextra -Werror -Wunused -pedantic -fPIC \
	-Isrc/include \
	-Ivendors/ffmpeg/build/include

MAKER_TARGET_DIR = target/$(PROFILE)
MAKER_TARGET_NAME = maker

MAKER_HEADERS = \
	src/include/maker.h

MAKER_LOCAL_HEADERS = \
	src/core/maker_internal.h

MAKER_SOURCES = \
	src/core/clock.c \
	src/core/decoder.c \
	src/core/demuxer.c \
	src/core/error.c \
	src/core/frame_queue.c \
	src/core/video_frame.c \
	src/core/media.c \
	src/core/packet_queue.c \
	src/core/pixel_format.c \
	src/core/thread.c \
	src/core/thread_pool.c \
	src/core/util.c \
	src/core/video_converter.c \
	src/core/video_decoder.c

MAKER_TARGET_OBJECTS = $(patsubst src/%.c,$(MAKER_TARGET_DIR)/objects/%.o,$(MAKER_SOURCES))

MAKER_TEST_SOURCES = \
	tests/media.c

MAKER_TEST_TARGETS = $(patsubst tests/%.c,$(MAKER_TARGET_DIR)/tests/%.o,$(MAKER_TEST_SOURCES))


# Check OS
UNAME?=$(shell uname -s)
ifeq ($(UNAME), Darwin)
	MAKER_NAME = libmaker.dylib
endif

MAKER_TARGET = $(MAKER_TARGET_DIR)/$(MAKER_NAME)

# Command-line flag to silence nested \$(MAKE).
$(VERBOSE)MAKESILENT = -s

all: help

$(MAKER_TARGET_DIR)/objects/%.o: src/%.c Makefile $(MAKER_HEADERS) $(MAKER_LOCAL_HEADERS)
	mkdir -p $(shell dirname $@)
	$(CC) $(BUILD_CFLAGS) -o $@ -c $<

$(MAKER_TARGET): $(MAKER_TARGET_OBJECTS)
	mkdir -p $(MAKER_TARGET_DIR)
	$(CC) $(BUILD_CFLAGS) -shared -o $(shell pwd)/$@ $^ $(CLIBS)

$(MAKER_TARGET_DIR)/tests/%.o: tests/%.c $(MAKER_TARGET)
	mkdir -p $(MAKER_TARGET_DIR)/tests
	mkdir -p tmp
	$(CC) $(BUILD_CFLAGS) -L$(MAKER_TARGET_DIR) -l$(MAKER_TARGET_NAME) -o $@ $^
# $(CC) $(BUILD_CFLAGS) -o $@ $^

##@ Help commands
help: ## Display this help
	@awk 'BEGIN {FS = ":.*##";                                               \
		printf "Usage: make \033[36m<target>\033[0m\n"} /^[a-zA-Z_-]+:.*?##/ \
		{ printf "  \033[36m%-10s\033[0m %s\n", $$1, $$2 } /^##@/            \
		{ printf "\n\033[1m%s\033[0m\n", substr($$0, 5) } '                  \
		$(MAKEFILE_LIST)

##@ Install commands
install: install-ffmpeg install-bindgen ## Install project dependencies

install-ffmpeg: ## Install ffmpeg (8.0)
	mkdir -p vendors
	rm -rf vendors/ffmpeg
	git clone \
		https://git.ffmpeg.org/ffmpeg.git \
		--depth=1 \
		--branch n8.0 \
		vendors/ffmpeg/sources
	cd vendors/ffmpeg/sources \
		&& ./configure \
			--prefix="$(shell pwd)/vendors/ffmpeg/build" \
			--disable-programs \
			--disable-static \
			--disable-doc \
			--enable-shared \
			--enable-pic \
			--enable-cross-compile \
			--enable-swscale \
			--enable-debug=2 \
	    && make \
	    && make install

install-bindgen: ## Install odin-c-bindgen
	mkdir -p vendors
	rm -rf vendors/odin-c-bindgen
	git clone \
		git@github.com:karl-zylinski/odin-c-bindgen.git \
		--depth=1 \
		--branch=807603709017926f9eaefaae8d4a8437b7a17a46
		vendors/odin-c-bindgen/sources
	mkdir -p vendors/odin-c-bindgen/build/bin
	cd vendors/odin-c-bindgen/sources \
    	&& odin build src -out:../build/bin/bindgen

##@ Build commands
build: $(MAKER_TARGET) ## Build marker.so

build-test: $(MAKER_TEST_TARGETS) ## Build all tests

##@ Test commands
test: $(MAKER_TARGET_DIR)/tests/$(TEST).o ## Run test given by TEST=testname
	@echo "==== Run $^ ===="
	$^

##@ Tools commands
bear: ## Generate compile_commands.json with BEAR
	bear -- $(MAKE) build -B VERBOSE=1

bindgen: build ## Generate odin bindings
	vendors/odin-c-bindgen/build/bin/bindgen bindgen.sjson
	cp $(MAKER_TARGET) bindings/maker/$(MAKER_NAME)

cscope: ## Navigate through files
	echo "$(wildcard $(SOURCE_DIR)/*)" > cscope.files
	cscope -X -i cscope.files

.PHONY: all build build-test test install install-bindgen install-ffmpeg bear cscope bindgen

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# https://gist.github.com/aont/75ba52c205ae76cc57b931a8a20f9fba
# https://renenyffenegger.ch/notes/development/languages/C-C-plus-plus/GCC/create-libraries/
