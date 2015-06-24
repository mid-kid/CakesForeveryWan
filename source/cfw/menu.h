#ifndef __menu_h__
#define __menu_h__

#define MAX_SELECTED_OPTIONS 10

int draw_menu(char *title, int back, int count, char *options[]);
int *draw_selection_menu(char *title, int count, char *options[], int *preselected);
int draw_loading(char *title, char *text);
void draw_message(char *title, char *text);

#endif
