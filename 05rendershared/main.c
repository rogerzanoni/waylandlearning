#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include <wayland-client.h>

#include "os-create-anonymous-file.h"

struct display {
    struct wl_display *wayland_display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shell *shell;
    struct wl_shm *shm;
};

struct buffer {
    struct wl_buffer *buffer;
    void *shm_data;
    bool buffer_busy;
};

struct window {
    int width;
    int height;
    struct display *display;
    struct wl_surface *surface;
    struct wl_shell_surface *shell_surface;
    struct wl_callback *draw_callback;
    struct buffer *buffer;
};

// registry listener callbacks
static void registry_add(void *, struct wl_registry *, uint32_t, const char *, uint32_t);
static void registry_remove(void *, struct wl_registry *, uint32_t);

// buffer listener callback
static void buffer_release(void *, struct wl_buffer *);

// frame listener callback
static void redraw(void *, struct wl_callback *, uint32_t);

// data helpers
static struct display *create_display();
static void destroy_display(struct display *);

static struct buffer *create_buffer();
static void destroy_buffer(struct buffer *);

static struct window *create_window(struct display *, int width, int height);
static void destroy_window(struct window *);

// buffer/painting helpers
static void paint_pixels(void *, int, int, int, uint32_t);
static void paint_loop(struct window *);

// listerners
static struct wl_buffer_listener buffer_listener = {
    buffer_release
};

static struct wl_callback_listener frame_listener = {
    redraw
};

static struct wl_registry_listener registry_listener = {
    .global = registry_add,
    .global_remove = registry_remove
};

int main()
{
    struct display *display = create_display();

    if (!display->wayland_display) {
        perror("Error connecting to the wayland server");
        return EXIT_FAILURE;
    }

    printf("Connected to the wayland server.\n");

    if (!display->registry) {
        perror("Error getting registry.\n");
        destroy_display(display);
        return EXIT_FAILURE;
    }
    printf("Registry created.\n");

    wl_registry_add_listener(display->registry, &registry_listener, display);
    wl_display_roundtrip(display->wayland_display);

    if (!display->compositor) {
        perror("Error binding to compositor interface.\n");
        destroy_display(display);
        return EXIT_FAILURE;
    }

    if (!display->shell) {
        perror("Error binding to shell interface.\n");
        destroy_display(display);
        return EXIT_FAILURE;
    }

    if (!display->shm) {
        perror("Error binding to shm interface.\n");
        destroy_display(display);
        return EXIT_FAILURE;
    }

    struct window *window = create_window(display, 300, 300);

    if (!window->surface) {
        perror("Error creating surface.\n");
        destroy_window(window);
        return EXIT_FAILURE;
    }

    printf("Surface created.\n");

    if (!window->shell_surface) {
        perror("Error creating shell surface.\n");
        destroy_window(window);
        return EXIT_FAILURE;
    }

    printf("Shell surface created.\n");

    if (window->buffer && !window->buffer->buffer) {
        perror("Error creating buffer.\n");
        destroy_window(window);
        return EXIT_FAILURE;
    }

    paint_loop(window);

    destroy_window(window);

    return EXIT_SUCCESS;
}

static void paint_loop(struct window *window)
{
    struct display *display = window->display;

    wl_display_roundtrip(display->wayland_display);

    wl_shell_surface_set_toplevel(window->shell_surface);

    wl_surface_damage(window->surface, 0, 0, window->width, window->height);

    redraw(window, NULL, 0);

    int ret = 0;
    while (ret != -1) {
        ret = wl_display_dispatch(display->wayland_display);
    }
}

static struct display *create_display()
{
    struct display *display = calloc(1, sizeof(struct display));
    display->wayland_display = wl_display_connect(NULL);

    if (display->wayland_display) {
        display->registry = wl_display_get_registry(display->wayland_display);
    }

    return display;
}

static void destroy_display(struct display *display)
{
    if (display->registry) {
        wl_registry_destroy(display->registry);
    }

    if (display->compositor) {
        wl_compositor_destroy(display->compositor);
    }

    if (display->wayland_display) {
        wl_display_disconnect(display->wayland_display);
    }

    free(display);
}

static struct buffer *create_buffer(struct window *window)
{
    struct buffer *buffer = calloc(1, sizeof(struct buffer));
    struct wl_shm_pool *pool;
    int fd, size, stride;
    void *data;

    stride = window->width * 4;
    size = stride * window->height;

    fd = os_create_anonymous_file(size);
    if (fd < 0) {
        fprintf(stderr, "creating a buffer file for %d B failed: %m\n", size);
        return buffer;
    }

