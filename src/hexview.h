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

struct simtk_hexview_properties *simtk_hexview_get_properties (const struct simtk_widget *);

void simtk_hexview_clear_regions (const struct simtk_widget *);
int simtk_hexview_mark_region_noflip (const struct simtk_widget *,
			       const char *,
			       uint64_t,
			       uint64_t,
			       uint32_t,
			       uint32_t);
int simtk_hexview_mark_region (struct simtk_widget *,
			       const char *,
			       uint64_t,
			       uint64_t,
			       uint32_t,
			       uint32_t);

void *simtk_hexview_get_opaque (const struct simtk_widget *);
void  simtk_hexview_set_opaque (struct simtk_widget *, void *);
void  simtk_hexview_render (struct simtk_widget *);
void  simtk_hexview_render_noflip (struct simtk_widget *);
void  simtk_hexview_scroll_to_noflip (struct simtk_widget *, uint32_t);
int   simtk_hexview_create (enum simtk_event_type, struct simtk_widget *, struct simtk_event *);
int   simtk_hexview_destroy (enum simtk_event_type, struct simtk_widget *, struct simtk_event *);
void  simtk_hexview_scroll_to (struct simtk_widget *, uint32_t);
struct simtk_widget *simtk_hexview_new (struct simtk_container *, int, int, int, int, uint32_t, const void *, uint32_t);

#endif /* _HEXVIEW_H */
