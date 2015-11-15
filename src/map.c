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
#include <SDL/SDL_thread.h>

#include "map.h"
#include "hexview.h"
#include "console.h"
#include "region.h"

PTR_LIST (struct filemap, map);

static int drag_flag;
static int drag_start_x;
static int drag_start_y;
static int drag_forced;

int last_id;

extern struct simtk_widget *console;

static int search_flag;

#define MAX_SEARCH_RESULTS 1000

struct filemap_search_data
{
  struct filemap *map;
  void *data;
  size_t size;
};

SDL_Thread *search_thread;

int
filemap_search_thread (void *data)
{
  struct filemap_search_data *sdata = (struct filemap_search_data *) data;
  int n = 0;
  uint32_t offset = 0;
  uint32_t first  = 0;
  
  scprintf (console, "Please wait, searching %d bytes in a file of %d bytes...\n", sdata->size, sdata->map->size);

  simtk_hexview_clear_regions (sdata->map->hexwid);
  simtk_bitview_clear_regions (sdata->map->hwid);
  simtk_bitview_clear_regions (sdata->map->vwid);
  
  while (offset < sdata->map->size - sdata->size && n < MAX_SEARCH_RESULTS)
  {
    if (memcmp (sdata->map->base + offset, sdata->data, sdata->size) == 0)
    {
      if (n == 0)
	first = offset;
      simtk_bitview_mark_region_noflip (sdata->map->hwid, "Search result", offset, sdata->size, OPAQUE (RGB (0, 255, 255)), OPAQUE (0));
      simtk_bitview_mark_region_noflip (sdata->map->vwid, "Search result", offset, sdata->size, OPAQUE (RGB (0, 255, 255)), OPAQUE (0));
      simtk_hexview_mark_region_noflip (sdata->map->hexwid, "Search result", offset, sdata->size, OPAQUE (RGB (0, 255, 255)), OPAQUE (0));
      
      scprintf (console, "  Result found at %p!\n", offset);
      offset += sdata->size;
      ++n;

    }
    else
      ++offset;
  }
  
  if (n > 0)
  {
    simtk_widget_switch_buffers (sdata->map->vwid);
    simtk_widget_switch_buffers (sdata->map->hwid);
    simtk_widget_switch_buffers (sdata->map->hexwid);

    sdata->map->search_offset = first;
    
    filemap_jump_to_offset (sdata->map, (first >> 4) << 4);
    
    scprintf (console, "%d results total\n", n);
  }
  else
    scprintf (console, "No results found.\n");

  search_flag = 0;

  free (sdata->data);
  
  return 0;
}

void
filemap_search (struct filemap *map, const void *data, size_t size)
{
  static struct filemap_search_data sdata;
  void *dup;
  
  if (search_flag)
  {
    scprintf (console, "Already searching, please wait...\n");
    return;
  }

  sdata.map = map;
  sdata.size = size;
  
  if ((dup = malloc (size)) == NULL)
  {
    scprintf (console, "No memory left!\n");
    return;
  }

  memcpy (dup, data, size);

  sdata.data = dup;

  search_flag = 1;
  
  if ((search_thread = SDL_CreateThread (filemap_search_thread, &sdata)) == NULL)
  {
    scprintf (console, "No memory left to create thread!\n");

    search_flag = 0;
    free (dup);
    
    return;
  }
}

void
filemap_jump_to_offset (struct filemap *map, uint32_t offset)
{
  int i;
  
  map->offset = offset;
  
  simtk_hexview_scroll_to_noflip (map->hexwid, offset);
  simtk_bitview_scroll_to_noflip (map->vwid, offset, 0x200);
  simtk_bitview_scroll_to_noflip (map->hwid, offset, 0x200);

  for (i = 0; i < map->hilbert_widget_count; ++i)
    if (map->hilbert_widget_list[i] != NULL)
      simtk_hilbert_scroll_to_noflip (map->hilbert_widget_list[i], offset);
  
  simtk_widget_switch_buffers (map->hwid);
  simtk_widget_switch_buffers (map->vwid);
  simtk_widget_switch_buffers (map->hexwid);
  
  for (i = 0; i < map->hilbert_widget_count; ++i)
    if (map->hilbert_widget_list[i] != NULL)
      simtk_widget_switch_buffers (map->hilbert_widget_list[i]);
}

void
filemap_scroll (struct filemap *map, int delta)
{
  uint32_t offset = map->offset;
    
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

    map->search_offset = offset;
    
    filemap_jump_to_offset (map, offset);
  }
}

int
generic_onkeyup (enum simtk_event_type type,
                   struct simtk_widget *widget,
                   struct simtk_event *event,
                   struct filemap *map)
{
  switch (event->button)
  {
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      drag_forced = 0;
      break;
  }

