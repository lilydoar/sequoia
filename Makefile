# Compiler settings
CC = clang
CFLAGS = -Wall -Wextra -Wpedantic
INC_FLAGS = -Iexternal/cglm/include -Iexternal/stb
LIB_FLAGS = external/cglm/build/libcglm.a `pkg-config --cflags --libs sdl3`

# File names
TARGET = build/bin/program
SOURCES = src/main.c

# Ensure the bin directory exists
$(shell mkdir -p build/bin)

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INC_FLAGS) $(LIB_FLAGS) $(PKGCONFIG) $(SOURCES) -o $(TARGET)

# Clean built files
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
