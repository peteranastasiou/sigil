#
# Usage: `make -j` to build
#        use `VERBOSE=1` to get stack traces
#        use `VERBOSE_GC=1` to get garbage collection traces 

.DEFAULT_GOAL := all

CC = g++


# TODO switch on cmakegoals debug
# Whether to build for debugging instead of release
DEBUG = 1

# Whether to enable garbage collection as often as possible
#DEBUG_STRESS_GC = 1

# Compilation flags
CFLAGS = -std=c++17 -W -Wall -Wextra -Werror -Wno-unused -Wconversion -MMD -MP -fno-exceptions
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -O0 -g
else
	CFLAGS += -DNDEBUG -O2 -flto
endif

# Linker flags
LDFLAGS = 
ifeq ($(DEBUG), 1)
	LDFLAGS += 
else
	LDFLAGS += -O2
endif

TARGET = bin/sigil

OBJECTS = $(patsubst src/%.cpp, build/%.o, $(wildcard src/*.cpp))
OBJECTS += $(patsubst src/inputstream/%.cpp, build/inputstream__%.o, $(wildcard src/inputstream/*.cpp))
DEPS = $(OBJECTS:.o=.d)

ifeq ($(OS), Windows_NT)
	MKDIR_BUILD = if not exist build md build
	MKDIR_BIN = if not exist bin md bin
	RMDIR = rd /s /q
else
	MKDIR_BUILD = mkdir -p build
	MKDIR_BIN = mkdir -p bin
	RMDIR = rm -rf
endif

# Defines
DEFINES = 

# Enable debug messages:
ifeq ($(VERBOSE), 1)
	DEFINES += -DDEBUG_TRACE_EXECUTION
endif

ifeq ($(VERBOSE_GC), 1)
	DEFINES += -DDEBUG_GC
endif

# Disable assert() calls:
ifeq ($(DEBUG), 0)
	DEFINES += -DNDEBUG
endif

ifeq ($(DEBUG_STRESS_GC), 1)
	DEFINES += -DDEBUG_STRESS_GC
endif

LIBS = -lreadline

all: $(TARGET)

# Link
$(TARGET): $(OBJECTS)
	@$(MKDIR_BIN)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)

# Compile sources
build/%.o: src/%.cpp
	@$(MKDIR_BUILD)
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

build/inputstream__%.o: src/inputstream/%.cpp
	@$(MKDIR_BUILD)
	$(CC) $(CFLAGS) $(DEFINES) -c $< -o $@

.PHONY: clean

clean:
	$(RMDIR) build
	$(RMDIR) bin

-include $(DEPS)
