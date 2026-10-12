# ARMv6-M Simulator (C)
# No external dependencies beyond libc

CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99
LDFLAGS =

SRC_DIR = src
OBJ_DIR = obj
TARGET  = armv6m-sim

SRCS = $(SRC_DIR)/armv6m.c    \
       $(SRC_DIR)/armv6m_decode.c  \
       $(SRC_DIR)/armv6m_execute.c \
       $(SRC_DIR)/elf_load.c       \
       $(SRC_DIR)/main.c

OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean
