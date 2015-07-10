/*
 * Copyright (c) 2014,2015 Hayaki Saito
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#if HAVE_LIMITS_H
# include <limits.h>
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#include "frame.h"
#include <sixel.h>


SIXELAPI sixel_frame_t *
sixel_frame_create(void)
{
    sixel_frame_t *frame;

    frame = malloc(sizeof(sixel_frame_t));
    if (frame == NULL) {
        return NULL;
    }
    frame->ref = 1;
    frame->pixels = NULL;
    frame->palette = NULL;
    frame->width = 0;
    frame->height = 0;
    frame->ncolors = (-1);
    frame->pixelformat = SIXEL_PIXELFORMAT_RGB888;
    frame->delay = 0;
    frame->frame_no = 0;
    frame->loop_count = 0;
    frame->multiframe = 0;
    frame->transparent = (-1);

    return frame;
}


SIXELAPI void
sixel_frame_destroy(sixel_frame_t *frame)
{
    if (frame) {
        free(frame->pixels);
        free(frame->palette);
        free(frame);
    }
}


SIXELAPI void
sixel_frame_ref(sixel_frame_t *frame)
{
    /* TODO: be thread safe */
    ++frame->ref;
}


SIXELAPI void
sixel_frame_unref(sixel_frame_t *frame)
{
    /* TODO: be thread safe */
    if (frame != NULL && --frame->ref == 0) {
        sixel_frame_destroy(frame);
    }
}


SIXELAPI int
sixel_frame_init(
    sixel_frame_t *frame,
    unsigned char *pixels,
    int width,
    int height,
    int pixelformat,
    unsigned char *palette,
    int ncolors
)
{
    frame->pixels = pixels;
    frame->width = width;
    frame->height = height;
    frame->pixelformat = pixelformat;
    frame->palette = palette;
    frame->ncolors = ncolors;

    return 0;
}


/* get pixels */
SIXELAPI unsigned char *
sixel_frame_get_pixels(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->pixels;
}


/* get palette */
SIXELAPI unsigned char *
sixel_frame_get_palette(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->palette;
}


/* get width */
SIXELAPI int
sixel_frame_get_width(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->width;
}


/* get height */
SIXELAPI int
sixel_frame_get_height(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->height;
}


/* get ncolors */
SIXELAPI int
sixel_frame_get_ncolors(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->ncolors;
}


/* get pixelformat */
SIXELAPI int
sixel_frame_get_pixelformat(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->pixelformat;
}


/* get transparent */
SIXELAPI int
sixel_frame_get_transparent(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->transparent;
}


/* get transparent */
SIXELAPI int
sixel_frame_get_multiframe(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->multiframe;
}


/* get delay */
SIXELAPI int
sixel_frame_get_delay(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->delay;
}


/* get frame no */
SIXELAPI int
sixel_frame_get_frame_no(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->frame_no;
}


/* get loop no */
SIXELAPI int
sixel_frame_get_loop_no(sixel_frame_t /* in */ *frame)  /* frame object */
{
    return frame->loop_count;
}


