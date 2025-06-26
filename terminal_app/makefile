BIN_DIR := bin
TARGET := $(BIN_DIR)/config_esp
SRC := src/terminal_app.c

all: $(BIN_DIR)
	gcc -o ${TARGET} ${SRC}

${BIN_DIR}:
	mkdir -p ${BIN_DIR}

clean:
	rm -f ${TARGET}