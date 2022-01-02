TARGETS := $(notdir $(patsubst %.c,%,$(wildcard src/*.c)))
FLAGS = $(shell cat compile_flags.txt | tr '\n' ' ')
SRC = src/helpers.c
SRC += $(wildcard deps/*/*.c)
BUILD_DIR = build

all: $(TARGETS)

.PHONY: $(TARGETS)
$(TARGETS): 
	mkdir -p $(BUILD_DIR)
	clang -o $(BUILD_DIR)/$@ src/$@.c $(SRC) $(FLAGS)
