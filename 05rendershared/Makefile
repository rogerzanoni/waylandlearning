WAYLAND_FLAGS = `pkg-config wayland-client --cflags --libs`

OS_CREATE_ANONYMOUS_FILE_FILES=os-create-anonymous-file.c os-create-anonymous-file.h

APPNAME = rendershared

CFLAGS ?= -std=c11 -Wall -ggdb3

all: main

main: main.c $(OS_CREATE_ANONYMOUS_FILE_FILES)
	$(CC) $(CFLAGS) $(OS_CREATE_ANONYMOUS_FILE_FILES) main.c -o $(APPNAME) $(WAYLAND_FLAGS)