  return HOOK_RESUME_CHAIN;
}

int
generic_onkeydown (enum simtk_event_type type,
		   struct simtk_widget *widget,
		   struct simtk_event *event,
		   struct filemap *map)
{
  int delta = 0;
  struct rbtree_node *node = NULL;
  struct simtk_hexview_properties *prop;
  struct file_region *region;
  
  prop = simtk_hexview_get_properties (map->hexwid);
  
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

  /* This should be somewhere else */
  case SDLK_LCTRL:
  case SDLK_RCTRL:
    drag_forced = 1;
    break;

  case 'n':
    simtk_hexview_properties_lock (prop);
    
    if ((node = file_region_find (prop->regions, map->search_offset)) != NULL)
      node = rbtree_node_next (node);

    simtk_hexview_properties_lock (prop);
    break;
    
  case 'p':
    simtk_hexview_properties_lock (prop);
    
    if ((node = file_region_find (prop->regions, map->search_offset)) != NULL)
      node = rbtree_node_prev (node);

    simtk_hexview_properties_lock (prop);
    break;
  }

  filemap_scroll (map, delta);
  
  if (node != NULL)
  {
    region = (struct file_region *) node;

    map->search_offset = region->start;
    
    filemap_jump_to_offset (map, (region->start >> 4) << 4);
  }

  return HOOK_RESUME_CHAIN;
}
  
int
hilbert_onkeydown (enum simtk_event_type type,
		   struct simtk_widget *widget,
		   struct simtk_event *event)
{
  return generic_onkeydown (type, widget, event, (struct filemap *) simtk_hilbert_get_opaque (widget));
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
hilbert_onkeyup (enum simtk_event_type type,
                   struct simtk_widget *widget,
                   struct simtk_event *event)
{
  return generic_onkeyup (type, widget, event, (struct filemap *) simtk_hilbert_get_opaque (widget));
}

int
bitview_onkeyup (enum simtk_event_type type,
                   struct simtk_widget *widget,
                   struct simtk_event *event)
{
  return generic_onkeyup (type, widget, event, (struct filemap *) simtk_bitview_get_opaque (widget));
}

int
hexview_onkeyup (enum simtk_event_type type,
                   struct simtk_widget *widget,
                   struct simtk_event *event)
{
  struct filemap *map;

  return generic_onkeyup (type, widget, event, (struct filemap *) simtk_hexview_get_opaque (widget));
}


static inline int
simtk_widget_can_drag_at (struct simtk_widget *widget, int x, int y)
{
  int i;

  if (drag_forced)
    return 1;

  x += widget->x;
  y += widget->y;

  for (i = 0; i < widget->container_count; ++i)
    if (widget->container_list[i] != NULL)
      if (x >= widget->container_list[i]->x && y >= widget->container_list[i]->y &&
          x < (widget->container_list[i]->x + widget->container_list[i]->width) &&
          y < (widget->container_list[i]->y + widget->container_list[i]->height))
        return 0;

  return 1;
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
  int button = event->button;
  struct filemap *map = NULL;

  if (button == SIMTK_MOUSE_BUTTON_LEFT) {
    if (simtk_widget_can_drag_at (
        widget,
        event->x,
        event->y))
    {
      drag_flag = 1;

      drag_start_x = event->x;
      drag_start_y = event->y;
    }
  } else {
    map = (struct filemap *) simtk_window_get_opaque (widget);

    if (map) {
      if (button == SIMTK_MOUSE_BUTTON_UP) {
        filemap_scroll (map, -1024);
      } else if (button == SIMTK_MOUSE_BUTTON_DOWN) {
        filemap_scroll (map, 1024);
      }
    }
  }

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
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEUP,   generic_drag_onmouseup);
}

