#include "draw.h"

#include <stdint.h>
#include <assert.h>
#include "memfuncs.h"
#include "font.h"
#include "config.h"

static struct framebuffers {
    uint8_t *top_left;
    uint8_t *top_right;
    uint8_t *bottom;
} *framebuffers = (struct framebuffers *)0x23FFFE00;

static uint8_t print_pos;
enum screen print_screen = screen_bottom;

struct buffer_select {
    uint8_t *buffer1;
    uint8_t *buffer2;
    unsigned int width;
    unsigned int height;
    unsigned int size;
};

void set_buffers(const enum screen screen, struct buffer_select *select)
{
    if (screen == screen_top_left || screen == screen_top_right) {
        select->buffer1 = framebuffers->top_left;
        select->buffer2 = framebuffers->top_right;
        select->width = SCREEN_TOP_WIDTH;
        select->height = SCREEN_TOP_HEIGHT;
        select->size = SCREEN_TOP_SIZE;
    } else {
        select->buffer1 = framebuffers->bottom;
        select->width = SCREEN_BOTTOM_WIDTH;
        select->height = SCREEN_BOTTOM_HEIGHT;
        select->size = SCREEN_BOTTOM_SIZE;
    }
}

void clear_screen(const enum screen screen)
{
    struct buffer_select select = {0};
    set_buffers(screen, &select);

    memset(select.buffer1, 0, select.size);

    if (select.buffer2) {
        memset(select.buffer2, 0, select.size);
    }
}

void clear_screens()
{
    clear_screen(screen_top_left);
    clear_screen(screen_bottom);
    print_pos = 0;
}

void scroll_area(const enum screen screen, const unsigned int pos_x, const unsigned int pos_y, const unsigned int width, const unsigned int height, const int pixels)
{
    struct buffer_select select = {0};
    set_buffers(screen, &select);

    for (unsigned int x = pos_x; x < pos_x + width; x++) {
        void *buffer1 = select.buffer1 + (x * select.height + pos_y) * 3;

        if (pixels < 0) {
            memmove(buffer1 + pixels * -3, buffer1, (height + pixels) * 3);
            memset(buffer1, 0, pixels * -3);
        } else {
            memmove(buffer1, buffer1 + pixels * 3, (height - pixels) * 3);
            memset(buffer1 + (height - pixels) * 3, 0, pixels * 3);
        }

        if (select.buffer2 && select.buffer1 != select.buffer2) {
            void *buffer2 = select.buffer2 + (x * select.height + pos_y) * 3;

            if (pixels < 0) {
                memmove(buffer2 + pixels * -3, buffer2, (height + pixels) * 3);
                memset(buffer2, 0, pixels * -3);
            } else {
                memmove(buffer2, buffer2 + pixels * 3, (height - pixels) * 3);
                memset(buffer2 + (height - pixels) * 3, 0, pixels * 3);
            }
        }
    }
}

void draw_character(const enum screen screen, const char character, const unsigned int pos_x, const unsigned int pos_y, const uint32_t color)
{
    struct buffer_select select = {0};
    set_buffers(screen, &select);

    for (int y = 0; y < 8; y++) {
        unsigned char char_pos = font[character * 8 + y];

        for (int x = 7; x >= 0; x--) {
            int screen_pos = ((pos_x + MARGIN_LEFT) * select.height * 3 + (select.height - y - (pos_y + MARGIN_TOP) - 1) * 3) + (7 - x) * 3 * select.height;

            if ((char_pos >> x) & 1) {
                select.buffer1[screen_pos] = color >> 16;
                select.buffer1[screen_pos + 1] = color >> 8;
                select.buffer1[screen_pos + 2] = color;

                if (select.buffer2) {
                    select.buffer2[screen_pos] = color >> 16;
                    select.buffer2[screen_pos + 1] = color >> 8;
                    select.buffer2[screen_pos + 2] = color;
                }
            }
        }
    }
}

int draw_string(const enum screen screen, const char *string, const unsigned int pos_x, unsigned int pos_y, const uint32_t color)
{
    unsigned int length = strlen(string);
    struct buffer_select select = {0};

    set_buffers(screen, &select);

    for (unsigned int i = 0, line_i = 0; i < length; i++, line_i++) {
        if (string[i] == '\n') {
            pos_y += SPACING_VERT;
            line_i = 0;
            i++;
        } else if (line_i >= (select.width - MARGIN_HORIZ) / SPACING_HORIZ) {
            // Make sure we never get out of the screen.
            pos_y += SPACING_VERT;
            line_i = 2;  // Little offset so we know the same string continues.
            if (string[i] == ' ') i++;  // Spaces at the start look weird
        }

        // Make sure we never get out of the screen... On the bottom.
        if (pos_y >= select.height - MARGIN_VERT) {
            if (screen == print_screen) {
                scroll_area(print_screen, MARGIN_LEFT, MARGIN_TOP, select.width - MARGIN_HORIZ, select.height - MARGIN_VERT, SPACING_VERT * -10);
                pos_y -= SPACING_VERT * 10;
            } else {
                return pos_y;  // Not sure how to handle this. Maybe I shouldn't.
            }
        }

        draw_character(screen, string[i], pos_x + line_i * SPACING_HORIZ, pos_y, color);
    }

    return pos_y;
}

void print(const char *string)
{
    // If silent boot is enabled, don't output.
    if (config->silent_boot)
        return;

    struct buffer_select select = {0};
    set_buffers(print_screen, &select);

    if (print_pos >= (select.height - MARGIN_VERT) / SPACING_VERT) {
        scroll_area(print_screen, MARGIN_LEFT, MARGIN_TOP, select.width - MARGIN_HORIZ, select.height - MARGIN_VERT, SPACING_VERT * -10);
        print_pos -= 10;
    }

    int pos = draw_string(print_screen, string, 0, SPACING_VERT * print_pos, 0xFFFFFF);
    print_pos = pos / SPACING_VERT + 1;
}
