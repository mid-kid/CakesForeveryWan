#ifndef __draw_h__
#define __draw_h__

#if defined(ENTRY_MSET) || defined(ARM9)

#include <stdint.h>

enum screen {
    screen_top_left,
    screen_top_right,
    screen_bottom
};

#if defined(ENTRY_MSET)
__attribute__((unused))
static enum screen *print_screen = (enum screen *)0x14A00001;

void draw_init();
#elif defined(ARM9)
__attribute__((unused))
static enum screen *print_screen = (enum screen *)0x20A00001;

void draw_init(uint32_t *data);
#endif

void clear_screen(enum screen screen);
void clear_screens();
void draw_character(enum screen screen, char character, int pos_x, int pos_y);
void draw_string(enum screen screen, char *string, int pos_x, int pos_y);
void print(char *string);
#else
#define draw_init(...)
#define clear_screen(...)
#define clear_screens()
#define draw_character(...)
#define draw_string(...)
#define print(...)
#endif

#endif