SIXELAPI SIXELSTATUS
sixel_frame_strip_alpha(
    sixel_frame_t  /* in */ *frame,
    unsigned char  /* in */ *bgcolor
)
{
    SIXELSTATUS status = SIXEL_FALSE;
    int x;
    int y;
    unsigned char *src;
    unsigned char *dst;
    unsigned char alpha;

    src = dst = frame->pixels;

    switch (frame->pixelformat) {
    case SIXEL_PIXELFORMAT_RGBA8888:
    case SIXEL_PIXELFORMAT_ARGB8888:
        for (y = 0; y < frame->height; y++) {
            for (x = 0; x < frame->width; x++) {
                if (bgcolor) {
                    alpha = src[3];
                    *dst++ = (*src++ * alpha + bgcolor[0] * (0xff - alpha)) >> 8;
                    *dst++ = (*src++ * alpha + bgcolor[1] * (0xff - alpha)) >> 8;
                    *dst++ = (*src++ * alpha + bgcolor[2] * (0xff - alpha)) >> 8;
                    src++;
                } else if (frame->pixelformat == SIXEL_PIXELFORMAT_ARGB8888){
                    src++;            /* A */
                    *dst++ = *src++;  /* R */
                    *dst++ = *src++;  /* G */
                    *dst++ = *src++;  /* B */
                } else if (frame->pixelformat == SIXEL_PIXELFORMAT_RGBA8888){
                    *dst++ = *src++;  /* R */
                    *dst++ = *src++;  /* G */
                    *dst++ = *src++;  /* B */
                    src++;            /* A */
                }
            }
        }
        frame->pixelformat = SIXEL_PIXELFORMAT_RGB888;
        break;
    default:
        break;
    }

    status = SIXEL_OK;

    return status;
}


SIXELAPI SIXELSTATUS
sixel_frame_convert_to_rgb888(sixel_frame_t /*in */ *frame)
{
    SIXELSTATUS status = SIXEL_FALSE;
    unsigned char *normalized_pixels = NULL;
    int size;
    unsigned char *dst;
    unsigned char *src;
    unsigned char *p;

    switch (frame->pixelformat) {
    case SIXEL_PIXELFORMAT_PAL1:
    case SIXEL_PIXELFORMAT_PAL2:
    case SIXEL_PIXELFORMAT_PAL4:
        size = frame->width * frame->height * 4;
        normalized_pixels = malloc(size);
        src = normalized_pixels + frame->width * frame->height * 3;
        dst = normalized_pixels;
        status = sixel_helper_normalize_pixelformat(src,
                                                    &frame->pixelformat,
                                                    frame->pixels,
                                                    frame->pixelformat,
                                                    frame->width,
                                                    frame->height);
        if (SIXEL_FAILED(status)) {
            free(normalized_pixels);
            goto end;
        }
        for (p = src; dst < src; ++p) {
            *dst++ = *(frame->palette + *p * 3 + 0);
            *dst++ = *(frame->palette + *p * 3 + 1);
            *dst++ = *(frame->palette + *p * 3 + 2);
        }
        free(frame->pixels);
        frame->pixels = normalized_pixels;
        frame->pixelformat = SIXEL_PIXELFORMAT_RGB888;
        break;
    case SIXEL_PIXELFORMAT_PAL8:
        size = frame->width * frame->height * 3;
        normalized_pixels = malloc(size);
        src = frame->pixels;
        dst = normalized_pixels;
        for (; dst != normalized_pixels + size; ++src) {
            *dst++ = frame->palette[*src * 3 + 0];
            *dst++ = frame->palette[*src * 3 + 1];
            *dst++ = frame->palette[*src * 3 + 2];
        }
        free(frame->pixels);
        frame->pixels = normalized_pixels;
        frame->pixelformat = SIXEL_PIXELFORMAT_RGB888;
        break;
    case SIXEL_PIXELFORMAT_RGB888:
        break;
    case SIXEL_PIXELFORMAT_G8:
    case SIXEL_PIXELFORMAT_GA88:
    case SIXEL_PIXELFORMAT_AG88:
    case SIXEL_PIXELFORMAT_RGB555:
    case SIXEL_PIXELFORMAT_RGB565:
    case SIXEL_PIXELFORMAT_BGR555:
    case SIXEL_PIXELFORMAT_BGR565:
    case SIXEL_PIXELFORMAT_RGBA8888:
    case SIXEL_PIXELFORMAT_ARGB8888:
        /* normalize pixelformat */
        size = frame->width * frame->height * 3;
        normalized_pixels = malloc(size);
        status = sixel_helper_normalize_pixelformat(normalized_pixels,
                                                    &frame->pixelformat,
                                                    frame->pixels,
                                                    frame->pixelformat,
                                                    frame->width,
                                                    frame->height);
        if (SIXEL_FAILED(status)) {
            free(normalized_pixels);
            goto end;
        }
        free(frame->pixels);
        frame->pixels = normalized_pixels;
        break;
    default:
        status = SIXEL_LOGIC_ERROR;
        sixel_helper_set_additional_message(
            "do_resize: invalid pixelformat.");
        goto end;
    }

    status = SIXEL_OK;

end:
    return status;
}


