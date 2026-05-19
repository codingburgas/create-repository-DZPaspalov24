# GradeMaster - Cross-platform Makefile
#
# Quick start:
#   Linux / WSL:   sudo apt install libraylib-dev   (or build from source)
#   macOS:         brew install raylib
#   Windows MSYS2: pacman -S mingw-w64-x86_64-raylib
#
# Then:
#   make           # build into ./build/grademaster
#   make run       # build and launch
#   make clean
#
# If raylib is installed in a non-standard location, override CFLAGS / LDLIBS:
#   make CFLAGS_EXTRA="-I/opt/raylib/include" LDLIBS_EXTRA="-L/opt/raylib/lib"

CXX       ?= g++
CXXSTD    ?= -std=c++17
OPT       ?= -O2
WARN      ?= -Wall -Wextra -Wno-unused-parameter

SRC_DIR   := src
BUILD_DIR := build
SOURCES   := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS   := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Output binary name
TARGET    := grademaster

# Detect host OS
UNAME_S   := $(shell uname -s 2>/dev/null || echo Windows_NT)

ifeq ($(OS),Windows_NT)
    PLATFORM := windows
else ifeq ($(UNAME_S),Darwin)
    PLATFORM := macos
else
    PLATFORM := linux
endif

# Per-platform link flags ----------------------------------------------------
ifeq ($(PLATFORM),linux)
    LDLIBS    := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    EXEEXT    :=
endif
ifeq ($(PLATFORM),macos)
    LDLIBS    := -lraylib -framework OpenGL -framework Cocoa \
                 -framework IOKit -framework CoreVideo -framework CoreAudio
    EXEEXT    :=
endif
ifeq ($(PLATFORM),windows)
    LDLIBS    := -lraylib -lopengl32 -lgdi32 -lwinmm
    EXEEXT    := .exe
endif

CXXFLAGS  := $(CXXSTD) $(OPT) $(WARN) -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS $(CFLAGS_EXTRA)
LDFLAGS   := $(LDLIBS_EXTRA) $(LDLIBS)

OUT       := $(BUILD_DIR)/$(TARGET)$(EXEEXT)

.PHONY: all run clean help

all: $(OUT)

$(OUT): $(OBJECTS) | $(BUILD_DIR)
	@echo "  LINK   $@"
	@$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo
	@echo "Built $(OUT)"
	@echo "Run with:  ./$(OUT)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "  CXX    $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

run: $(OUT)
	@./$(OUT)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned."

help:
	@echo "GradeMaster build targets:"
	@echo "  make         build the application"
	@echo "  make run     build and run"
	@echo "  make clean   remove build artifacts"
	@echo
	@echo "Detected platform: $(PLATFORM)"
	@echo "Compiler:          $(CXX)"
