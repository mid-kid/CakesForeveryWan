#include "menu.h"

#include <stdint.h>
#include <stddef.h>
#include "memfuncs.h"
#include "draw.h"
#include "hid.h"

int selected_options[MAX_SELECTED_OPTIONS];

// TODO: Clean up the first two functions. They have too much in common.
// No boundary checks, use this responsibly.
int draw_menu(const char *title, int back, int count, char *options[])
{
    int current = 0;
    int pos_y_text[count];
    int current_pos_y = 40;

    clear_screen(screen_top_left);
    draw_string(screen_top_left, title, 10, 10, COLOR_TITLE);

    pos_y_text[0] = current_pos_y;
    current_pos_y = draw_string(screen_top_left, options[0], 10, current_pos_y, COLOR_SELECTED);
    for (int i = 1; i < count; i++) {
        current_pos_y += SPACING_VERT;
        pos_y_text[i] = current_pos_y;
        current_pos_y = draw_string(screen_top_left, options[i], 10, current_pos_y, COLOR_NEUTRAL);
    }

    while (1) {
        uint16_t key = wait_key();

        if (key == (key_released | key_up)) {
            draw_string(screen_top_left, options[current], 10, pos_y_text[current], COLOR_NEUTRAL);

            if (current <= 0) {
                current = count - 1;
            } else {
                current--;
            }

            draw_string(screen_top_left, options[current], 10, pos_y_text[current], COLOR_SELECTED);
        } else if (key == (key_released | key_down)) {
            draw_string(screen_top_left, options[current], 10, pos_y_text[current], COLOR_NEUTRAL);

            if (current >= count - 1) {
                current = 0;
            } else {
                current++;
            }

            draw_string(screen_top_left, options[current], 10, pos_y_text[current], COLOR_SELECTED);
        } else if (key == (key_released | key_a)) {
            return current;
        } else if (key == (key_released | key_b) && back) {
            return -1;
        }
    }
}

int *draw_selection_menu(const char *title, int count, char *options[], const int *preselected)
{
    // The caller has to make sure it does not exceed MAX_SELECTED_OPTIONS
    if (count > MAX_SELECTED_OPTIONS) {
        return NULL;
    }

    memset32(selected_options, 0, sizeof(selected_options));

    int current = 0;
    int pos_x_text = 10 + 4 * SPACING_HORIZ;
    int pos_y_text[count];
    int current_pos_y = 40;

    clear_screen(screen_top_left);
    draw_string(screen_top_left, title, 10, 10, COLOR_TITLE);

    pos_y_text[0] = current_pos_y;
    draw_string(screen_top_left, "[ ]", 10, current_pos_y, COLOR_NEUTRAL);
    current_pos_y = draw_string(screen_top_left, options[0], pos_x_text, current_pos_y, COLOR_SELECTED);
    int i;
    for (i = 1; i < count; i++) {
        current_pos_y += SPACING_VERT;
        pos_y_text[i] = current_pos_y;
        draw_string(screen_top_left, "[ ]", 10, current_pos_y, COLOR_NEUTRAL);
        current_pos_y = draw_string(screen_top_left, options[i], pos_x_text, current_pos_y, COLOR_NEUTRAL);
    }
    draw_string(screen_top_left, "Press START to confirm", 10, current_pos_y + SPACING_VERT * 2, COLOR_SELECTED);

    for (int i = 0; i < count; i++) {
        if (preselected[i]) {
            draw_character(screen_top_left, 'x', 10 + SPACING_HORIZ, pos_y_text[i], COLOR_NEUTRAL);
            selected_options[i] = 1;
        }
    }

    while (1) {
        uint16_t key = wait_key();

        if (key == (key_released | key_up)) {
            draw_string(screen_top_left, options[current], pos_x_text, pos_y_text[current], COLOR_NEUTRAL);

            if (current <= 0) {
                current = count - 1;
            } else {
                current--;
            }

            draw_string(screen_top_left, options[current], pos_x_text, pos_y_text[current], COLOR_SELECTED);
        } else if (key == (key_released | key_down)) {
            draw_string(screen_top_left, options[current], pos_x_text, pos_y_text[current], COLOR_NEUTRAL);

            if (current >= count - 1) {
                current = 0;
            } else {
                current++;
            }

            draw_string(screen_top_left, options[current], pos_x_text, pos_y_text[current], COLOR_SELECTED);
        } else if (key == (key_released | key_a)) {
            if (selected_options[current]) {
                draw_character(screen_top_left, 'x', 10 + SPACING_HORIZ, pos_y_text[current], COLOR_BACKGROUND);
                selected_options[current] = 0;
            } else {
                draw_character(screen_top_left, 'x', 10 + SPACING_HORIZ, pos_y_text[current], COLOR_NEUTRAL);
                selected_options[current] = 1;
            }
        } else if (key == (key_released | key_start) || key == (key_released | key_b)) {
            return selected_options;
        }
    }
}

int draw_loading(const char *title, const char *text)
{
    clear_screen(screen_top_left);
    draw_string(screen_top_left, title, 10, 10, COLOR_TITLE);
    return draw_string(screen_top_left, text, 10, 40, COLOR_NEUTRAL);
}

void draw_message(const char *title, const char *text)
{
    int pos_y = draw_loading(title, text);

    draw_string(screen_top_left, "Press A to continue", 10, pos_y + 20, COLOR_SELECTED);

    while (1) {
        uint16_t key = wait_key();

        if (key == (key_released | key_a)) {
            return;
        }
    }
}
