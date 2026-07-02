#ifndef NOTEPAD_APP_H
#define NOTEPAD_APP_H

void app_notepad_init(void);
void app_notepad_render(int x, int y, int w, int h);
void app_notepad_key(char c);
/* Returns 1 if the click at (mx,my) (window-relative) was consumed
   by a notepad UI element (e.g. the Save button). */
int  app_notepad_click(int x, int y, int w, int h, int mx, int my);

#endif
