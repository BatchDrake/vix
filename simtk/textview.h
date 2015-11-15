/*
  
  Copyright (C) 2014 Gonzalo José Carracedo Carballal
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#ifndef _SIMTK_TEXTVIEW_H
#define _SIMTK_TEXTVIEW_H

struct simtk_textview_properties
{
  SDL_mutex *lock;

  /* cpi_disp_font_from_display (display_from_simtk_widget (widget)) */
  struct cpi_disp_font *font;
  
  int render_x, render_y;
  int rows, cols;

  char *text;
  uint32_t *fore;
  uint32_t *back;

  void *opaque;
};

void simtk_textview_set_opaque (struct simtk_widget *, void *);
void *simtk_textview_get_opaque (const struct simtk_widget *);
void simtk_textview_render_text_noflip (struct simtk_widget *);
void simtk_textview_render_text (struct simtk_widget *);
void simtk_textview_set_text (struct simtk_widget *, int, int, uint32_t, uint32_t, const void *, size_t);
void simtk_textview_repeat (struct simtk_widget *, int, int, uint32_t, uint32_t, char, size_t);
void simtk_textview_properties_lock (const struct simtk_textview_properties *);
void simtk_textview_properties_unlock (const struct simtk_textview_properties *);
struct simtk_widget *simtk_textview_new (struct simtk_container *, int, int, int, int);
struct simtk_textview_properties *simtk_textview_get_properties (const struct simtk_widget *);

#endif /* _SIMTK_TEXTVIEW_H */
