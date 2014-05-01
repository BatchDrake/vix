#include <vix.h>
#include <simtk/simtk.h>
#include "hexview.h"

struct simtk_hexview_properties *
simtk_hexview_properties_new (uint32_t vaddr, const void *base, uint32_t size)
{
  struct simtk_hexview_properties *new;

  if ((new = calloc (1, sizeof (struct simtk_hexview_properties))) == NULL)
    return NULL;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    free (new);

    return NULL;
  }

  new->map  = base;
  new->map_size = size;

  return new;
}

void
simtk_hexview_properties_lock (const struct simtk_hexview_properties *prop)
{
  SDL_mutexP (prop->lock);
}

void
simtk_hexview_properties_unlock (const struct simtk_hexview_properties *prop)
{
  SDL_mutexV (prop->lock);
}

void
simtk_hexview_properties_destroy (struct simtk_hexview_properties *prop)
{
  SDL_DestroyMutex (prop->lock);

  free (prop);
}

struct simtk_hexview_properties *
simtk_hexview_get_properties (const struct simtk_widget *widget)
{
  return (struct simtk_hexview_properties *) simtk_textview_get_opaque (widget);
}

void *
simtk_hexview_get_opaque (const struct simtk_widget *widget)
{
  struct simtk_hexview_properties *prop;
  void *opaque;

  prop = simtk_hexview_get_properties (widget);

  simtk_hexview_properties_lock (prop);

  opaque = prop->opaque;

  simtk_hexview_properties_unlock (prop);

  return opaque;
}

void
simtk_hexview_set_opaque (struct simtk_widget *widget, void *opaque)
{
  struct simtk_hexview_properties *prop;

  prop = simtk_hexview_get_properties (widget);

  simtk_hexview_properties_lock (prop);

  prop->opaque = opaque;

  simtk_hexview_properties_unlock (prop);
}

#define HEX_MINIMAL 0x80
#define ASCII_MINIMAL 0x80

void
simtk_hexview_render (struct simtk_widget *widget)
{
  struct simtk_hexview_properties *hprop;
  struct simtk_textview_properties *prop;
  
  unsigned int i;
  const void *addr;
  char buffer[11];
  int common_y = 0;
  int this_x = 0;
  uint8_t byte;
  uint32_t vaddr;
  unsigned int maxcols = 16;
  
  hprop = simtk_hexview_get_properties (widget);
  prop  = simtk_textview_get_properties (widget);
  
  simtk_hexview_properties_lock (hprop);

  vaddr = hprop->vaddr + hprop->start;
  addr  = hprop->map   + hprop->start;
  
  for (i = hprop->start; i < hprop->map_size; ++i)
  {
    if (this_x == maxcols || i == hprop->start)
    {
      if (i != hprop->start)
        if (common_y++ >= prop->rows)
          break;
      
      this_x = 0;
      
      sprintf (buffer, "%04x:%04x ", (uint32_t) vaddr >> 16, (uint32_t) vaddr & 0xffff);
      
      simtk_textview_set_text (widget, 0, common_y, OPAQUE (0xff0000), 0x80000000, buffer, 10);
    }

    sprintf (buffer, "%02x ", byte = *((uint8_t *) addr));

    simtk_textview_set_text (widget, 3 * this_x + 10 , common_y, OPAQUE (RGB ((byte + HEX_MINIMAL) * 0xff / (0xff + HEX_MINIMAL), (byte + HEX_MINIMAL) * 0xa5 / (0xff + HEX_MINIMAL), 0)), 0x80000000, buffer, 3);
    simtk_textview_set_text (widget, 10 + 3 * maxcols + this_x, common_y, OPAQUE (RGB (0, (byte + ASCII_MINIMAL) * 0xff / (0xff + HEX_MINIMAL), 0)), 0x80000000, addr++, 1);

    ++this_x;
    ++vaddr;
  }

  simtk_hexview_properties_unlock (hprop);
    
  simtk_textview_render_text (widget);
}

int
simtk_hexview_create (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
{
  simtk_hexview_render (widget);

  return HOOK_LOCK_CHAIN;
}

int
simtk_hexview_destroy (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
{
  simtk_hexview_properties_destroy (simtk_hexview_get_properties (widget));

  return HOOK_RESUME_CHAIN;
}

void
simtk_hexview_scroll_to (struct simtk_widget *widget, uint32_t offset)
{
  struct simtk_hexview_properties *prop;

  prop = simtk_hexview_get_properties (widget);

  simtk_hexview_properties_lock (prop);

  if (offset + 0x380 > prop->map_size)
  {
    if (prop->map_size < 0x380)
      prop->start = 0;
    else
      prop->start = prop->map_size - 0x380;
  }
  else
    prop->start = offset;
  
  simtk_hexview_properties_unlock (prop);

  simtk_hexview_render (widget);
}

struct simtk_widget *
simtk_hexview_new (struct simtk_container *cont, int x, int y, int rows, int cols, uint32_t vaddr, const void *base, uint32_t size)
{
  struct simtk_widget *new;
  struct simtk_hexview_properties *prop;

  if ((new = simtk_textview_new (cont, x, y, rows, cols)) == NULL)
    return NULL;

  if ((prop = simtk_hexview_properties_new (vaddr, base, size)) == NULL)
  {
    simtk_widget_destroy (new);

    return NULL;
  }

  simtk_widget_inheritance_add (new, "HexView");

  simtk_textview_set_opaque (new, prop);

  simtk_event_connect (new, SIMTK_EVENT_DESTROY, simtk_hexview_destroy);

  simtk_event_connect (new, SIMTK_EVENT_CREATE, simtk_hexview_create);

  simtk_hexview_render (new);
  
  return new;
}