SIXELAPI SIXELSTATUS
sixel_frame_resize(
    sixel_frame_t *frame,
    int width,
    int height,
    int method_for_resampling
)
{
    SIXELSTATUS status = SIXEL_FALSE;
    int size;
    unsigned char *scaled_frame = NULL;

    status = sixel_frame_convert_to_rgb888(frame);
    if (SIXEL_FAILED(status)) {
        goto end;
    }

    size = width * height * 3;
    scaled_frame = malloc(size);
    if (scaled_frame == NULL) {
        status = SIXEL_BAD_ALLOCATION;
        goto end;
    }

    status = sixel_helper_scale_image(
        scaled_frame,
        frame->pixels, frame->width, frame->height, 3,
        width,
        height,
        method_for_resampling);
    if (SIXEL_FAILED(status)) {
        goto end;
    }
    free(frame->pixels);
    frame->pixels = scaled_frame;
    frame->width = width;
    frame->height = height;

    status = SIXEL_OK;

end:
    return status;
}


static SIXELSTATUS
clip(unsigned char *pixels,
     int sx,
     int sy,
     int pixelformat,
     int cx,
     int cy,
     int cw,
     int ch)
{
    SIXELSTATUS status = SIXEL_FALSE;
    int y;
    unsigned char *src;
    unsigned char *dst;
    int depth;
    char message[256];
    int nwrite;

    /* unused */ (void) sx;
    /* unused */ (void) sy;
    /* unused */ (void) cx;

    switch (pixelformat) {
    case SIXEL_PIXELFORMAT_PAL8:
    case SIXEL_PIXELFORMAT_G8:
    case SIXEL_PIXELFORMAT_RGB888:
        depth = sixel_helper_compute_depth(pixelformat);
        if (depth < 0) {
            status = SIXEL_LOGIC_ERROR;
            nwrite = sprintf(message,
                             "clip: "
                             "sixel_helper_compute_depth(%08x) failed.",
                             pixelformat);
            if (nwrite > 0) {
                sixel_helper_set_additional_message(message);
            }
            goto end;
        }

        dst = pixels;
        src = pixels + cy * sx * depth + cx * depth;
        for (y = 0; y < ch; y++) {
            memmove(dst, src, cw * depth);
            dst += (cw * depth);
            src += (sx * depth);
        }

        status = SIXEL_OK;

        break;
    default:
        status = SIXEL_BAD_ARGUMENT;
        nwrite = sprintf(message,
                         "clip: "
                         "invalid pixelformat(%08x) is specified.",
                         pixelformat);
        if (nwrite > 0) {
            sixel_helper_set_additional_message(message);
        }
        break;
    }

end:
    return status;
}


SIXELAPI SIXELSTATUS
sixel_frame_clip(
    sixel_frame_t *frame,
    int x,
    int y,
    int width,
    int height
)
{
    SIXELSTATUS status = SIXEL_FALSE;
    unsigned char *normalized_pixels;

    switch (frame->pixelformat) {
    case SIXEL_PIXELFORMAT_PAL1:
    case SIXEL_PIXELFORMAT_PAL2:
    case SIXEL_PIXELFORMAT_PAL4:
    case SIXEL_PIXELFORMAT_G1:
    case SIXEL_PIXELFORMAT_G2:
    case SIXEL_PIXELFORMAT_G4:
        normalized_pixels = malloc(frame->width * frame->height);
        status = sixel_helper_normalize_pixelformat(normalized_pixels,
                                                    &frame->pixelformat,
                                                    frame->pixels,
                                                    frame->pixelformat,
                                                    frame->width,
                                                    frame->height);
        if (SIXEL_FAILED(status)) {
            free(normalized_pixels);
            goto end;
        }
        free(frame->pixels);
        frame->pixels = normalized_pixels;
        break;
    default:
        break;
    }

    status = clip(frame->pixels,
                  frame->width,
                  frame->height,
                  frame->pixelformat,
                  x,
                  y,
                  width,
                  height);
    if (SIXEL_FAILED(status)) {
        goto end;
    }
    frame->width = width;
    frame->height = height;

    status = SIXEL_OK;

end:
    return status;
}