    data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "mmap failed: %m\n");
        close(fd);
        return buffer;
    }

    pool = wl_shm_create_pool(window->display->shm, fd, size);
    buffer->buffer = wl_shm_pool_create_buffer(pool, 0, window->width, window->height,
                                               stride, WL_SHM_FORMAT_XRGB8888);

    wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);
    wl_shm_pool_destroy(pool);

    close(fd);

    buffer->shm_data = data;

    memset(buffer->shm_data, 0xff, window->width * window->height * 4);

    return buffer;
}

static void destroy_buffer(struct buffer *buffer)
{
    if (buffer->buffer) {
        wl_buffer_destroy(buffer->buffer);
    }

    free(buffer);
}

static struct window *create_window(struct display *display, int width, int height)
{
    struct window *window = calloc(1, sizeof(struct window));
    window->width = width;
    window->height = height;
    window->display = display;

    window->surface = wl_compositor_create_surface(display->compositor);

    if (window->surface) {
        window->shell_surface = wl_shell_get_shell_surface(display->shell, window->surface);
    }

    window->buffer = create_buffer(window);

    return window;
}

static void destroy_window(struct window *window)
{
    if (window->buffer) {
        destroy_buffer(window->buffer);
    }

    if (window->draw_callback) {
        wl_callback_destroy(window->draw_callback);
    }

    if (window->shell_surface) {
        wl_shell_surface_destroy(window->shell_surface);
    }

    if (window->surface) {
        wl_surface_destroy(window->surface);
    }

    if (window->display) {
        destroy_display(window->display);
    }
}

static void registry_add(void *data,
        struct wl_registry *registry,
        uint32_t name,
        const char *interface,
        uint32_t version)
{
    struct display *display = data;

    printf("[registry_add] ID(name): %"PRIu32" Interface: %s\n", name, interface);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        display->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
        printf("    [registry_add] compositor bound to %s interface\n", interface);
    } if (strcmp(interface, wl_shell_interface.name) == 0) {
        display->shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
        printf("    [registry_add] shell bound to %s interface\n", interface);
    } if (strcmp(interface, wl_shm_interface.name) == 0) {
        display->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
        printf("    [registry_add] shm bound to %s interface\n", interface);
    }
}

static void registry_remove(void *data,
        struct wl_registry *registry,
        uint32_t name)
{
    printf("[registry_remove] ID(name): %"PRIu32"\n", name);
}

static void buffer_release(void *data, struct wl_buffer *buffer)
{
    struct buffer *buf = data;
    buf->buffer_busy = false;
}

static void redraw(void *data, struct wl_callback *callback, uint32_t time)
{
    struct window *window = data;
    struct buffer *buffer = window->buffer;

    if (buffer->buffer_busy) {
        return;
    }

    paint_pixels(buffer->shm_data, 20, window->width, window->height, time);

    // set the buffer as content of the surface
    wl_surface_attach(window->surface, buffer->buffer, 0, 0);

    // tells compositor which area of the surface should be repainted
    wl_surface_damage(window->surface, 20, 20, window->width - 40, window->height - 40);

    if (callback) {
        wl_callback_destroy(callback);
    }

    // requests to be notified when it should start drawing again
    window->draw_callback = wl_surface_frame(window->surface);
    wl_callback_add_listener(window->draw_callback, &frame_listener, window);

    // requests the compositor to update buffer state
    wl_surface_commit(window->surface);

    buffer->buffer_busy = true;
}

static void paint_pixels(void *image, int padding, int width, int height, uint32_t time)
{
    const int halfh = padding + (height - padding * 2) / 2;
    const int halfw = padding + (width  - padding * 2) / 2;
    int ir, or;
    uint32_t *pixel = image;
    int y;

    /* squared radii thresholds */
    or = (halfw < halfh ? halfw : halfh) - 8;
    ir = or - 32;
    or *= or;
    ir *= ir;

    pixel += padding * width;
    for (y = padding; y < height - padding; y++) {
        int x;
        int y2 = (y - halfh) * (y - halfh);

        pixel += padding;
        for (x = padding; x < width - padding; x++) {
            uint32_t v;

            /* squared distance from center */
            int r2 = (x - halfw) * (x - halfw) + y2;

            if (r2 < ir)
                v = (r2 / 32 + time / 64) * 0x0080401;
            else if (r2 < or)
                v = (y + time / 32) * 0x0080401;
            else
                v = (x + time / 16) * 0x0080401;
            v &= 0x00ffffff;

            /* cross if compositor uses X from XRGB as alpha */
            if (abs(x - y) > 6 && abs(x + y - height) > 6)
                v |= 0xff000000;

            *pixel++ = v;
        }

        pixel += padding;
    }
}
