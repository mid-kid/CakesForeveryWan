#pragma once

#define MAX_SELECTED_OPTIONS 10

int draw_menu(const char *title, int back, int count, char *options[]);
int *draw_selection_menu(const char *title, int count, char *options[], const int *preselected);
int draw_loading(const char *title, const char *text);
void draw_message(const char *title, const char *text);
