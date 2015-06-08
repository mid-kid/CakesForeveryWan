#include "draw.h"

#include <stdint.h>
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
static uint8_t *print_pos = (uint8_t *)0x20A00000;
#endif

struct buffer_select {
    uint8_t *buffer1;
    uint8_t *buffer2;
    uint8_t *buffer3;
    uint8_t *buffer4;
    int height;
    int size;
};

int strlen(char *string)
{
    char *string_end = string;
    while (*string_end) string_end++;
    return string_end - string;
}

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
        select->height = screen_top_height;
        select->size = screen_top_size;
#ifdef ENTRY_MSET
        select->buffer2 = screen_top_left2;
        select->buffer4 = screen_top_right2;
#endif
    } else {
        select->buffer1 = screen_bottom1;
        select->height = screen_top_height;
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

    for (int i = 0; i < select.size; i++) {
        select.buffer1[i] = 0;
#ifdef ENTRY_MSET
        select.buffer2[i] = 0;
#endif
        if (select.buffer3 || select.buffer4) {
            select.buffer3[i] = 0;
#ifdef ENTRY_MSET   
            select.buffer4[i] = 0;
#endif
        }
    }
}

void clear_screens()
{
    clear_screen(screen_top_left);
    clear_screen(screen_top_right);
    clear_screen(screen_bottom);
    *print_pos = 0;
}

void draw_character(enum screen screen, char character, int pos_x, int pos_y)
{
    struct buffer_select select = {0};
    set_buffers(screen, &select);

    for (int y = 0; y < 8; y++) {
        unsigned char char_pos = font[character * 8 + y];

        for (int x = 7; x >= 0; x--) {
            int screen_pos = (pos_x * select.height * 3 + (select.height - y - pos_y - 1) * 3) + (7 - x) * 3 * select.height;

            if ((char_pos >> x) & 1) {
                select.buffer1[screen_pos] = 0xFF;
                select.buffer1[screen_pos + 1] = 0xFF;
                select.buffer1[screen_pos + 2] = 0xFF;
#ifdef ENTRY_MSET
                select.buffer2[screen_pos] = 0xFF;
                select.buffer2[screen_pos + 1] = 0xFF;
                select.buffer2[screen_pos + 2] = 0xFF;
#endif
                if (select.buffer3 || select.buffer4) {
                    select.buffer3[screen_pos] = 0xFF;
                    select.buffer3[screen_pos + 1] = 0xFF;
                    select.buffer3[screen_pos + 2] = 0xFF;
#ifdef ENTRY_MSET
                    select.buffer4[screen_pos] = 0xFF;
                    select.buffer4[screen_pos + 1] = 0xFF;
                    select.buffer4[screen_pos + 2] = 0xFF;
#endif
                }
            }
        }
    }
}

void draw_string(enum screen screen, char *string, int pos_x, int pos_y)
{
    int length = strlen(string);
    for (int i = 0; i < length; i++) {
        draw_character(screen, string[i], pos_x + i * 8, pos_y);
    }
}

void print(char *string)
{
    // I'll just assume both screens have the same height, because I'm too lazy
    if (*print_pos > (screen_top_height - 30) / 10) {
        clear_screen(*print_screen);
        *print_pos = 0;
    }

    draw_string(*print_screen, string, 10, 10 + 10 * *print_pos);

    *print_pos += 1;
}
