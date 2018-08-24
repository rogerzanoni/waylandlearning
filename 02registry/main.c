#include <stdio.h>
#include <stdlib.h>
#include <wayland-client.h>

static void registry_add(void *, struct wl_registry *, uint32_t, const char *, uint32_t);
static void registry_remove(void *, struct wl_registry *, uint32_t);

int main()
{
    struct wl_display *display = wl_display_connect(NULL);

    if (!display) {
        perror("Error connecting to the wayland server");
        return EXIT_FAILURE;
    }

    printf("Connected to the wayland server.\n");

    struct wl_registry *registry = wl_display_get_registry(display);

    if (!registry) {
        perror("Error getting registry.\n");
        return EXIT_FAILURE;
    }

    printf("Registry created.\n");


    struct wl_registry_listener listener = {
        .global = registry_add,
        .global_remove = registry_remove
    };

    wl_registry_add_listener(registry, &listener, NULL);
    wl_display_dispatch(display);

    wl_registry_destroy(registry);
    wl_display_disconnect(display);

    return EXIT_SUCCESS;
}

static void registry_add(void *data,
        struct wl_registry *registry,
        uint32_t name,
        const char *interface,
        uint32_t version)
{
    printf("[registry_add] ID(name): %"PRIu32" Interface: %s\n", name, interface);
}

static void registry_remove(void *data,
        struct wl_registry *registry,
        uint32_t name)
{
    printf("[registry_remove] ID(name): %"PRIu32"\n", name);
}
