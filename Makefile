TARGETS := $(notdir $(patsubst %.c,%,$(wildcard src/*.c)))
ASM_TARGETS := $(notdir $(patsubst %.S,%,$(wildcard src/*.S)))
CFLAGS = $(shell cat compile_flags.txt | tr '\n' ' ')
SRC = src/helpers.c
SRC += $(wildcard deps/*/*.c)
BUILD_DIR = build
TEST_DIR = test
PLATFORM := $(shell sh -c 'uname -s 2>/dev/null | tr 'a-z' 'A-Z'')

ifeq ($(PLATFORM),LINUX)
	CFLAGS += -lBlocksRuntime -ldispatch -lbsd
else ifeq ($(PLATFORM),DARWIN)
	CFLAGS += -fsanitize=address
endif

all: $(TARGETS)

.PHONY: $(TARGETS)
$(TARGETS): test_setup
	mkdir -p $(BUILD_DIR)
	clang -o $(BUILD_DIR)/$@ src/$@.c $(SRC) $(CFLAGS)

# .PHONY: $(ASM_TARGETS)
# $(ASM_TARGETS): test_setup
# 	mkdir -p $(BUILD_DIR)
# 	as -o build/$@.o src/$@.S
# 	ld -o build/$@ build/$@.o -e _start -L /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -lSystem

test_setup:
	mkdir -p $(TEST_DIR)