#if HAVE_TESTS
static int
test1(void)
{
    sixel_frame_t *frame = NULL;
    int nret = EXIT_FAILURE;

    frame = sixel_frame_create();
    if (frame == NULL) {
        goto error;
    }
    sixel_frame_ref(frame);
    sixel_frame_unref(frame);
    nret = EXIT_SUCCESS;

error:
    sixel_frame_unref(frame);
    return nret;
}


static int
test2(void)
{
    sixel_frame_t *frame = NULL;
    int nret = EXIT_FAILURE;
    unsigned char *pixels = malloc(4);
    unsigned char *bgcolor = malloc(3);
    SIXELSTATUS status;

    pixels[0] = 0x43;
    pixels[1] = 0x89;
    pixels[2] = 0x97;
    pixels[3] = 0x32;

    memset(bgcolor, 0x10, 3);

    frame = sixel_frame_create();
    if (frame == NULL) {
        goto error;
    }

    status = sixel_frame_init(frame,
                              pixels,
                              1,
                              1,
                              SIXEL_PIXELFORMAT_RGBA8888,
                              NULL,
                              0);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    status = sixel_frame_strip_alpha(frame, bgcolor);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    if (frame->pixelformat != SIXEL_PIXELFORMAT_RGB888) {
        goto error;
    }

    if (frame->pixels[0] != (0x43 * 0x32 + 0x10 * (0xff - 0x32)) >> 8) {
        goto error;
    }

    if (frame->pixels[1] != (0x89 * 0x32 + 0x10 * (0xff - 0x32)) >> 8) {
        goto error;
    }

    if (frame->pixels[2] != (0x97 * 0x32 + 0x10 * (0xff - 0x32)) >> 8) {
        goto error;
    }

    nret = EXIT_SUCCESS;

error:
    sixel_frame_unref(frame);
    return nret;
}


static int
test3(void)
{
    sixel_frame_t *frame = NULL;
    int nret = EXIT_FAILURE;
    unsigned char *pixels = malloc(4);
    SIXELSTATUS status;

    pixels[0] = 0x43;
    pixels[1] = 0x89;
    pixels[2] = 0x97;
    pixels[3] = 0x32;

    frame = sixel_frame_create();
    if (frame == NULL) {
        goto error;
    }

    status = sixel_frame_init(frame,
                              pixels,
                              1,
                              1,
                              SIXEL_PIXELFORMAT_RGBA8888,
                              NULL,
                              0);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    status = sixel_frame_strip_alpha(frame, NULL);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    if (frame->pixelformat != SIXEL_PIXELFORMAT_RGB888) {
        goto error;
    }

    if (frame->pixels[0] != 0x43) {
        goto error;
    }

    if (frame->pixels[1] != 0x89) {
        goto error;
    }

    if (frame->pixels[2] != 0x97) {
        goto error;
    }

    nret = EXIT_SUCCESS;

error:
    sixel_frame_unref(frame);
    return nret;
}


