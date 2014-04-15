#include <vix.h>
#include <simtk/simtk.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include "map.h"
#include "hexview.h"

static int drag_flag;
static int drag_start_x;
static int drag_start_y;
static int moves;

int
generic_onkeydown (enum simtk_event_type type,
		   struct simtk_widget *widget,
		   struct simtk_event *event,
		   struct filemap *map)
{
  int delta = 0;

  switch (event->button)
  {
  case SDLK_UP:
    delta = -128;
    break;

  case SDLK_DOWN:
    delta = 128;
    break;

  case SDLK_PAGEUP:
    delta = -0x200;
    break;

  case SDLK_PAGEDOWN:
    delta = 0x200;
    break;
  }

  if (delta != 0)
  {
    if ((int) map->offset + delta < 0)
      map->offset = 0;
    else if (map->offset + (uint32_t) delta >= map->size)
    {
      if ((int) map->size - 0x200 < 0)
	map->offset = 0;
      else
	map->offset = map->size - 0x200;
    }
    else
      map->offset += delta;

    simtk_hexview_scroll_to (map->hexwid, map->offset);
    simtk_bitview_scroll_to (map->vwid, map->offset, 0x200);
    simtk_bitview_scroll_to (map->hwid, map->offset, 0x200);

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
  
  if ((map->hexview = simtk_window_new (map->container, 10, 10, 640 - 8 * 5 - 4, 480, title)) == NULL)
    return -1;

  snprintf (title, 255, "Bit rows: %s", name);
  
  if ((map->hbits = simtk_window_new (map->container, 20, 20, 258, 524, title)) == NULL)
    return -1;

  snprintf (title, 255, "Byte cols: %s", name);
  
  if ((map->vbits = simtk_window_new (map->container, 30, 30, 1026, 140, title)) == NULL)
    return -1;

  simtk_event_connect (map->hexview, SIMTK_EVENT_MOUSEMOVE, generic_drag_onmousemove);
  simtk_event_connect (map->hexview, SIMTK_EVENT_MOUSEDOWN, generic_drag_onmousedown);
  simtk_event_connect (map->hexview, SIMTK_EVENT_MOUSEUP, generic_drag_onmouseup);

  simtk_event_connect (map->vbits, SIMTK_EVENT_MOUSEMOVE, generic_drag_onmousemove);
  simtk_event_connect (map->vbits, SIMTK_EVENT_MOUSEDOWN, generic_drag_onmousedown);
  simtk_event_connect (map->vbits, SIMTK_EVENT_MOUSEUP, generic_drag_onmouseup);
  
  simtk_event_connect (map->hbits, SIMTK_EVENT_MOUSEMOVE, generic_drag_onmousemove);
  simtk_event_connect (map->hbits, SIMTK_EVENT_MOUSEDOWN, generic_drag_onmousedown);
  simtk_event_connect (map->hbits, SIMTK_EVENT_MOUSEUP,   generic_drag_onmouseup);
  
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

