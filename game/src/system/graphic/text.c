#define STB_TRUETYPE_IMPLEMENTATION
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "external/stb_truetype.h"
#include "system/graphic/text.h"
#include "system/graphic/graphic.h"

void convert_to_cps(struct codepoint *cps, stbtt_bakedchar *bakedchr, size_t len)
{
    for (int i = 0; i < len; i++) {
        cps[i].x0 = bakedchr[i].x0; cps[i].y0 = bakedchr[i].y0;
        cps[i].x1 = bakedchr[i].x1; cps[i].y1 = bakedchr[i].y1;
        
        cps[i].xoff = bakedchr[i].xoff; cps[i].yoff = bakedchr[i].yoff;
        cps[i].xadv = bakedchr[i].xadvance;
    }
}

struct baked_font create_ascii_baked_font(unsigned char *ttf)
{
    struct baked_font   font;

    stbtt_bakedchar     cdata[96] = {0};
    struct codepoint   *cps;

    font.width   = 512;
    font.height  = 512;
    font.bitmap  = malloc(512*512);

    cps = malloc(sizeof(struct codepoint) * 96);

    stbtt_BakeFontBitmap(ttf, 0, 32.0, font.bitmap, font.width, font.height, 32, 96, cdata);

    convert_to_cps(cps, cdata, 96); 
    font.cps = cps;
    font.cps_len = 96;
    return font;
}

float *create_text_verts(struct baked_font font, size_t *vert_count, float line_height, const char *text)
{
    struct codepoint    cp;
    float               cp_width, cp_height;
    float               y_head = 0, x_head = 0;

    size_t              verts_per_quad = 6;
    rect2D              dimension, tex;
    float              *vert_data = malloc(strlen(text) * (verts_per_quad*VERT_SIZE));

    *vert_count = 0;

    if (vert_data == NULL) 
        return NULL;

    for (const char *c = text; *c; c++) {
        if (*c == '\n') {
            y_head += line_height;
            x_head = 0;
            continue;
        }

        cp = font.cps[*c - 32];
        cp_width = cp.x1 - cp.x0;
        cp_height = cp.y1 - cp.y0;

        dimension.x0 =  cp.xoff + x_head;
        dimension.y0 = -cp.yoff + y_head;
        dimension.x1 =  cp.xoff + cp_width + x_head;
        dimension.y1 = -cp.yoff - cp_height + y_head;

        tex.x0 = cp.x0 / font.width;
        tex.y0 = cp.y0 / font.height;
        tex.x1 = cp.x1 / font.width;
        tex.y1 = cp.y1 / font.height;

        construct_3D_quad(vert_data + ((*vert_count) * VERT_NUM_OF_ELEMENTS), dimension, tex);
        *vert_count += verts_per_quad;

        x_head += cp.xadv;
    }
    return vert_data;
}

struct graphic_vertecies *graphic_vertecies_create_text(struct baked_font font, float line_height, const char *text)
{
    struct graphic_vertecies *graphic_verts;
    size_t                   vert_count;
    float                   *verts;

    verts = create_text_verts(font, &vert_count, line_height, text);
    graphic_verts = graphic_vertecies_create(verts, vert_count);
    free(verts);
    return graphic_verts;
}