int
filemap_open_views (struct filemap *map)
{
  struct simtk_bitview_properties *prop;
  struct simtk_widget *widget;
  int vwidsize, hwidsize;
  
  char *name;
  char title[256];
  
  if ((name = strrchr (map->path, '/')) == NULL)
    name = map->path;
  else
    ++name;

  vwidsize = 64 * 16;

  if (__UNITS (map->size, 16) < vwidsize)
    vwidsize = __UNITS (map->size, 16);
  

  hwidsize = 512;

  if (__UNITS (map->size, 32) < hwidsize)
    hwidsize = __UNITS (map->size, 32);

  snprintf (title, 255, "Hexdump: %s (%d bytes)", name, map->size);
  
  if ((map->hexview = simtk_window_new (map->container, (last_id % 4) * 15 + 300, (last_id % 4) * 15 + 10, 640 - 8 * 5 - 4, 480, title)) == NULL)
    return -1;

  snprintf (title, 255, "Bit rows: %s", name);
  
  if ((map->hbits = simtk_window_new (map->container, (last_id % 4) * 15 + 10, (last_id % 4) * 15 + 10, 258, hwidsize + 12, title)) == NULL)
    return -1;

  snprintf (title, 255, "Byte cols: %s", name);
  
  if ((map->vbits = simtk_window_new (map->container, (last_id % 4) * 15 + 10, (last_id % 4) * 15 + 600, vwidsize + 2, 140, title)) == NULL)
    return -1;

  last_id++;
  
  simtk_widget_make_draggable (map->hexview);
  simtk_widget_make_draggable (map->vbits);
  simtk_widget_make_draggable (map->hbits);

  simtk_window_set_opaque (map->hexview, map);
  simtk_window_set_opaque (map->vbits, map);
  simtk_window_set_opaque (map->hbits, map);
  
  if ((map->hexwid = widget = simtk_hexview_new (simtk_window_get_body_container (map->hexview),
						 1, 1, 80, 80, 0, map->base, map->size)) == NULL)
    return -1;

  simtk_event_connect (map->hexwid, SIMTK_EVENT_KEYDOWN, hexview_onkeydown);
  simtk_event_connect (map->hexwid, SIMTK_EVENT_KEYUP,   hexview_onkeyup);
  
  simtk_hexview_set_opaque (map->hexwid, map);
			   
  if ((map->vwid = widget = simtk_bitview_new (simtk_window_get_body_container (map->vbits), 0, 0, 128, vwidsize, SIMTK_HORIZONTAL, SIMTK_VERTICAL, map->base, map->size, 512)) == NULL)
    return -1;

  simtk_event_connect (map->vwid, SIMTK_EVENT_KEYDOWN, bitview_onkeydown);
  simtk_event_connect (map->vwid, SIMTK_EVENT_KEYUP,   bitview_onkeyup);

  simtk_bitview_set_opaque (map->vwid, map);
  
  prop = simtk_bitview_get_properties (widget);

  prop->background = OPAQUE (0);

  if ((map->hwid = widget = simtk_bitview_new (simtk_window_get_body_container (map->hbits), 0, 0, hwidsize, 64 * 4, SIMTK_HORIZONTAL, SIMTK_HORIZONTAL, map->base, map->size, 512)) == NULL)
    return -1;

  simtk_event_connect (map->hwid, SIMTK_EVENT_KEYDOWN, bitview_onkeydown);
  simtk_event_connect (map->hwid, SIMTK_EVENT_KEYUP,   bitview_onkeyup);

  simtk_bitview_set_opaque (map->hwid, map);
  
  prop = simtk_bitview_get_properties (widget);

  prop->color_lsb = OPAQUE (0x4f3300);
  prop->color_msb = OPAQUE (0xbf7b00);

  simtk_bitview_render_bits (widget);
  
  prop->background = OPAQUE (0);

  return 0;
}

int
filemap_open_hilbert (struct filemap *map, int power)
{
  struct simtk_widget *window, *widget;
  char title[256];
  int resolution = 1 << power;
  
  if (power < HILBERT_MIN_POWER || power > HILBERT_MAX_POWER)
    return -1;
  
  snprintf (title, sizeof (title) - 1, "2^%d Hilbert of %s", power, map->path);
  
  if ((window = simtk_window_new (map->container, (last_id % 4) * 15 + 300, (last_id % 4) * 15 + 10, resolution, resolution, title)) == NULL)
    return -1;

  if ((widget = simtk_hilbert_new (simtk_window_get_body_container (window), 0, 0, power, map->base, map->size)) == NULL)
  {
    simtk_widget_destroy (window);
    return -1;
  }

  simtk_widget_make_draggable (window);
  simtk_window_set_opaque (window, map); /* Link window to its map */
  simtk_hilbert_set_opaque (widget, map); /* Same with the widget itself */

  simtk_event_connect (widget, SIMTK_EVENT_KEYDOWN, hilbert_onkeydown);
  
  
  if (PTR_LIST_APPEND_CHECK (map->hilbert_window, window) == -1)
  {
    simtk_widget_destroy (window);
    simtk_widget_destroy (widget);

    return -1;
  }
  
  if (PTR_LIST_APPEND_CHECK (map->hilbert_widget, widget) == -1)
  {
    PTR_LIST_REMOVE (map->hilbert_window, window);
    
    simtk_widget_destroy (window);
    simtk_widget_destroy (widget);

    return -1;
  }
  
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
  int i;
  
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

  for (i = 0; i < filemap->hilbert_window_count; ++i)
    if (filemap->hilbert_window_list[i] != NULL)
      simtk_widget_destroy (filemap->hilbert_window_list[i]);

  if (filemap->hilbert_window_list != NULL)
    free (filemap->hilbert_window_list);

  if (filemap->hilbert_widget_list != NULL)
    free (filemap->hilbert_widget_list);
  
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
