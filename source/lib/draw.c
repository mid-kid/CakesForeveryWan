#include "draw.h"

#include <stdint.h>
#include "memfuncs.h"
#include "font.h"

static uint8_t *screen_top_left1 = (uint8_t *)0x14184E60;
static uint8_t *screen_top_right1 = (uint8_t *)0x14282160;
static uint8_t *screen_bottom1 = (uint8_t *)0x142118E0;

#ifdef ENTRY_MSET
static uint8_t *screen_top_left2 = (uint8_t *)0x141CB370;
static uint8_t *screen_top_right2 = (uint8_t *)0x142C8670;
static uint8_t *screen_bottom2 = (uint8_t *)0x14249CF0;
#endif

#define screen_top_width 400
#define screen_top_height 240
#define screen_bottom_width 340
#define screen_bottom_height 240
#define screen_top_size (screen_top_width * screen_top_height * 3)
#define screen_bottom_size (screen_bottom_width * screen_bottom_height * 3)

#if defined(ENTRY_MSET)
static uint8_t *print_pos = (uint8_t *)0x14A00000;
#elif defined(ARM9)
static uint8_t print_pos_local = 0;
static uint8_t *print_pos = &print_pos_local;
#endif

struct buffer_select {
    uint8_t *buffer1;
    uint8_t *buffer2;
    uint8_t *buffer3;
    uint8_t *buffer4;
    int size;
};

#if defined(ENTRY_MSET)
void draw_init()
#elif defined(ARM9)
void draw_init(uint32_t *data)
#endif
{
#if defined(ENTRY_MSET)
    *print_screen = screen_top_left;
#elif defined(ARM9)
    screen_top_left1 = (uint8_t *)data[0];
    screen_top_right1 = (uint8_t *)data[1];
    screen_bottom1 = (uint8_t *)data[2];
    *print_screen = screen_bottom;
#endif
    clear_screens();
}

void set_buffers(enum screen screen, struct buffer_select *select)
{
    if (screen == screen_top_left || screen == screen_top_right) {
        select->buffer1 = screen_top_left1;
        select->buffer3 = screen_top_right1;
        select->size = screen_top_size;
#ifdef ENTRY_MSET
        select->buffer2 = screen_top_left2;
        select->buffer4 = screen_top_right2;
#endif
    } else {
        select->buffer1 = screen_bottom1;
        select->size = screen_bottom_size;
#ifdef ENTRY_MSET
        select->buffer2 = screen_bottom2;
#endif
    }
}

void clear_screen(enum screen screen)
{
    struct buffer_select select = {0};
    set_buffers(screen, &select);

    memset32(select.buffer1, 0, select.size);
#ifdef ENTRY_MSET
    memset32(select.buffer2, 0, select.size);
#endif

    if (select.buffer3 || select.buffer4) {
        memset32(select.buffer3, 0, select.size);
#ifdef ENTRY_MSET
        memset32(select.buffer4, 0, select.size);
#endif
    }
}

void clear_screens()
{
    clear_screen(screen_top_left);
    clear_screen(screen_top_right);
    clear_screen(screen_bottom);
    *print_pos = 0;
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
#ifdef ENTRY_MSET
                select.buffer2[screen_pos] = color >> 16;
                select.buffer2[screen_pos + 1] = color >> 8;
                select.buffer2[screen_pos + 2] = color;
#endif
                if (select.buffer3 || select.buffer4) {
                    select.buffer3[screen_pos] = color >> 16;
                    select.buffer3[screen_pos + 1] = color >> 8;
                    select.buffer3[screen_pos + 2] = color;
#ifdef ENTRY_MSET
                    select.buffer4[screen_pos] = color >> 16;
                    select.buffer4[screen_pos + 1] = color >> 8;
                    select.buffer4[screen_pos + 2] = color;
#endif
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
            pos_y += 10;
            line_i = 0;
            i++;
        }

        draw_character(screen, string[i], pos_x + line_i * 8, pos_y, color);
    }

    return pos_y;
}

void print(const char *string)
{
    // I'll just assume both screens have the same height.
    if (*print_pos > (screen_top_height - 30) / 10) {
        clear_screen(*print_screen);
        *print_pos = 0;
    }

    int pos = draw_string(*print_screen, string, 10, 10 + 10 * *print_pos, 0xFFFFFF);
    *print_pos = pos / 10;
}
