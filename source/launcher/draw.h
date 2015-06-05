#ifndef __draw_h__
#define __draw_h__

#ifdef ENTRY_MSET
enum screen {
    screen_top_left,
    screen_top_right,
    screen_bottom
};

void clear_screen(enum screen screen);
void clear_screens();
void draw_character(enum screen screen, char character, int pos_x, int pos_y);
void draw_string(enum screen screen, char *string, int pos_x, int pos_y);
void print(char *string);
#else
#define clear_screen(a)
#define clear_screens()
#define draw_character(a, b, c, d)
#define draw_string(a, b, c, d)
#define print(x)
#endif

#endif
