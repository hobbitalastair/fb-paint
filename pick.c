/* pick.c
 *
 * Colour picker.
 *
 * Author:  Alastair Hughes
 * Contact: hobbitalastair at yandex dot com
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

typedef enum { SOURCE_GUI, SOURCE_STDIN } source_t;
typedef uint32_t colour_t;
void select_colour(source_t source, colour_t colour);

void select_colour(source_t source, colour_t colour) {
    /* Update the display and output with the given selected colour */

    fprintf(stdout, "0x%06x\n", colour);

    /* Render */
    // TODO: Implement rendering...
    fprintf(stderr, "0x%06x\n", colour);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "No input file supplied\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "starting\n");

    FILE* control = fopen(argv[1], "r");
    if (control == NULL) {
        fprintf(stderr, "File %s not found or invalid\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "have opened file\n");

    fd_set fdread = {0};
    FD_ZERO(&fdread);
    fd_set fderr = {0};
    FD_ZERO(&fderr);
    fprintf(stderr, "finding file numbers\n");
    int fds[2] = {fileno(stdin), fileno(control)};

    while (true) {
        FD_SET(fds[0], &fdread);
        FD_SET(fds[1], &fdread);
        FD_SET(fds[0], &fderr);
        FD_SET(fds[1], &fderr);
        fprintf(stderr, "select...\n");
        if (select(2, &fdread, NULL, &fderr, NULL) == -1) {
            fprintf(stderr, "error in select\n");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(fds[0], &fderr) || FD_ISSET(fds[1], &fderr)) {
            if (feof(control)) {
                exit(EXIT_SUCCESS);
            } else {
                fprintf(stderr, "error reading from a file\n");
                exit(EXIT_FAILURE);
            }
        }

        FILE* f = NULL;
        source_t source;
        if (FD_ISSET(fds[0], &fdread)) {
            fprintf(stderr, "reading from stdin\n");
            f = stdin;
            source = SOURCE_STDIN;
        } else if (FD_ISSET(fds[1], &fdread)) {
            fprintf(stderr, "reading from control\n");
            f = control;
            source = SOURCE_GUI;
        }

        if (f != NULL) {
            char tok[10] = {0}; /* 0x123456 \n \0 */
            size_t len = fread(tok, 1, sizeof(tok) - 1, f);
            tok[len - 1] = '\0'; /* Remove newline */
            if (len != sizeof(tok) - 1 || tok[0] != '0' || tok[1] != 'x') {
                fprintf(stderr, "invalid input '%s' (should be in the form 0xRRGGBB)\n", tok);
            } else {
                char* endptr;
                colour_t colour = strtol(tok, &endptr, 16);
                if (endptr != &(tok[len - 1])) {
                    fprintf(stderr, "invalid input - not hexadecimal number '%s'\n", endptr);
                } else {
                    select_colour(source, colour);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
