#include <draw.h>
#include <region.h>

#include "widget.h"
#include "bitview.h"
#include "primitives.h"

struct simtk_bitview_properties *
simtk_bitview_properties_new (int cols, int rows, const void *base, uint32_t size)
{
  struct simtk_bitview_properties *new;

  if ((new = calloc (1, sizeof (struct simtk_bitview_properties))) == NULL)
    return NULL;

  if ((new->regions = file_region_tree_new ()) == NULL)
  {
    free (new);

    return NULL;
  }
  
  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    rbtree_destroy (new->regions);
    free (new);

    return NULL;
  }

  new->rows = rows;
  new->cols = cols;
  new->map  = base;
  new->map_size = size;
  
  new->byte_orientation = SIMTK_BITVIEW_DEFAULT_BYTE_ORIENTATION;
  new->widget_orientation = SIMTK_BITVIEW_DEFAULT_WIDGET_ORIENTATION;

  new->color_msb = SIMTK_BITVIEW_DEFAULT_COLOR_MSB;
  new->color_lsb = SIMTK_BITVIEW_DEFAULT_COLOR_LSB;

  new->select_msb = SIMTK_BITVIEW_DEFAULT_SELECT_MSB;
  new->select_lsb = SIMTK_BITVIEW_DEFAULT_SELECT_LSB;

  new->background = SIMTK_BITVIEW_DEFAULT_BACKGROUND;

  return new;
}

void
simtk_bitview_properties_lock (const struct simtk_bitview_properties *prop)
{
  SDL_mutexP (prop->lock);
}

void
simtk_bitview_properties_unlock (const struct simtk_bitview_properties *prop)
{
  SDL_mutexV (prop->lock);
}

void
simtk_bitview_properties_destroy (struct simtk_bitview_properties *prop)
{
  SDL_DestroyMutex (prop->lock);

  free (prop);
}

struct simtk_bitview_properties *
simtk_bitview_get_properties (const simtk_widget_t *widget)
{
  return (struct simtk_bitview_properties *) simtk_widget_get_opaque (widget);
}

void *
simtk_bitview_get_opaque (const simtk_widget_t *widget)
{
  struct simtk_bitview_properties *prop;
  void *opaque;

  prop = simtk_bitview_get_properties (widget);

  simtk_bitview_properties_lock (prop);

  opaque = prop->opaque;

  simtk_bitview_properties_unlock (prop);

  return opaque;
}

void
simtk_bitview_set_opaque (simtk_widget_t *widget, void *opaque)
{
  struct simtk_bitview_properties *prop;

  prop = simtk_bitview_get_properties (widget);

  simtk_bitview_properties_lock (prop);

  prop->opaque = opaque;

  simtk_bitview_properties_unlock (prop);
}

void
simtk_bitview_clear_regions (const simtk_widget_t *widget)
{
  struct simtk_bitview_properties *prop;

  prop = simtk_bitview_get_properties (widget);

  simtk_bitview_properties_lock (prop);

  rbtree_clear (prop->regions);

  simtk_bitview_properties_unlock (prop);
}

int
simtk_bitview_mark_region_noflip (const simtk_widget_t *widget,
			   const char *name,
			   uint64_t start,
			   uint64_t length,
			   uint32_t fgcolor,
			   uint32_t bgcolor)
{
  struct simtk_bitview_properties *prop;

  int result;
  
  prop = simtk_bitview_get_properties (widget);

  simtk_bitview_properties_lock (prop);

  result = file_region_register (prop->regions, name, start, length, fgcolor, bgcolor);

  simtk_bitview_properties_unlock (prop);

  return result;
}

int
simtk_bitview_mark_region (simtk_widget_t *widget,
			   const char *name,
			   uint64_t start,
			   uint64_t length,
			   uint32_t fgcolor,
			   uint32_t bgcolor)

{
  int result;
  
  result = simtk_bitview_mark_region_noflip (widget, name, start, length, fgcolor, bgcolor);

  simtk_widget_switch_buffers (widget);

  return result;
}

