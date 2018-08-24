WAYLAND_FLAGS = `pkg-config wayland-client --cflags --libs`

all: simple

simple:	main.c
	$(CC) $(CFLAGS) main.c -o simple $(WAYLAND_FLAGS)
