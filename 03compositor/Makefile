WAYLAND_FLAGS = `pkg-config wayland-client --cflags --libs`
APPNAME = compositor

all: main

main:	main.c
	$(CC) $(CFLAGS) main.c -o $(APPNAME) $(WAYLAND_FLAGS)
