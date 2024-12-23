# Compiler settings
CC = clang
CFLAGS = -Wall -Wextra -Wpedantic
INC_FLAGS = -Iexternal/cglm/include -Iexternal/stb -Isrc/ -Iexternal/json-c
LIB_FLAGS = external/cglm/build/libcglm.a `pkg-config --cflags --libs sdl3`

# File names
TARGET = sequoia
BUILD = build/bin
SOURCES = src/main.c $(wildcard src/gen/**/*.c)

# Ensure the bin directory exists
$(shell mkdir -p build/bin)

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INC_FLAGS) $(LIB_FLAGS) $(PKGCONFIG) $(SOURCES) -o $(BUILD)/$(TARGET)

# Clean built files
clean:
	rm -rf $(BUILD)

# Phony targets
.PHONY: all clean
