#include <stdio.h>
#include <stdlib.h>
#include <wayland-client.h>

int main()
{
    struct wl_display *display = wl_display_connect(NULL);

    if (!display) {
        perror("Error connecting to the wayland server");
        return EXIT_FAILURE;
    }

    printf("Connected to the wayland server.\n");

    wl_display_disconnect(display);

    return EXIT_SUCCESS;
}
