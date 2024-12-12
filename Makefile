# Compiler settings
CC = cc
CFLAGS = -Iexternal/cglm/include
CGLM_LIB = external/cglm/build/libcglm.a
PKGCONFIG = `pkg-config --cflags --libs sdl3`

# File names
TARGET = bin/program
SOURCES = src/main.c

# Default target
all: $(TARGET)

# Linking the program
$(TARGET): $(SOURCES)
	$(CC) $(SOURCES) $(CFLAGS) $(CGLM_LIB) $(PKGCONFIG) -o $(TARGET)

# Clean built files
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
