# Compiler settings
C_COMPILER = clang
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

# Build target
$(TARGET): $(SOURCES)
	$(C_COMPILER) $(CFLAGS) $(INC_FLAGS) $(LIB_FLAGS) $(PKGCONFIG) $(SOURCES) -o $(BUILD)/$(TARGET)

# Check target to test initialization
check: $(TARGET)
	$(BUILD)/$(TARGET) & \
	APP_PID=$$!; \
	sleep 1; \
	kill -TERM $$APP_PID; \
	wait $$APP_PID || exit $$?

# Clean built files
clean:
	rm -rf $(BUILD)

# Phony targets
.PHONY: all clean
