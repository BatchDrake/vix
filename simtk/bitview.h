#ifndef _SIMTK_BITVIEW_H
#define _SIMTK_BITVIEW_H

#include <stdint.h>
#include <rbtree.h>

#define SIMTK_BITVIEW_DEFAULT_COLOR_MSB  OPAQUE (RGB (0, 0xff, 0))
#define SIMTK_BITVIEW_DEFAULT_COLOR_LSB  OPAQUE (RGB (0, 0x7f, 0))

#define SIMTK_BITVIEW_DEFAULT_SELECT_MSB OPAQUE (RGB (0xff, 0, 0))
#define SIMTK_BITVIEW_DEFAULT_SELECT_LSB OPAQUE (RGB (0x7f, 0, 0))

#define SIMTK_BITVIEW_DEFAULT_MARK_MSB OPAQUE (RGB (0xff, 0xff, 0))
#define SIMTK_BITVIEW_DEFAULT_MARK_LSB OPAQUE (RGB (0x7f, 0x7f, 0))

#define SIMTK_BITVIEW_DEFAULT_BACKGROUND DEFAULT_BACKGROUND
#define SIMTK_BITVIEW_MARK_BACKGROUND    DEFAULT_BACKGROUND

#define SIMTK_BITVIEW_DEFAULT_BYTE_ORIENTATION   SIMTK_HORIZONTAL
#define SIMTK_BITVIEW_DEFAULT_WIDGET_ORIENTATION SIMTK_VERTICAL

enum simtk_orientation
{
  SIMTK_HORIZONTAL,
  SIMTK_VERTICAL
};

struct simtk_bitview_properties
{
  SDL_mutex *lock;
  
  uint32_t map_size;
  
  uint32_t start;

  int rows, cols;
  
  uint32_t sel_start;
  uint32_t sel_size;

  uint32_t mark_start;
  uint32_t mark_size;
  
  union
  {
    const void *map;
    const uint8_t *bytes;
  };
  
  enum simtk_orientation byte_orientation;
  enum simtk_orientation widget_orientation;

  uint32_t color_msb;
  uint32_t color_lsb;

  uint32_t select_msb;
  uint32_t select_lsb;

  uint32_t background;
  
  uint32_t mark_msb;
  uint32_t mark_lsb;

  uint32_t mark_background;

  rbtree_t *regions;
  
  void *opaque;
};

struct simtk_bitview_properties *simtk_bitview_properties_new (int, int, const void *, uint32_t);
void simtk_bitview_properties_lock (const struct simtk_bitview_properties *);
void simtk_bitview_properties_unlock (const struct simtk_bitview_properties *);
void simtk_bitview_properties_destroy (struct simtk_bitview_properties *);
void simtk_bitview_clear_regions (const simtk_widget_t *);
int simtk_bitview_mark_region_noflip (const simtk_widget_t *,
			       const char *,
			       uint64_t,
			       uint64_t,
			       uint32_t,
			       uint32_t);
int simtk_bitview_mark_region (simtk_widget_t *,
			       const char *,
			       uint64_t,
			       uint64_t,
			       uint32_t,
			       uint32_t);

struct simtk_bitview_properties *simtk_bitview_get_properties (const simtk_widget_t *);
void *simtk_bitview_get_opaque (const simtk_widget_t *);
void simtk_bitview_set_opaque (simtk_widget_t *, void *);
void simtk_bitview_render_bits (simtk_widget_t *);
int simtk_bitview_create (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);
int simtk_bitview_destroy (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);
void simtk_bitview_scroll_to (simtk_widget_t *, uint32_t, int);
void simtk_bitview_scroll_to_noflip (simtk_widget_t *, uint32_t, int);
simtk_widget_t *simtk_bitview_new (struct simtk_container *, int, int, int, int, enum simtk_orientation, enum simtk_orientation, const void *, uint32_t, int);

#endif /* _SIMTK_BITVIEW_H */