static int
test4(void)
{
    sixel_frame_t *frame = NULL;
    int nret = EXIT_FAILURE;
    unsigned char *pixels = malloc(4);
    SIXELSTATUS status;

    pixels[0] = 0x43;
    pixels[1] = 0x89;
    pixels[2] = 0x97;
    pixels[3] = 0x32;

    frame = sixel_frame_create();
    if (frame == NULL) {
        goto error;
    }

    status = sixel_frame_init(frame,
                              pixels,
                              1,
                              1,
                              SIXEL_PIXELFORMAT_ARGB8888,
                              NULL,
                              0);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    status = sixel_frame_strip_alpha(frame, NULL);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    if (frame->pixelformat != SIXEL_PIXELFORMAT_RGB888) {
        goto error;
    }

    if (frame->pixels[0] != 0x89) {
        goto error;
    }

    if (frame->pixels[1] != 0x97) {
        goto error;
    }

    if (frame->pixels[2] != 0x32) {
        goto error;
    }

    nret = EXIT_SUCCESS;

error:
    sixel_frame_unref(frame);
    return nret;
}


static int
test5(void)
{
    sixel_frame_t *frame = NULL;
    int nret = EXIT_FAILURE;
    unsigned char *pixels = malloc(1);
    unsigned char *palette = malloc(3);
    SIXELSTATUS status;

    palette[0] = 0x43;
    palette[1] = 0x89;
    palette[2] = 0x97;

    pixels[0] = 0;

    frame = sixel_frame_create();
    if (frame == NULL) {
        goto error;
    }

    status = sixel_frame_init(frame,
                              pixels,
                              1,
                              1,
                              SIXEL_PIXELFORMAT_PAL8,
                              palette,
                              1);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    status = sixel_frame_convert_to_rgb888(frame);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    if (frame->pixelformat != SIXEL_PIXELFORMAT_RGB888) {
        goto error;
    }

    if (frame->pixels[0] != 0x43) {
        goto error;
    }

    if (frame->pixels[1] != 0x89) {
        goto error;
    }

    if (frame->pixels[2] != 0x97) {
        goto error;
    }

    nret = EXIT_SUCCESS;

error:
    sixel_frame_unref(frame);
    return nret;
}


static int
test6(void)
{
    sixel_frame_t *frame = NULL;
    int nret = EXIT_FAILURE;
    unsigned char *pixels = malloc(6);
    unsigned char *palette = malloc(3);
    SIXELSTATUS status;

    palette[0] = 0x43;
    palette[1] = 0x89;
    palette[2] = 0x97;

    pixels[0] = 0;

    frame = sixel_frame_create();
    if (frame == NULL) {
        goto error;
    }

    status = sixel_frame_init(frame,
                              pixels,
                              1,
                              1,
                              SIXEL_PIXELFORMAT_PAL1,
                              palette,
                              1);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    status = sixel_frame_convert_to_rgb888(frame);
    if (SIXEL_FAILED(status)) {
        goto error;
    }

    if (frame->pixelformat != SIXEL_PIXELFORMAT_RGB888) {
        goto error;
    }

    if (frame->pixels[0] != 0x43) {
        goto error;
    }

    if (frame->pixels[1] != 0x89) {
        goto error;
    }

    if (frame->pixels[2] != 0x97) {
        goto error;
    }

    nret = EXIT_SUCCESS;

error:
    sixel_frame_unref(frame);
    return nret;
}


int
sixel_frame_tests_main(void)
{
    int nret = EXIT_FAILURE;
    size_t i;
    typedef int (* testcase)(void);

    static testcase const testcases[] = {
        test1,
        test2,
        test3,
        test4,
        test5,
        test6,
    };

    for (i = 0; i < sizeof(testcases) / sizeof(testcase); ++i) {
        nret = testcases[i]();
        if (nret != EXIT_SUCCESS) {
            goto error;
        }
    }

    nret = EXIT_SUCCESS;

error:
    return nret;
}
#endif  /* HAVE_TESTS */


/* emacs, -*- Mode: C; tab-width: 4; indent-tabs-mode: nil -*- */
/* vim: set expandtab ts=4 : */
/* EOF */
