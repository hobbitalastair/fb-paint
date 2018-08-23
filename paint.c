/* paint.c
 *
 * Pixel manipulator.
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
#include <libnsfb_cursor.h>

#define SURFACE_TYPE NSFB_SURFACE_SDL /* Default surface type */
#define IMG_DEPTH (sizeof(nsfb_colour_t)) /* Image depth, in bytes */

char* name = __FILE__;

typedef struct {
    int x;
    int y;
} point_t;

typedef struct {
    int width;
    int height;
    nsfb_colour_t* buf;
} image_t;

typedef struct {
    /* Visible display width and height */
    int width;
    int height;

    /* Display format, buffer and stride */
    enum nsfb_format_e format;
    uint8_t* buf;
    int stride;

    /* Cursor location */
    int cur_x;
    int cur_y;
    image_t cur_content;

    /* Image scaling */
    int offset_x;
    int offset_y;
    int scale;

    /* Shared libnsfb context */
    nsfb_t *nsfb;
} display_t;

typedef struct {
    nsfb_colour_t background;
    nsfb_colour_t active;
    image_t img;
} content_t;


void initialise_content(content_t *c, int argc, char** argv) {
    /* Initialise the given contents struct.
     *
     * This exits on failure.
     */

    c->background = 0xFFFFFFFF;
    c->active = 0xFF00FFFF;
    c->img.width = 50;
    c->img.height = 50;
    c->img.buf = malloc(c->img.width * c->img.height * IMG_DEPTH);
    if (c->img.buf == NULL) {
        fprintf(stderr, "%s: failed to allocate image buffer\n", name);
        exit(1);
    }
    for (size_t i = 0; i < c->img.width * c->img.height; i++) {
        c->img.buf[i] = 0;
    }
}

void initialise_display(display_t *d, content_t *c) {
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
        nsfb_free(d->nsfb);
        exit(EXIT_FAILURE);
    }
    if (nsfb_get_buffer(d->nsfb, &d->buf, &d->stride) != 0) {
        fprintf(stderr, "%s: failed to get window buffer\n", name);
        nsfb_free(d->nsfb);
        exit(EXIT_FAILURE);
    }

    d->cur_x = 0;
    d->cur_y = 0;
    d->cur_content.width = 10;
    d->cur_content.height = 15;
    d->cur_content.buf = malloc(d->cur_content.width * d->cur_content.height * IMG_DEPTH);
    if (d->cur_content.buf == NULL) {
        fprintf(stderr, "%s: failed to allocate cursor backing buffer\n", name);
        exit(EXIT_FAILURE);
    }

    d->scale = 5;
    d->offset_x = (d->width / (2 * d->scale)) - (c->img.width / 2);
    d->offset_y = (d->height / (2 * d->scale)) - (c->img.height / 2);
}

void resize_display(display_t *d, int width, int height) {
    /* Resize the display, updating the display struct as required.
     *
     * This (may) call exit on failure.
     */

    if (nsfb_set_geometry(d->nsfb, width, height, NSFB_FMT_ANY) != 0) {
        fprintf(stderr, "%s: failed to resize window region\n", name);
        return;
    }

    if (nsfb_get_geometry(d->nsfb, &d->width, &d->height, &d->format) != 0) {
        fprintf(stderr, "%s: failed to get window geometry\n", name);
        nsfb_free(d->nsfb);
        exit(EXIT_FAILURE);
    }
    if (nsfb_get_buffer(d->nsfb, &d->buf, &d->stride) != 0) {
        fprintf(stderr, "%s: failed to get window buffer\n", name);
        nsfb_free(d->nsfb);
        exit(EXIT_FAILURE);
    }
}

static inline point_t display_to_img(display_t* d, int x, int y) {
    /* Convert from a point in display coordinates to a point in the image */
    point_t p = {(x / d->scale) - d->offset_x, (y / d->scale) - d->offset_y};
    return p;
}

void render_cursor(display_t *d) {
    /* Draw the cursor at the given coordinates */

    for (int y = 0; y < d->cur_content.height; y++) {
        for (int x = 0; x < d->cur_content.width; x++) {
            int offset = (d->cur_x + x) + (d->cur_y + y) * (d->stride / 4);
            d->cur_content.buf[x + y * d->cur_content.height] = 
                ((nsfb_colour_t*)d->buf)[offset];
            ((nsfb_colour_t*)d->buf)[offset] = 0xFFFF0000;
        }
    }
}

void render(display_t *d, content_t *c) {
    /* Render the visible screen content */

    nsfb_bbox_t display_box = {0, 0, d->width, d->height};
    if (nsfb_claim(d->nsfb, &display_box) != 0) {
        fprintf(stderr, "%s: failed to claim window region\n", name);
        return;
    }

    for (int y = 0; y < d->height; y++) {
        for (int x = 0; x < d->width; x++) {
            point_t p = display_to_img(d, x, y);
            nsfb_colour_t colour = c->background;
            if (p.x < c->img.width && p.x >= 0 && p.y < c->img.height && p.y >= 0) {
                colour = c->img.buf[p.x + (p.y * c->img.height)];
            } else if ((p.x + p.y) % 2 == 0) {
                colour = 0;
            }
            ((nsfb_colour_t*)d->buf)[x + y * (d->stride / 4)] = colour;
        }
    }

    render_cursor(d);

    if (nsfb_update(d->nsfb, &display_box) != 0) {
        fprintf(stderr, "%s: failed to update window\n", name);
    }
}

