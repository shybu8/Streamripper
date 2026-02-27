.PHONY: clean

CC = gcc
APP = streamripper

UI_FILES = $(wildcard data/*.ui)

RES_XML = data/resources.gresource.xml
RES_C = src/resources.c
RES_H = src/resources.h

SRC = src/main.c src/clip-row.c src/my-window.c $(RES_C) $(RES_H)

all: $(SRC)
	$(CC) \
		$$(pkg-config --cflags --libs gtk4) \
		-Wall -Wextra -Wpedantic \
		-g \
		-o $(APP) \
		$(SRC)

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
	./$(APP)
