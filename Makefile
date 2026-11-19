CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude
LDFLAGS = -lmosquitto -lcjson -lm
SRC = $(wildcard src/*.c)
TARGET = iot_monitor
OBJ = $(SRC:%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean