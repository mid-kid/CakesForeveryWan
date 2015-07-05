#include "draw.h"

#include <stdint.h>
#include "memfuncs.h"
#include "font.h"

static uint8_t *buffer_top_left;
static uint8_t *buffer_top_right;
static uint8_t *buffer_bottom;

#define screen_top_width 400
#define screen_top_height 240
#define screen_bottom_width 340
#define screen_bottom_height 240
#define screen_top_size (screen_top_width * screen_top_height * 3)
#define screen_bottom_size (screen_bottom_width * screen_bottom_height * 3)

static uint8_t print_pos;

struct buffer_select {
    uint8_t *buffer1;
    uint8_t *buffer2;
    int size;
};

void draw_init(uint32_t *data)
{
    buffer_top_left = (uint8_t *)data[0];
    buffer_top_right = (uint8_t *)data[1];
    buffer_bottom = (uint8_t *)data[2];
    print_screen = screen_bottom;
    clear_screens();
}

void set_buffers(enum screen screen, struct buffer_select *select)
{
    if (screen == screen_top_left || screen == screen_top_right) {
        select->buffer1 = buffer_top_left;
        select->buffer2 = buffer_top_right;
        select->size = screen_top_size;
    } else {
        select->buffer1 = buffer_bottom;
        select->size = screen_bottom_size;
    }
}

void clear_screen(enum screen screen)
{
    struct buffer_select select = {0};
    set_buffers(screen, &select);

    memset32(select.buffer1, 0, select.size);

    if (select.buffer2) {
        memset32(select.buffer2, 0, select.size);
    }
}

void clear_screens()
{
    clear_screen(screen_top_left);
    clear_screen(screen_top_right);
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
            int screen_pos = (pos_x * screen_top_height * 3 + (screen_top_height - y - pos_y - 1) * 3) + (7 - x) * 3 * screen_top_height;

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
    for (int i = 0, line_i = 0; i < length; i++, line_i++) {
        if (string[i] == '\n') {
            pos_y += SPACING_VERT;
            line_i = 0;
            i++;
        }

        draw_character(screen, string[i], pos_x + line_i * SPACING_HORIZ, pos_y, color);
    }

    return pos_y;
}

void print(const char *string)
{
    // I'll just assume both screens have the same height.
    if (print_pos > (screen_top_height - 30) / SPACING_VERT) {
        clear_screen(print_screen);
        print_pos = 0;
    }

    int pos = draw_string(print_screen, string, 10, 10 + SPACING_VERT * print_pos, 0xFFFFFF);
    print_pos = pos / SPACING_VERT;
}
