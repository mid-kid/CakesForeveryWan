#pragma once

#define MAX_SELECTED_OPTIONS 0x10

#define COLOR_TITLE 0x0000FF
#define COLOR_NEUTRAL 0xFFFFFF
#define COLOR_SELECTED 0xFF0000
#define COLOR_BACKGROUND 0x000000

int draw_menu(const char *title, int back, int count, char *options[]);
int *draw_selection_menu(const char *title, int count, char *options[], const int *preselected);
int draw_loading(const char *title, const char *text);
void draw_message(const char *title, const char *text);
