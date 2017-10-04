/* raster.c
 *
 * Triangle rasteration viewer.
 *
 * Author:  Alastair Hughes
 * Contact: hobbitalastair at yandex dot com
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include <libnsfb.h>
#include <libnsfb_event.h>
#include <libnsfb_plot.h>

#define SURFACE_TYPE NSFB_SURFACE_SDL /* Default surface type */
#define DEPTH_TYPE unsigned int /* Depth buffer type */


typedef struct {
    /* Visible display width and height */
    int width;
    int height;

    /* Display format, buffer and stride */
    enum nsfb_format_e format;
    uint8_t* buf;
    int stride;

    /* Depth buffer (width * height in size) */
    DEPTH_TYPE* depth_buf;

    /* Shared libnsfb context */
    nsfb_t *nsfb;
} display_t;

typedef struct {
    /* Temporary placeholder contents struct */
    // TODO: Implement.

    nsfb_colour_t background;
} content_t;


void initialise_content(char* name, content_t *c, int argc, char** argv) {
    /* Initialise the given contents struct.
     *
     * This exits on failure.
     */

    // TODO: Implement.
    c->background = 0xFFFFFF;
}

void initialise_display(char* name, display_t *d) {
    /* Initialise the given display struct.
     *
     * This exits on failure.
     */

    d->nsfb = nsfb_new(SURFACE_TYPE);
    if (d->nsfb == NULL || nsfb_init(d->nsfb) != 0) {
        fprintf(stderr, "%s: failed to initialise libnsfb\n", name);
        exit(EXIT_FAILURE);
    }

    if (nsfb_get_geometry(d->nsfb, &d->width, &d->height, &d->format) != 0) {
        fprintf(stderr, "%s: failed to get window geometry\n", name);
        exit(EXIT_FAILURE);
    }
    if (nsfb_get_buffer(d->nsfb, &d->buf, &d->stride) != 0) {
        fprintf(stderr, "%s: failed to get window buffer\n", name);
        exit(EXIT_FAILURE);
    }

    d->depth_buf = malloc(sizeof(DEPTH_TYPE) * d->width * d->height);
    if (d->depth_buf == NULL) {
        fprintf(stderr, "%s: failed to allocate depth buffer\n", name);
        exit(EXIT_FAILURE);
    }
}

void resize_display(char* name, display_t *d, int width, int height) {
    /* Resize the display, updating the display struct as required.
     *
     * This (may) call exit on failure.
     */

    int orig_width = d->width;
    int orig_height = d->height;

    if (nsfb_set_geometry(d->nsfb, width, height, NSFB_FMT_ANY) != 0) {
        fprintf(stderr, "%s: failed to resize window region\n", name);
        return;
    }

    if (nsfb_get_geometry(d->nsfb, &d->width, &d->height, &d->format) != 0) {
        fprintf(stderr, "%s: failed to get window geometry\n", name);
        exit(EXIT_FAILURE);
    }
    if (nsfb_get_buffer(d->nsfb, &d->buf, &d->stride) != 0) {
        fprintf(stderr, "%s: failed to get window buffer\n", name);
        exit(EXIT_FAILURE);
    }
    
    if (orig_width * orig_height < d->width * d->height) {
        /* Allocate a new buffer if the window region has grown */
        free(d->depth_buf);
        d->depth_buf = malloc(sizeof(DEPTH_TYPE) * d->width * d->height);
        if (d->depth_buf == NULL) {
            fprintf(stderr, "%s: failed to allocate new depth buffer\n", name);
            exit(EXIT_FAILURE);
        }
    }
}

void render(char* name, display_t *d, content_t *c) {
    /* Render the visible screen content */

    nsfb_bbox_t display_box = {0, 0, d->width, d->height};
    if (nsfb_claim(d->nsfb, &display_box) != 0) {
        fprintf(stderr, "%s: failed to claim window region\n", name);
        return;
    }

    nsfb_plot_clg(d->nsfb, c->background);

    // TODO: Render triangles here!

    if (nsfb_update(d->nsfb, &display_box) != 0) {
        fprintf(stderr, "%s: failed to update window\n", name);
    }
}


int main(int argc, char** argv) {
    char* name = __FILE__;
    if (argc > 0) name = argv[0];
    if (argc != 1) {
        fprintf(stderr, "usage: %s <triangles> ...\n", name);
        exit(EINVAL);
    }

    content_t content;
    initialise_content(name, &content, argc - 1, &(argv[1]));

    display_t d;
    initialise_display(name, &d);

    render(name, &d, &content);

    /* Handle events */
    while (1) {
        nsfb_event_t event;
        if (nsfb_event(d.nsfb, &event, -1)) {

            if (event.type == NSFB_EVENT_CONTROL) {
                if (event.value.controlcode == NSFB_CONTROL_QUIT) exit(0);
            } else if (event.type == NSFB_EVENT_KEY_DOWN) {
                enum nsfb_key_code_e code = event.value.keycode;
                if (code == NSFB_KEY_q) exit(0);
            } else if (event.type == NSFB_EVENT_RESIZE) {
                resize_display(name, &d,
                        event.value.resize.w, event.value.resize.h);
                render(name, &d, &content);
            }
        }
    }
}
