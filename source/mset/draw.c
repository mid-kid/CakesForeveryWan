#include "draw.h"

#include <stdint.h>
#include "misc.h"
#include "font.h"

static uint8_t *screen_top_left1 = (uint8_t *)0x14184E60;
static uint8_t *screen_top_left2 = (uint8_t *)0x141CB370;
static uint8_t *screen_top_right1 = (uint8_t *)0x14282160;
static uint8_t *screen_top_right2 = (uint8_t *)0x142C8670;
static uint8_t *screen_bottom1 = (uint8_t *)0x142118E0;
static uint8_t *screen_bottom2 = (uint8_t *)0x14249CF0;

#define screen_top_width 400
#define screen_top_height 240
#define screen_bottom_width 340
#define screen_bottom_height 240
#define screen_top_size (screen_top_width * screen_top_height * 3)
#define screen_bottom_size (screen_bottom_width * screen_bottom_height * 3)

uint8_t *print_pos = (uint8_t *)0x1419c561 /* screen_top_left1 + screen_top_size + 1 */;

void clear_screen(enum screen screen)
{
    uint8_t *buffer1, *buffer2;
    int size;
    if (screen == screen_top_left) {
        buffer1 = screen_top_left1;
        buffer2 = screen_top_left2;
        size = screen_top_size;
    } else if (screen == screen_top_right) {
        buffer1 = screen_top_right1;
        buffer2 = screen_top_right2;
        size = screen_top_size;
    } else {
        buffer1 = screen_bottom1;
        buffer2 = screen_bottom2;
        size = screen_top_size;
    }

    for (int i = 0; i < size; i++) {
        buffer1[i] = 0;
        buffer2[i] = 0;
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
    uint8_t *buffer1, *buffer2;
    uint8_t *buffer3 = (uint8_t *)0;
    uint8_t *buffer4 = (uint8_t *)0;
    int height;
    if (screen == screen_top_left || screen  == screen_top_right) {
        buffer1 = screen_top_left1;
        buffer2 = screen_top_left2;
        buffer3 = screen_top_right1;
        buffer4 = screen_top_right2;
        height = screen_top_height;
    } else {
        buffer1 = screen_bottom1;
        buffer2 = screen_bottom2;
        height = screen_top_height;
    }

    for (int y = 0; y < 8; y++) {
        unsigned char char_pos = font[character * 8 + y];

        for (int x = 7; x >= 0; x--) {
            int screen_pos = (pos_x * height * 3 + (height - y - pos_y - 1) * 3) + (7 - x) * 3 * height;

            if ((char_pos >> x) & 1) {
                buffer1[screen_pos] = 0xFF;
                buffer1[screen_pos + 1] = 0xFF;
                buffer1[screen_pos + 2] = 0xFF;
                buffer2[screen_pos] = 0xFF;
                buffer2[screen_pos + 1] = 0xFF;
                buffer2[screen_pos + 2] = 0xFF;
                if (buffer3 && buffer4) {
                    buffer3[screen_pos] = 0xFF;
                    buffer3[screen_pos + 1] = 0xFF;
                    buffer3[screen_pos + 2] = 0xFF;
                    buffer4[screen_pos] = 0xFF;
                    buffer4[screen_pos + 1] = 0xFF;
                    buffer4[screen_pos + 2] = 0xFF;
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
    if (*print_pos > (screen_top_height - 30) / 10) {
        clear_screen(screen_top_left);
        clear_screen(screen_top_right);
        *print_pos = 0;
    }
    draw_string(screen_top_left, string, 10, 10 + 10 * *print_pos);
    *print_pos += 1;
}
