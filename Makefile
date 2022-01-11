TARGETS := $(notdir $(patsubst %.c,%,$(wildcard src/*.c)))
CFLAGS = $(shell cat compile_flags.txt | tr '\n' ' ')
SRC = src/helpers.c src/server-helpers.c
SRC += $(wildcard deps/*/*.c)
BUILD_DIR = build
PLATFORM := $(shell sh -c 'uname -s 2>/dev/null | tr 'a-z' 'A-Z'')

ifeq ($(PLATFORM),LINUX)
	CFLAGS += -lBlocksRuntime -ldispatch -lbsd -fsanitize=leak
endif

all: $(TARGETS)

.PHONY: $(TARGETS)
$(TARGETS): 
	mkdir -p $(BUILD_DIR)
	clang -o $(BUILD_DIR)/$@ src/$@.c $(SRC) $(CFLAGS)
