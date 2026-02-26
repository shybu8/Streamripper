.PHONY: clean

COMPILER = gcc
NAME = streamripper
SRC = main.c

all: main.c
	${COMPILER} \
		$$(pkg-config --cflags gtk4) \
		$$(pkg-config --libs gtk4) \
		-Wall -Wextra -Wpedantic \
		-g \
		-o ${NAME} \
		${SRC}

clean:
	rm ${NAME}
