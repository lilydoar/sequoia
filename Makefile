# Compiler settings
CC = cc
PKGCONFIG = `pkg-config --cflags --libs sdl3`

# File names
TARGET = program
SOURCES = src/main.c

# Default target
all: $(TARGET)

# Linking the program
$(TARGET): $(SOURCES)
	$(CC) $(SOURCES) $(PKGCONFIG) -o $(TARGET)

# Clean built files
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
