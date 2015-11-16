#include <draw.h>
#include <region.h>

#include <simtk/widget.h>

#include "hilbert.h"
#include <simtk/primitives.h>

struct simtk_hilbert_properties *
simtk_hilbert_properties_new (int power, const void *base, uint32_t size)
{
  struct simtk_hilbert_properties *new;

  if (power < HILBERT_MIN_POWER || power > HILBERT_MAX_POWER)
  {
    ERROR ("Power 2^%d out of bounds!\n", power);
    return NULL;
  }
  
  if ((new = calloc (1, sizeof (struct simtk_hilbert_properties))) == NULL)
    return NULL;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    free (new);

    return NULL;
  }

  new->power = power;
  new->resolution = 1 << power;
  new->map  = base;
  new->map_size = size;

  return new;
}

void
simtk_hilbert_properties_lock (const struct simtk_hilbert_properties *prop)
{
  SDL_mutexP (prop->lock);
}

void
simtk_hilbert_properties_unlock (const struct simtk_hilbert_properties *prop)
{
  SDL_mutexV (prop->lock);
}

void
simtk_hilbert_properties_destroy (struct simtk_hilbert_properties *prop)
{
  SDL_DestroyMutex (prop->lock);

  free (prop);
}

struct simtk_hilbert_properties *
simtk_hilbert_get_properties (const simtk_widget_t *widget)
{
  return (struct simtk_hilbert_properties *) simtk_widget_get_opaque (widget);
}

void *
simtk_hilbert_get_opaque (const simtk_widget_t *widget)
{
  struct simtk_hilbert_properties *prop;
  void *opaque;

  prop = simtk_hilbert_get_properties (widget);

  simtk_hilbert_properties_lock (prop);

  opaque = prop->opaque;

  simtk_hilbert_properties_unlock (prop);

  return opaque;
}

void
simtk_hilbert_set_opaque (simtk_widget_t *widget, void *opaque)
{
  struct simtk_hilbert_properties *prop;

  prop = simtk_hilbert_get_properties (widget);

  simtk_hilbert_properties_lock (prop);

  prop->opaque = opaque;

  simtk_hilbert_properties_unlock (prop);
}

void
simtk_hilbert_clear_regions (const simtk_widget_t *widget)
{
  struct simtk_hilbert_properties *prop;

  prop = simtk_hilbert_get_properties (widget);

  simtk_hilbert_properties_lock (prop);

  simtk_hilbert_properties_unlock (prop);
}

static void rot (int , int *, int *, int, int);

static int
xy2d (int n, int x, int y)
{
  int rx, ry, s, d = 0;
  
  for (s = n >> 1; s > 0; s >>= 1)
  {
    rx = (x & s) > 0;
    ry = (y & s) > 0;
    d += s * s * ((3 * rx) ^ ry);
    rot (s, &x, &y, rx, ry);
  }
  return d;
}
 
static void
d2xy (int n, int d, int *x, int *y)
{
  int rx, ry, s, t = d;
  
  *x = *y = 0;

  for (s = 1; s < n; s *= 2)
  {
    rx = 1 & (t >> 1);
    ry = 1 & (t ^ rx);
    
    rot (s, x, y, rx, ry);
    
    *x += s * rx;
    *y += s * ry;
    t >>= 2;
  }
}

static void
rot (int n, int *x, int *y, int rx, int ry)
{
  int t;
  
  if (ry == 0)
  {
    if (rx == 1)
    {
      *x = n-1 - *x;
      *y = n-1 - *y;
    }
    
    t  = *x;
    *x = *y;
    *y = t;
  }
}

void
simtk_hilbert_render_bits_noflip (simtk_widget_t *widget)
{
  struct simtk_hilbert_properties *prop;
  unsigned int i, offset;
  unsigned int max;
  int x, y;
  uint8_t *bytes;
  
  prop = simtk_hilbert_get_properties (widget);

  simtk_hilbert_properties_lock (prop);

  max = 4 * prop->resolution * prop->resolution;

  bytes = (uint8_t *) prop->map + ((prop->start >> 2) << 2);
  
  for (i = 0; i < max; i += 4)
  {
    offset = ((i + prop->start) >> 2) << 2;

    d2xy (prop->resolution, i >> 2, &x, &y);

    if (offset + 3 >= prop->map_size)
      simtk_widget_pset (widget, x, y, OPAQUE (0));
    else
      simtk_widget_pset (widget, x, y, OPAQUE (RGB (bytes[i], bytes[i + 1], bytes[i + 2])));
  }
  
  simtk_hilbert_properties_unlock (prop);
}

void
simtk_hilbert_render_bits (simtk_widget_t *widget)
{
  simtk_hilbert_render_bits_noflip (widget);

  simtk_widget_switch_buffers (widget);
}

int
simtk_hilbert_create (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_hilbert_render_bits (widget);

  return HOOK_RESUME_CHAIN;
}

int
simtk_hilbert_destroy (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_hilbert_properties_destroy (simtk_hilbert_get_properties (widget));

  return HOOK_RESUME_CHAIN;
}

void
simtk_hilbert_scroll_to_noflip (simtk_widget_t *widget, uint32_t offset)
{
  struct simtk_hilbert_properties *prop;
  
  prop = simtk_hilbert_get_properties (widget);

  simtk_hilbert_properties_lock (prop);

  prop->start = offset;
  
  simtk_hilbert_properties_unlock (prop);

  simtk_hilbert_render_bits_noflip (widget);
}

void
simtk_hilbert_scroll_to (simtk_widget_t *widget, uint32_t offset)
{
  simtk_hilbert_scroll_to_noflip (widget, offset);

  simtk_widget_switch_buffers (widget);
}

simtk_widget_t *
simtk_hilbert_new (struct simtk_container *cont, int x, int y, int power, const void *map, uint32_t map_size)
{
  struct simtk_hilbert_properties *prop;
  simtk_widget_t *new;
  int resolution;

  if (power < HILBERT_MIN_POWER ||
      power > HILBERT_MAX_POWER)
  {
    ERROR ("Hilbert view power (2^%d) out of bounds\n", power);
    return NULL;
  }
  
  resolution = 1 << power;

  if ((new = simtk_widget_new (cont, x, y, resolution, resolution)) == NULL)
    return NULL;

  if ((prop = simtk_hilbert_properties_new (power, map, map_size)) == NULL)
  {
    simtk_widget_destroy (new);

    return NULL;
  }

  simtk_widget_inheritance_add (new, "Hilbert");

  simtk_widget_set_opaque (new, prop);

  simtk_event_connect (new, SIMTK_EVENT_DESTROY, simtk_hilbert_destroy);

  simtk_event_connect (new, SIMTK_EVENT_CREATE, simtk_hilbert_create);

  simtk_hilbert_render_bits (new);
  
  return new;
}
