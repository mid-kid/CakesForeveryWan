#ifndef __draw_h__
#define __draw_h__

#include <stdint.h>

enum screen {
    screen_top_left,
    screen_top_right,
    screen_bottom
};

enum screen print_screen;

void draw_init(uint32_t *data);
void clear_screen(enum screen screen);
void clear_screens();
void draw_character(enum screen screen, char character, int pos_x, int pos_y, uint32_t color);
int draw_string(enum screen screen, const char *string, int pos_x, int pos_y, uint32_t color);
void print(const char *string);

#endif
