#ifndef BROWSER_APP_H
#define BROWSER_APP_H

void app_browser_init(void);
void app_browser_render(int x, int y, int w, int h);
void app_browser_click(int x, int y, int w, int h, int mx, int my);
/* Loads a file from the in-memory file system as the current page. */
void app_browser_load(const char *filename);

#endif
