#pragma once

#include <stdint.h>

#define SCREEN_TOP_WIDTH 400
#define SCREEN_TOP_HEIGHT 240
#define SCREEN_BOTTOM_WIDTH 320
#define SCREEN_BOTTOM_HEIGHT 240
#define SCREEN_TOP_SIZE (SCREEN_TOP_WIDTH * SCREEN_TOP_HEIGHT * 3)
#define SCREEN_BOTTOM_SIZE (SCREEN_BOTTOM_WIDTH * SCREEN_BOTTOM_HEIGHT * 3)

#define SPACING_VERT 10
#define SPACING_HORIZ 8
#define MARGIN_LEFT 10
#define MARGIN_RIGHT 10
#define MARGIN_TOP 10
#define MARGIN_BOTTOM 10
#define MARGIN_HORIZ (MARGIN_LEFT + MARGIN_RIGHT)
#define MARGIN_VERT (MARGIN_TOP + MARGIN_BOTTOM)

enum screen {
    screen_top_left,
    screen_top_right,
    screen_bottom
};

extern enum screen print_screen;

void clear_screen(const enum screen screen);
void clear_screens();
void draw_character(const enum screen screen, const char character, const unsigned int pos_x, const unsigned int pos_y, const uint32_t color);
int draw_string(const enum screen screen, const char *string, const unsigned int pos_x, unsigned int pos_y, const uint32_t color);
void print(const char *string);
