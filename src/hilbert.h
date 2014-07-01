#ifndef _SIMTK_HILBERT_H
#define _SIMTK_HILBERT_H

#include <stdint.h>

#define HILBERT_MIN_POWER 3
#define HILBERT_MAX_POWER 10

struct simtk_hilbert_properties
{
  SDL_mutex *lock;
  
  uint32_t map_size;
  
  uint32_t start;

  int power;
  int resolution;
  
  union
  {
    const void *map;
    const uint8_t *bytes;
  };
  
  void *opaque;
};

struct simtk_hilbert_properties *simtk_hilbert_properties_new (int, const void *, uint32_t);
void simtk_hilbert_properties_lock (const struct simtk_hilbert_properties *);
void simtk_hilbert_properties_unlock (const struct simtk_hilbert_properties *);
void simtk_hilbert_properties_destroy (struct simtk_hilbert_properties *);
struct simtk_hilbert_properties *simtk_hilbert_get_properties (const struct simtk_widget *);
void *simtk_hilbert_get_opaque (const struct simtk_widget *);
void simtk_hilbert_set_opaque (struct simtk_widget *, void *);
void simtk_hilbert_render_bits (struct simtk_widget *);
int simtk_hilbert_create (enum simtk_event_type, struct simtk_widget *, struct simtk_event *);
int simtk_hilbert_destroy (enum simtk_event_type, struct simtk_widget *, struct simtk_event *);
void simtk_hilbert_scroll_to (struct simtk_widget *, uint32_t);
void simtk_hilbert_scroll_to_noflip (struct simtk_widget *, uint32_t);
struct simtk_widget *simtk_hilbert_new (struct simtk_container *cont, int, int, int, const void *, uint32_t);

#endif /* _SIMTK_HILBERT_H */
