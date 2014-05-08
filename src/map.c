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

#include <vix.h>
#include <simtk/simtk.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include "map.h"
#include "hexview.h"

PTR_LIST (struct filemap, map);

static int drag_flag;
static int drag_start_x;
static int drag_start_y;
static int moves;

int last_id;

void
filemap_jump_to_offset (struct filemap *map, uint32_t offset)
{
  map->offset = offset;
  
  simtk_hexview_scroll_to_noflip (map->hexwid, offset);
  simtk_bitview_scroll_to_noflip (map->vwid, offset, 0x200);
  simtk_bitview_scroll_to_noflip (map->hwid, offset, 0x200);

  simtk_widget_switch_buffers (map->hwid);
  simtk_widget_switch_buffers (map->vwid);
  simtk_widget_switch_buffers (map->hexwid);
}

int
generic_onkeydown (enum simtk_event_type type,
		   struct simtk_widget *widget,
		   struct simtk_event *event,
		   struct filemap *map)
{
  int delta = 0;
  uint32_t offset = map->offset;
  
  switch (event->button)
  {
  case SDLK_UP:
    delta = -128;
    break;

  case SDLK_DOWN:
    delta = 128;
    break;

  case SDLK_PAGEUP:
    delta = -0x3e00;
    break;

  case SDLK_PAGEDOWN:
    delta = 0x3e00;
    break;
  }

  if (delta != 0)
  {
    if ((int) offset + delta < 0)
      offset = 0;
    else if (offset + (uint32_t) delta >= map->size)
    {
      if ((int) map->size - 0x200 < 0)
	offset = 0;
      else
	offset = map->size - 0x200;
    }
    else
      offset += delta;

    filemap_jump_to_offset (map, offset);
  }
}
  
  
int
bitview_onkeydown (enum simtk_event_type type,
		   struct simtk_widget *widget,
		   struct simtk_event *event)
{
  return generic_onkeydown (type, widget, event, (struct filemap *) simtk_bitview_get_opaque (widget));
}

int
hexview_onkeydown (enum simtk_event_type type,
		   struct simtk_widget *widget,
		   struct simtk_event *event)
{
  struct filemap *map;
  
  return generic_onkeydown (type, widget, event, (struct filemap *) simtk_hexview_get_opaque (widget));
}


int
generic_drag_onmousemove (enum simtk_event_type type,
			  struct simtk_widget *widget,
			  struct simtk_event *event)
{
  if (drag_flag && !simtk_is_drawing ())
  { 
    simtk_widget_move (widget,
		       event->x + widget->x - drag_start_x,
		       event->y + widget->y - drag_start_y);
  

    simtk_set_redraw_pending ();
  }
  
  return HOOK_RESUME_CHAIN;
}

int
generic_drag_onmousedown (enum simtk_event_type type,
	  struct simtk_widget *widget,
	  struct simtk_event *event)
{
  drag_flag = 1;
  
  drag_start_x = event->x;
  drag_start_y = event->y;

  return HOOK_RESUME_CHAIN;
}


int
generic_drag_onmouseup (enum simtk_event_type type,
	  struct simtk_widget *widget,
	  struct simtk_event *event)
{
  if (drag_flag)
  {
    drag_flag = 0;

    simtk_set_redraw_pending ();
  }
  
  return HOOK_RESUME_CHAIN;
}

void
simtk_widget_make_draggable (struct simtk_widget *widget)
{
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEMOVE, generic_drag_onmousemove);
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEDOWN, generic_drag_onmousedown);
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEUP, generic_drag_onmouseup);
}

