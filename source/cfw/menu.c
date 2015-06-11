#include "menu.h"

#include <stdint.h>
#include "../draw.h"
#include "hid.h"

#define COLOR_TITLE 0x0000FF
#define COLOR_IDLE 0xFFFFFF
#define COLOR_SELECTED 0xFF0000

// No boundary checks, use this responsibly.
int draw_menu(char *title, int back, int count, char *options[])
{
    int current = 0;

    clear_screen(screen_top_left);
    draw_string(screen_top_left, title, 10, 10, COLOR_TITLE);

    draw_string(screen_top_left, options[0], 10, 40, COLOR_SELECTED);
    for (int i = 1; i < count; i++) {
        draw_string(screen_top_left, options[i], 10, 40 + 10 * i, COLOR_IDLE);
    }

    while (1) {
        uint16_t key = wait_key();

        if (key == (key_released | key_up)) {
            draw_string(screen_top_left, options[current], 10, 40 + 10 * current, COLOR_IDLE);

            if (current <= 0) {
                current = count - 1;
            } else {
                current--;
            }

            draw_string(screen_top_left, options[current], 10, 40 + 10 * current, COLOR_SELECTED);
        } else if (key == (key_released | key_down)) {
            draw_string(screen_top_left, options[current], 10, 40 + 10 * current, COLOR_IDLE);

            if (current >= count - 1) {
                current = 0;
            } else {
                current++;
            }

            draw_string(screen_top_left, options[current], 10, 40 + 10 * current, COLOR_SELECTED);
        } else if (key == (key_released | key_a)) {
            return current;
        } else if (key == (key_released | key_b) && back) {
            return -1;
        }
    }
}
