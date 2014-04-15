#ifndef _MAP_H
#define _MAP_H

#include <stdint.h>
#include <simtk/simtk.h>

struct filemap
{
  void    *base;
  char    *path;
  uint32_t size;

  uint32_t offset;
  
  struct simtk_container *container;
  
  struct simtk_widget *hexview, *hexwid;
  struct simtk_widget *vbits, *vwid;
  struct simtk_widget *hbits, *hwid;
};

struct filemap *filemap_new (struct simtk_container *, const char *);
void filemap_destroy (struct filemap *);

#endif /* _MAP_H */