void move_cursor(display_t *d, int x, int y) {
    /* Restore the content under the old cursor */

    nsfb_bbox_t cur_box_orig = {d->cur_x, d->cur_y,
        d->cur_x+d->cur_content.width, d->cur_y+d->cur_content.height};
    if (nsfb_claim(d->nsfb, &cur_box_orig) != 0) {
        fprintf(stderr, "%s: failed to claim cursor window region\n", name);
        return;
    }

    for (int y = 0; y < d->cur_content.height; y++) {
        for (int x = 0; x < d->cur_content.width; x++) {
            int offset = (d->cur_x + x) + (d->cur_y + y) * (d->stride / 4);
            ((nsfb_colour_t*)d->buf)[offset] = 
                d->cur_content.buf[x + y * d->cur_content.height];
        }
    }

    if (nsfb_update(d->nsfb, &cur_box_orig) != 0) {
        fprintf(stderr, "%s: failed to update window\n", name);
    }

    d->cur_x = x;
    d->cur_y = y;

    nsfb_bbox_t cur_box = {d->cur_x, d->cur_y,
        d->cur_x+d->cur_content.width, d->cur_y+d->cur_content.height};
    if (nsfb_claim(d->nsfb, &cur_box) != 0) {
        fprintf(stderr, "%s: failed to claim cursor window region\n", name);
        return;
    }

    render_cursor(d);

    if (nsfb_update(d->nsfb, &cur_box) != 0) {
        fprintf(stderr, "%s: failed to update window\n", name);
    }
}

int main(int argc, char** argv) {
    if (argc > 0) name = argv[0];
    if (argc != 2) {
        fprintf(stderr, "usage: %s <image>\n", name);
        exit(EINVAL);
    }

    content_t content;
    initialise_content(&content, argc - 1, &(argv[1]));

    display_t d;
    initialise_display(&d, &content);

    render(&d, &content);

    /* Handle events */
    bool needs_redraw = false;
    while (1) {
        nsfb_event_t event;
        if (nsfb_event(d.nsfb, &event, needs_redraw ? (1000/30) : -1)) {
            if (event.type == NSFB_EVENT_CONTROL) {
                if (event.value.controlcode == NSFB_CONTROL_QUIT) {
                    nsfb_free(d.nsfb);
                    exit(0);
                }
            } else if (event.type == NSFB_EVENT_KEY_DOWN) {
                enum nsfb_key_code_e code = event.value.keycode;
                if (code == NSFB_KEY_q) {
                    nsfb_free(d.nsfb);
                    exit(0);
                }
                
                if (code == NSFB_KEY_MOUSE_1) {
                    point_t p = display_to_img(&d, d.cur_x, d.cur_y);
                    if (p.x >= 0 && p.x < content.img.width && p.y >= 0 && p.y < content.img.height) {
                        content.img.buf[p.x + p.y * content.img.width] = content.active;
                    }
                }
                if (code == NSFB_KEY_LEFT) d.offset_x--;
                if (code == NSFB_KEY_RIGHT) d.offset_x++;
                if (code == NSFB_KEY_UP) d.offset_y--;
                if (code == NSFB_KEY_DOWN) d.offset_y++;
                if (d.scale > 1 && (code == NSFB_KEY_MINUS || code == NSFB_KEY_KP_MINUS)) {
                    d.offset_x *= 2;
                    d.offset_y *= 2;
                    d.scale /= 2;
                }
                if (code == NSFB_KEY_PLUS || code == NSFB_KEY_KP_PLUS ||
                    code == NSFB_KEY_EQUALS || code == NSFB_KEY_KP_EQUALS) {
                    d.offset_x /= 2;
                    d.offset_y /= 2;
                    d.scale *= 2;
                }
                needs_redraw = true;
            } else if (event.type == NSFB_EVENT_RESIZE) {
                int clamped_x = d.cur_x;
                int clamped_y = d.cur_y;
                if (clamped_x >= event.value.resize.w) clamped_x = event.value.resize.w - 1;
                if (clamped_x < 0) clamped_x = 0;
                if (clamped_y >= event.value.resize.h) clamped_y = event.value.resize.h - 1;
                if (clamped_y < 0) clamped_y = 0;
                move_cursor(&d, clamped_x, clamped_y);
                resize_display(&d,
                    event.value.resize.w, event.value.resize.h);
                needs_redraw = true;
            } else if (event.type == NSFB_EVENT_MOVE_ABSOLUTE) {
                int clamped_x = event.value.vector.x;
                int clamped_y = event.value.vector.y;
                if (clamped_x >= d.width) clamped_x = d.width - 1;
                if (clamped_x < 0) clamped_x = 0;
                if (clamped_y >= d.height) clamped_y = d.height - 1;
                if (clamped_y < 0) clamped_y = 0;
                move_cursor(&d, clamped_x, clamped_y);
            }
            
            if (needs_redraw || (event.type == NSFB_EVENT_CONTROL &&
                    event.value.controlcode == NSFB_CONTROL_TIMEOUT)) {
                render(&d, &content);
                needs_redraw = false;
            }
        }
    }
}