int
filemap_open_views (struct filemap *map)
{
  struct simtk_bitview_properties *prop;
  struct simtk_widget *widget;
  
  char *name;
  char title[256];
  
  if ((name = strrchr (map->path, '/')) == NULL)
    name = map->path;
  else
    ++name;
  
  snprintf (title, 255, "Hexdump: %s (%d bytes)", name, map->size);
  
  if ((map->hexview = simtk_window_new (map->container, (last_id % 4) * 15 + 300, (last_id % 4) * 15 + 10, 640 - 8 * 5 - 4, 480, title)) == NULL)
    return -1;

  snprintf (title, 255, "Bit rows: %s", name);
  
  if ((map->hbits = simtk_window_new (map->container, (last_id % 4) * 15 + 10, (last_id % 4) * 15 + 10, 258, 524, title)) == NULL)
    return -1;

  snprintf (title, 255, "Byte cols: %s", name);
  
  if ((map->vbits = simtk_window_new (map->container, (last_id % 4) * 15 + 10, (last_id % 4) * 15 + 600, 1026, 140, title)) == NULL)
    return -1;

  last_id++;
  
  simtk_widget_make_draggable (map->hexview);
  simtk_widget_make_draggable (map->vbits);
  simtk_widget_make_draggable (map->hbits);
  
  if ((map->hexwid = widget = simtk_hexview_new (simtk_window_get_body_container (map->hexview),
						 1, 1, 80, 80, 0, map->base, map->size)) == NULL)
    return -1;

  simtk_event_connect (map->hexwid, SIMTK_EVENT_KEYDOWN, hexview_onkeydown);
  
  simtk_hexview_set_opaque (map->hexwid, map);
			   

  if ((map->vwid = widget = simtk_bitview_new (simtk_window_get_body_container (map->vbits), 0, 0, 128, 64 * 16, SIMTK_HORIZONTAL, SIMTK_VERTICAL, map->base, map->size, 512)) == NULL)
    return -1;

  simtk_event_connect (map->vwid, SIMTK_EVENT_KEYDOWN, bitview_onkeydown);

  simtk_bitview_set_opaque (map->vwid, map);
  
  prop = simtk_bitview_get_properties (widget);

  prop->background = OPAQUE (0);


  if ((map->hwid = widget = simtk_bitview_new (simtk_window_get_body_container (map->hbits), 0, 0, 512, 64 * 4, SIMTK_HORIZONTAL, SIMTK_HORIZONTAL, map->base, map->size, 512)) == NULL)
    return -1;

  simtk_event_connect (map->hwid, SIMTK_EVENT_KEYDOWN, bitview_onkeydown);

  simtk_bitview_set_opaque (map->hwid, map);
  
  prop = simtk_bitview_get_properties (widget);

  prop->color_lsb = OPAQUE (0x4f3300);
  prop->color_msb = OPAQUE (0xbf7b00);

  simtk_bitview_render_bits (widget);
  
  prop->background = OPAQUE (0);

  return 0;
}

struct filemap *
filemap_new (struct simtk_container *container, const char *path)
{
  struct filemap *new = NULL;
  void *base = (caddr_t) -1;
  int fd = -1;
  uint32_t size = 0;
  
  if ((fd = open (path, O_RDONLY)) == -1)
    goto fail;
  
  size = lseek (fd, 0, SEEK_END);
  
  base = mmap (NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);

  close (fd);

  if (base == (caddr_t) -1)
    goto fail;
  
  if ((new = calloc (1, sizeof (struct filemap))) == NULL)
    goto fail;

  if ((new->path = strdup (path)) == NULL)
    goto fail;
  
  new->container = container;
  new->base = base;
  new->size = size;
  
  if (filemap_open_views (new) == -1)
    goto fail;

  return new;
  
fail:
  if (fd != -1)
    close (fd);

  if (new != NULL)
    filemap_destroy (new);
  else  if (base != (caddr_t) -1)
    munmap (base, size);

  return NULL;
}


void
filemap_destroy (struct filemap *filemap)
{
  if (filemap->base != NULL)
    munmap (filemap->base, filemap->size);

  if (filemap->hexview != NULL)
    simtk_widget_destroy (filemap->hexview);

  if (filemap->vbits != NULL)
    simtk_widget_destroy (filemap->vbits);

  if (filemap->hbits != NULL)
    simtk_widget_destroy (filemap->hbits);

  if (filemap->path != NULL)
    free (filemap->path);
  
  free (filemap);
}

int
vix_open_file (const char *path)
{
  struct filemap *map;
  int id;
  
  if ((map = filemap_new (simtk_get_root_container (), path)) == NULL)
    return -1;
  
  if ((id = PTR_LIST_APPEND_CHECK (map, map)) == -1)
  {
    filemap_destroy (map);
    return -1;
  }

  vix_open_file_hook_run (id);
  
  return 0;
}

void
vix_close_all_files (void)
{
  int i;

  for (i = 0; i < map_count; ++i)
    if (map_list[i] != NULL)
      filemap_destroy (map_list[i]);
  
  if (map_list != NULL)
    free (map_list);
}
