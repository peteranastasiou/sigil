
.DEFAULT_GOAL := all

CC = g++

# Whether to build for debugging instead of release
DEBUG = 0

# Compilation flags
CFLAGS = -W -Wall -Wextra -Werror -Wno-unused -Wconversion -MMD -MP -fno-exceptions
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

TARGET = bin/pond

OBJECTS = $(patsubst src/%.cpp, build/%.o, $(wildcard src/*.cpp))
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

all: $(TARGET)

# Link
$(TARGET): $(OBJECTS)
	@$(MKDIR_BIN)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

# Compile sources
build/%.o: src/%.cpp
	@$(MKDIR_BUILD)
	$(CC) -c $< -o $@ $(CFLAGS)

.PHONY: clean

clean:
	$(RMDIR) build
	$(RMDIR) bin

-include $(DEPS)