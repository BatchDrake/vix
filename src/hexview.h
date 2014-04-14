#ifndef _HEXVIEW_H
#define _HEXVIEW_H

struct simtk_hexview_properties
{
  SDL_mutex *lock;

  void  *buffer;
  size_t buffer_size;
  int    fd;
  size_t filesize;
  
  void *opaque;
};

#endif /* _HEXVIEW_H */
