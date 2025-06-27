CFLAGS = -Wall -Wextra -std=c99

BIN_DIR := bin
TARGET := ${BIN_DIR}/config_esp

SRC := src/terminal_app.c src/util.c
OBJ := ${SRC:.c=.o}

all: ${BIN_DIR} | ${OBJ}
	gcc ${CFLAGS} -o ${TARGET} ${OBJ}
	rm -f ${OBJ}

%.o: %.c
	gcc ${CFLAGS} -c $< -o $@

${BIN_DIR}:
	mkdir -p ${BIN_DIR}

clean:
	rm -f ${TARGET}