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

#ifndef _SIMTK_ENTRY_H
#define _SIMTK_ENTRY_H

#define SIMTK_ENTRY_DEFAULT_BGCOLOR OPAQUE (0)
#define SIMTK_ENTRY_DEFAULT_FGCOLOR OPAQUE (RGB (0xff, 0x7f, 0))

#define SIMTK_ENTRY_DEFAULT_SEL_BGCOLOR SIMTK_ENTRY_DEFAULT_FGCOLOR
#define SIMTK_ENTRY_DEFAULT_SEL_FGCOLOR SIMTK_ENTRY_DEFAULT_BGCOLOR
#define SIMTK_ENTRY_DEFAULT_BORDERCOLOR SIMTK_ENTRY_DEFAULT_FGCOLOR

#define SIMTK_ENTRY_DEFAULT_CUR_BGCOLOR OPAQUE (RGB (0, 0xff, 0))
#define SIMTK_ENTRY_DEFAULT_CUR_FGCOLOR OPAQUE (RGB (0, 0, 0))

enum simtk_entry_cursor_type
{
  SIMTK_ENTRY_CURSOR_BAR,
  SIMTK_ENTRY_CURSOR_BLOCK,
  SIMTK_ENTRY_CURSOR_UNDERSCORE
};

struct simtk_entry_properties
{
  SDL_mutex *lock;

  char *buffer;
  
  int buffer_length;
  int text_length;
  
  int autogrow;
  int cursor;
  int text_offset;
  int sel_start;
  int sel_length;
  int blinkstat;
  int blinking;
  
  uint32_t bgcolor, fgcolor;
  uint32_t sel_bgcolor, sel_fgcolor;
  uint32_t cur_bgcolor, cur_fgcolor;
  uint32_t bordercolor;

  char *(*textfilter) (const char *);
  
  int mark;
  
  void *opaque;
};

struct simtk_entry_properties *simtk_entry_properties_new (int, const char *text);
void simtk_entry_properties_lock (const struct simtk_entry_properties *);
void simtk_entry_properties_unlock (const struct simtk_entry_properties *);
void simtk_entry_properties_destroy (struct simtk_entry_properties *);

struct simtk_entry_properties *simtk_entry_get_properties (const simtk_widget_t *);

void *simtk_entry_get_opaque (const simtk_widget_t *);
void  simtk_entry_set_opaque (simtk_widget_t *, void *);
void  simtk_entry_render (simtk_widget_t *);

int   simtk_entry_create (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);
int   simtk_entry_destroy (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);

void  simtk_entry_select (simtk_widget_t *, int, int);
void  simtk_entry_set_cursor (simtk_widget_t *, int);
int   simtk_entry_get_cursor (simtk_widget_t *);
int   simtk_entry_insert_text (simtk_widget_t *, const char *);
int   simtk_entry_insert_char (simtk_widget_t *, char);
void  simtk_entry_remove_selected (simtk_widget_t *);
void  simtk_entry_set_textfilter (simtk_widget_t *, char *(*) (const char *));
char *simtk_entry_get_text (simtk_widget_t *);
void  simtk_entry_disable_blinking (simtk_widget_t *);
void  simtk_entry_enable_blinking (simtk_widget_t *);
void  simtk_entry_clear (simtk_widget_t *);

simtk_widget_t *simtk_entry_new (struct simtk_container *, int, int, int);

#endif /* _SIMTK_ENTRY_H */
