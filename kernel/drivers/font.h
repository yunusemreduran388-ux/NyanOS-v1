#ifndef FONT_H
#define FONT_H

/* Returns the 7-row, 5-pixel-wide glyph pattern for a character.
   Each row uses '#' for a lit pixel and '.' for empty. Unknown
   characters fall back to a blank glyph. */
const char* const* font_get_glyph(char c);

#endif
