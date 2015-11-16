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

#ifndef _SIMTK_CONSOLE_H
#define _SIMTK_CONSOLE_H

#define SIMTK_CONSOLE_DEFAULT_BGCOLOR OPAQUE (0)
#define SIMTK_CONSOLE_DEFAULT_FGCOLOR OPAQUE (RGB (0xbf, 0xbf, 0xbf))

struct simtk_console_properties
{
  SDL_mutex *lock;

  uint32_t fgcolor;
  uint32_t bgcolor;

  int cur_x, cur_y;
  int off_x, off_y;
  int rows,  cols;
  
  char *buffer;
  int buffer_length;
  int blinkstat;
  
  void *opaque;
};

void simtk_console_properties_destroy (struct simtk_console_properties *);
struct simtk_console_properties *simtk_console_properties_new (int, int);
void simtk_console_properties_lock (struct simtk_console_properties *);
void simtk_console_properties_unlock (struct simtk_console_properties *);
struct simtk_console_properties *simtk_console_get_properties (const simtk_widget_t *);
void simtk_console_set_properties (simtk_widget_t *, struct simtk_console_properties *);
void *simtk_console_get_opaque (const simtk_widget_t *);
void simtk_console_set_opaque (simtk_widget_t *, void *);
int simtk_console_create (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);
int simtk_console_destroy (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);
int simtk_console_hearbeat (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);
void simtk_console_render (simtk_widget_t *);
void simtk_console_puts (simtk_widget_t *, const char *);
void simtk_console_vprintf (simtk_widget_t *, const char *, va_list);
void scputs (simtk_widget_t *, const char *);
void vscprintf (simtk_widget_t *, const char *, va_list);
void scprintf (simtk_widget_t *, const char *, ...);
void simtk_console_printf (simtk_widget_t *, const char *, ...);
simtk_widget_t *simtk_console_new (struct simtk_container *, int, int, int, int, int, int);

#endif /* _SIMTK_CONSOLE_H */
