PROJECT := echo_glass
SRC := $(wildcard src/*.cpp)
BUILD_DIR := build
NATIVE_TARGET := $(BUILD_DIR)/$(PROJECT)
WEB_DIR := $(BUILD_DIR)/web
WEB_TARGET := $(WEB_DIR)/$(PROJECT).html

CXX ?= g++
EMXX ?= em++

CPPFLAGS := -Iinclude
CXXFLAGS := -std=c++20 -Wall -Wextra -Wpedantic -O2

RAYLIB_CFLAGS ?= $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS ?= $(shell pkg-config --libs raylib 2>/dev/null)
ifeq ($(strip $(RAYLIB_LIBS)),)
RAYLIB_LIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
endif

RAYLIB_SRC ?= $(HOME)/dev/raylib/src
RAYLIB_WEB ?= $(RAYLIB_SRC)/libraylib.web.a
WEB_FLAGS := -DPLATFORM_WEB -I$(RAYLIB_SRC) \
			 -sGROWABLE_ARRAYBUFFERS=0 \
             -sUSE_GLFW=3 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 \
             -sINITIAL_MEMORY=67108864 -sSTACK_SIZE=1048576 \
             --shell-file web/shell.html

.PHONY: all native web raylib-web run run-web clean help

all: native

native: $(NATIVE_TARGET)

$(NATIVE_TARGET): $(SRC) include/game.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(RAYLIB_CFLAGS) $(CXXFLAGS) $(SRC) -o $@ $(RAYLIB_LIBS)

web: $(WEB_TARGET)

$(WEB_TARGET): $(SRC) include/game.hpp web/shell.html $(RAYLIB_WEB) | $(WEB_DIR)
	$(EMXX) $(CPPFLAGS) $(CXXFLAGS) $(WEB_FLAGS) $(SRC) $(RAYLIB_WEB) -o $@

raylib-web:
	emmake $(MAKE) -C $(RAYLIB_SRC) PLATFORM=PLATFORM_WEB RAYLIB_LIBTYPE=STATIC

$(BUILD_DIR):
	mkdir -p $@

$(WEB_DIR):
	mkdir -p $@

run: native
	./$(NATIVE_TARGET)

run-web: web
	emrun --no_browser --port 8080 $(WEB_TARGET)

clean:
	rm -rf $(BUILD_DIR)

help:
	@printf '%s\n' \
	  'make native      Build Linux desktop version' \
	  'make run         Build and run desktop version' \
	  'make raylib-web  Compile raylib for WebAssembly' \
	  'make web         Build build/web/echo_glass.html' \
	  'make run-web     Serve it on http://localhost:8080' \
	  'make clean       Remove build output'
