#include "draw.h"

#include <stdint.h>
#include "memfuncs.h"
#include "font.h"

static struct framebuffers {
    uint8_t *top_left;
    uint8_t *top_right;
    uint8_t *bottom;
} *framebuffers = (struct framebuffers *)0x23FFFE00;

#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOTTOM_WIDTH 320
#define SCREEN_BOTTOM_HEIGHT 240
#define SCREEN_TOP_SIZE (SCREEN_TOP_WIDTH * SCREEN_TOP_HEIGHT * 3)
#define SCREEN_BOTTOM_SIZE (SCREEN_BOTTOM_WIDTH * SCREEN_BOTTOM_HEIGHT * 3)

static uint8_t print_pos;
enum screen print_screen = screen_bottom;

struct buffer_select {
    uint8_t *buffer1;
    uint8_t *buffer2;
    int size;
};

void set_buffers(enum screen screen, struct buffer_select *select)
{
    if (screen == screen_top_left || screen == screen_top_right) {
        select->buffer1 = framebuffers->top_left;
        select->buffer2 = framebuffers->top_right;
        select->size = SCREEN_TOP_SIZE;
    } else {
        select->buffer1 = framebuffers->bottom;
        select->size = SCREEN_BOTTOM_SIZE;
    }
}

void clear_screen(enum screen screen)
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

void draw_character(enum screen screen, char character, int pos_x, int pos_y, uint32_t color)
{
    struct buffer_select select = {0};
    set_buffers(screen, &select);

    for (int y = 0; y < 8; y++) {
        unsigned char char_pos = font[character * 8 + y];

        for (int x = 7; x >= 0; x--) {
            // I'll just assume both screens have the same height.
            int screen_pos = (pos_x * SCREEN_TOP_HEIGHT * 3 + (SCREEN_TOP_HEIGHT - y - pos_y - 1) * 3) + (7 - x) * 3 * SCREEN_TOP_HEIGHT;

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

int draw_string(enum screen screen, const char *string, int pos_x, int pos_y, uint32_t color)
{
    int length = strlen(string);

    int screen_width;
    if (screen == screen_top_left || screen == screen_top_right) {
        screen_width = SCREEN_TOP_WIDTH;
    } else {
        screen_width = SCREEN_BOTTOM_WIDTH;
    }

    for (int i = 0, line_i = 0; i < length; i++, line_i++) {
        if (string[i] == '\n') {
            pos_y += SPACING_VERT;
            line_i = 0;
            i++;
        } else if (line_i >= (screen_width - pos_x) / SPACING_HORIZ) {
            // Make sure we never get out of the screen.
            pos_y += SPACING_VERT;
            line_i = 2;  // Little offset so we know the same string continues.
            if (string[i] == ' ') i++;  // Spaces at the start look weird
        }

        draw_character(screen, string[i], pos_x + line_i * SPACING_HORIZ, pos_y, color);
    }

    return pos_y;
}

void print(const char *string)
{
    // I'll just assume both screens have the same height.
    if (print_pos > (SCREEN_TOP_HEIGHT - 30) / SPACING_VERT) {
        clear_screen(print_screen);
        print_pos = 0;
    }

    int pos = draw_string(print_screen, string, 10, 10 + SPACING_VERT * print_pos, 0xFFFFFF);
    print_pos = pos / SPACING_VERT;
}