void
simtk_bitview_render_bits_noflip (simtk_widget_t *widget)
{
  struct simtk_bitview_properties *prop;
  struct rbtree_node *node;
  struct file_region *region;
  
  int i, j, p;
  int paint;
  int real_x, real_y;
  int *x = &real_x, *y = &real_y;
  uint32_t offset;
  double t;
  
  uint32_t normal_colors[8];
  uint32_t sel_colors[8];

  uint32_t fgcolor, bgcolor;
  
  uint8_t byte;
  
  uint8_t a, r, g, b;
  uint8_t na0, nr0, ng0, nb0, na7, nr7, ng7, nb7;
  uint8_t sa0, sr0, sg0, sb0, sa7, sr7, sg7, sb7;

  
  prop = simtk_bitview_get_properties (widget);

  simtk_bitview_properties_lock (prop);
  
  na0 = G_ALPHA (prop->color_lsb);
  nr0 = G_RED (prop->color_lsb);
  ng0 = G_GREEN (prop->color_lsb);
  nb0 = G_BLUE (prop->color_lsb);

  na7 = G_ALPHA (prop->color_msb);
  nr7 = G_RED (prop->color_msb);
  ng7 = G_GREEN (prop->color_msb);
  nb7 = G_BLUE (prop->color_msb);

  sa0 = G_ALPHA (prop->select_lsb);
  sr0 = G_RED (prop->select_lsb);
  sg0 = G_GREEN (prop->select_lsb);
  sb0 = G_BLUE (prop->select_lsb);

  sa7 = G_ALPHA (prop->select_msb);
  sr7 = G_RED (prop->select_msb);
  sg7 = G_GREEN (prop->select_msb);
  sb7 = G_BLUE (prop->select_msb);

  if ((node = file_region_find (prop->regions, prop->start)) != NULL)
    region = (struct file_region *) rbtree_node_data (node);
  else
    region = NULL;
  
  for (i = 0; i < 8; ++i)
  {
    t = (double) i / 7.0;

    a = na0 * (1 - t) + na7 * t;
    r = nr0 * (1 - t) + nr7 * t;
    g = ng0 * (1 - t) + ng7 * t;
    b = nb0 * (1 - t) + nb7 * t;

    normal_colors[i] = ARGB (a, r, g, b);

    a = sa0 * (1 - t) + sa7 * t;
    r = sr0 * (1 - t) + sr7 * t;
    g = sg0 * (1 - t) + sg7 * t;
    b = sb0 * (1 - t) + sb7 * t;

    sel_colors[i] = ARGB (a, r, g, b);
  }

  if (prop->widget_orientation == SIMTK_VERTICAL)
  {
    x = &real_y;
    y = &real_x;
  }

  for (j = 0; j < prop->rows; ++j)
  {
    *y = j;

    if (prop->byte_orientation == SIMTK_HORIZONTAL)
      offset = prop->start + j * (prop->cols >> 3);
    else
      offset = prop->start + j;
    
    if (offset >= prop->map_size)
      break;

    for (i = 0; i < prop->cols; ++i)
    {
      if (prop->byte_orientation == SIMTK_HORIZONTAL)
      {
	if (offset + (p = i >> 3) >= prop->map_size)
	  break;
      }
      else
      {
	if (offset + (p = (i >> 3) * prop->rows) >= prop->map_size)
	  break;
      }
	  
      byte = prop->bytes[offset + p];
      *x = i;

      if (region != NULL)
	if (offset + p >= region->start + region->length)
	{
	  if ((node = rbtree_node_next (node)) != NULL)
	    region = (struct file_region *) rbtree_node_data (node);
	  else
	    region = NULL;
	}

      if (region != NULL)
      {
	bgcolor = region->bgcolor;

	paint = offset + p >= region->start && offset + p < region->start + region->length;
      }
      else
	paint = 0;
      
      if (byte & (1 << (i & 7)))
      {
	if ((prop->sel_start <= offset + p) &&
	    (offset + p < prop->sel_start + prop->sel_size))
	  simtk_widget_pset (widget, real_x, real_y, sel_colors[i & 7]);
	else
	  simtk_widget_pset (widget, real_x, real_y, paint ? region->colors[i & 7] : normal_colors[i & 7]);
      }
      else
	simtk_widget_pset (widget, real_x, real_y, paint ? bgcolor : prop->background);
    }
  }

  simtk_bitview_properties_unlock (prop);
}

void
simtk_bitview_render_bits (simtk_widget_t *widget)
{
  simtk_bitview_render_bits_noflip (widget);

  simtk_widget_switch_buffers (widget);
}

int
simtk_bitview_create (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_bitview_render_bits (widget);

  return HOOK_RESUME_CHAIN;
}

int
simtk_bitview_destroy (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  simtk_bitview_properties_destroy (simtk_bitview_get_properties (widget));

  return HOOK_RESUME_CHAIN;
}

void
simtk_bitview_scroll_to_noflip (simtk_widget_t *widget, uint32_t offset, int size)
{
  struct simtk_bitview_properties *prop;
  unsigned int bits, bytes;
  
  prop = simtk_bitview_get_properties (widget);

  simtk_bitview_properties_lock (prop);

  bits  = prop->rows * prop->cols;
  bytes = bits >> 3;
  
  if ((int) offset < 0)
    offset = 0;
  else if (offset >= prop->map_size - prop->sel_size)
    offset = prop->map_size - prop->sel_size;

  if (offset < prop->start)
    prop->start = offset;
  else if (offset > prop->start - prop->sel_size + bytes)
    prop->start = offset - (bytes - prop->sel_size);

  prop->sel_start = offset;
  
  simtk_bitview_properties_unlock (prop);

  simtk_bitview_render_bits_noflip (widget);
}

void
simtk_bitview_scroll_to (simtk_widget_t *widget, uint32_t offset, int size)
{
  simtk_bitview_scroll_to_noflip (widget, offset, size);

  simtk_widget_switch_buffers (widget);
}

simtk_widget_t *
simtk_bitview_new (struct simtk_container *cont, int x, int y, int rows, int cols, enum simtk_orientation widget_orientation, enum simtk_orientation byte_orientation, const void *map, uint32_t map_size, int sel_size)
{
  struct simtk_bitview_properties *prop;
  simtk_widget_t *new;

  int width;
  int height;
  
  if (widget_orientation == SIMTK_HORIZONTAL)
  {
    width  = cols;
    height = rows;
  }
  else
  {
    width  = rows;
    height = cols;
  }
  
  if ((new = simtk_widget_new (cont, x, y, width, height)) == NULL)
    return NULL;

  if ((prop = simtk_bitview_properties_new (width, height, map, map_size)) == NULL)
  {
    simtk_widget_destroy (new);

    return NULL;
  }

  simtk_widget_inheritance_add (new, "BitView");

  prop->byte_orientation = byte_orientation;

  prop->widget_orientation = widget_orientation;

  prop->sel_size = sel_size;
  
  simtk_widget_set_opaque (new, prop);

  simtk_event_connect (new, SIMTK_EVENT_DESTROY, simtk_bitview_destroy);

  simtk_event_connect (new, SIMTK_EVENT_CREATE, simtk_bitview_create);

  simtk_bitview_render_bits (new);
  
  return new;
}
