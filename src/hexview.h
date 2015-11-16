#ifndef _HEXVIEW_H
#define _HEXVIEW_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <rbtree.h>

struct simtk_hexview_properties
{
  SDL_mutex *lock;

  uint32_t vaddr;
  uint32_t map_size;
  uint32_t start;
  
  union
  {
    const void *map;
    const uint8_t *bytes;
  };

  rbtree_t *regions;
  void *opaque;
};

struct simtk_hexview_properties *simtk_hexview_properties_new (uint32_t, const void *, uint32_t);
void simtk_hexview_properties_lock (const struct simtk_hexview_properties *);
void simtk_hexview_properties_unlock (const struct simtk_hexview_properties *);
void simtk_heview_properties_destroy (struct simtk_hexview_properties *);

struct simtk_hexview_properties *simtk_hexview_get_properties (const simtk_widget_t *);

void simtk_hexview_clear_regions (const simtk_widget_t *);
int simtk_hexview_mark_region_noflip (const simtk_widget_t *,
			       const char *,
			       uint64_t,
			       uint64_t,
			       uint32_t,
			       uint32_t);
int simtk_hexview_mark_region (simtk_widget_t *,
			       const char *,
			       uint64_t,
			       uint64_t,
			       uint32_t,
			       uint32_t);

void *simtk_hexview_get_opaque (const simtk_widget_t *);
void  simtk_hexview_set_opaque (simtk_widget_t *, void *);
void  simtk_hexview_render (simtk_widget_t *);
void  simtk_hexview_render_noflip (simtk_widget_t *);
void  simtk_hexview_scroll_to_noflip (simtk_widget_t *, uint32_t);
int   simtk_hexview_create (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);
int   simtk_hexview_destroy (enum simtk_event_type, simtk_widget_t *, struct simtk_event *);
void  simtk_hexview_scroll_to (simtk_widget_t *, uint32_t);
simtk_widget_t *simtk_hexview_new (struct simtk_container *, int, int, int, int, uint32_t, const void *, uint32_t);

#endif /* _HEXVIEW_H */
