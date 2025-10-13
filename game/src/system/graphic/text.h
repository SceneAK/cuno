#ifndef TEXT_H
#define TEXT_H
#include <stddef.h>
#include "system/graphic.h"

struct codepoint {
    float x0, y0, x1, y1, xoff, yoff, xadv;
};
struct baked_font {
    unsigned char       *bitmap;
    unsigned int         width,
                         height;

    struct codepoint    *cps;
    size_t               cps_len;
};
struct baked_font create_ascii_baked_font(unsigned char *ttf);
float *create_text_verts(struct baked_font font, size_t *vert_count, float line_height, const char *text);
struct graphic_vertecies *graphic_vertecies_create_text(struct baked_font font, float line_height, const char *text);

#endif
