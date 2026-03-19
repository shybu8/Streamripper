.PHONY: clean

CC = gcc
APP = streamripper

UI_FILES = $(wildcard data/*.ui)

RES_XML = data/resources.gresource.xml
RES_C = src/resources.c
RES_H = src/resources.h

SRC = src/main.c src/clip-row.c src/my-window.c src/ffmpeg.c $(RES_C) $(RES_H)
CFLAGS = -g -Wall -Wextra -Wpedantic $(shell pkg-config --cflags gtk4)
LDFLAGS = $(shell pkg-config --libs gtk4)

all: $(SRC)
	$(CC) \
		-o $(APP) \
		$(SRC) \
		${CFLAGS} \
		${LDFLAGS}

$(RES_C): $(RES_XML) $(UI_FILES)
	glib-compile-resources $(RES_XML) \
		--sourcedir=data \
		--c-name myres \
		--generate-source \
		--target=$@

$(RES_H): $(RES_XML) $(UI_FILES)
	glib-compile-resources $(RES_XML) \
		--sourcedir=data \
		--c-name myres \
		--generate-header \
		--target=$@

clean:
	rm -f $(APP) $(RES_C) $(RES_H)

run: all
	G_MESSAGES_DEBUG=all ./$(APP)
