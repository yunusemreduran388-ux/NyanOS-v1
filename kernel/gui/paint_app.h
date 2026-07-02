#ifndef PAINT_APP_H
#define PAINT_APP_H

void app_paint_init(void);
void app_paint_render(int x, int y, int w, int h);
/* Called every frame with the mouse position (screen coords) and
   whether the left button is currently held down. Window-relative
   clipping/hit-testing happens inside. */
void app_paint_mouse(int x, int y, int w, int h, int mx, int my, int left_down);

#endif
