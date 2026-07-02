#ifndef GUI_H
#define GUI_H

typedef enum {
    APP_NONE = 0,
    APP_ABOUT,
    APP_TERMINAL,
    APP_NOTEPAD,
    APP_PAINT,
    APP_BROWSER,
    APP_COUNT
} app_id_t;

void gui_init(void);
void gui_run(void); /* main GUI loop - never returns */

/* Used by apps (e.g. the terminal's "open"/"notepad"/"paint" commands
   and the taskbar/start menu) to show and focus a given app window. */
void gui_open_app(app_id_t id);
void gui_close_app(app_id_t id);

/* Lets the terminal's "browser <file>" command pick a page. */
void gui_browser_load(const char *filename);

#endif
