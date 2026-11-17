CC = gcc
CFLAGS = -Wall -Wextra -Werror
LDFLAGS = -lmosquitto -lcjson
SRC = main.c
TARGET = iot_monitor
OBJ = $(SRC:%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean