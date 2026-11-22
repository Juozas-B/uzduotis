CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude
LDFLAGS = -lmosquitto -lcjson -lm -pthread
SRC = $(wildcard src/*.c)
TARGET = iot_monitor
OBJ = $(SRC:%.c=build/%.o)
TEST_TARGET = build/test_parser

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): tests/test_parser.c src/telemetry_parser.c include/telemetry_parser.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) tests/test_parser.c src/telemetry_parser.c -o $@ -lcjson -lm

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf build $(TARGET)

.PHONY: all test clean
