# Invoke from the repository root: make -f tests/fuzz/Makefile
CC = clang
ENGINE ?= libfuzzer
OUT ?= build/fuzz/$(ENGINE)
BASE = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -g -O1 -fno-omit-frame-pointer
INCLUDES = -Icomponents/streaming/include -Icomponents/configuration/include -Imanaged_components/espressif__cjson/cJSON
SOURCES = tests/fuzz/parsers.c components/streaming/streamoo_playlist_resolver.c components/streaming/streamoo_hls.c components/streaming/streamoo_icy.c components/configuration/streamoo_config_model.c managed_components/espressif__cjson/cJSON/cJSON.c
ifeq ($(ENGINE),libfuzzer)
FLAGS = -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=all
else ifeq ($(ENGINE),afl)
FLAGS = -DSTREAMOO_FUZZ_STANDALONE -fsanitize=address,undefined -fno-sanitize-recover=all
else ifeq ($(ENGINE),replay)
FLAGS = -DSTREAMOO_FUZZ_STANDALONE -fsanitize=address,undefined -fno-sanitize-recover=all
else
$(error Unknown ENGINE)
endif
.PHONY: all
all: $(OUT)/playlist $(OUT)/hls $(OUT)/icy $(OUT)/stations $(OUT)/wifi
$(OUT):
	mkdir -p $@
$(OUT)/playlist: TARGET = 1
$(OUT)/hls: TARGET = 2
$(OUT)/icy: TARGET = 3
$(OUT)/stations: TARGET = 4
$(OUT)/wifi: TARGET = 5
$(OUT)/%: $(SOURCES) $(wildcard components/streaming/include/*.h components/configuration/include/*.h) tests/fuzz/Makefile | $(OUT)
	$(CC) $(BASE) $(FLAGS) $(INCLUDES) -DSTREAMOO_FUZZ_TARGET=$(TARGET) $(SOURCES) -lm -o $@
