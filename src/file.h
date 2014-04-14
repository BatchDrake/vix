#ifndef _FILE_H
#define _FILE_H

#include <stdio.h>
#include <util.h>

struct openfile_view
{
  void  *buffer;
  size_t buffer_size;
  size_t buffer_fill;
  off_t  offset;

  struct openfile *file;
};

struct openfile
{
  char   *path;
  FILE  *fp;
  size_t size;

  SDL_mutex *lock;
  
  PTR_LIST (struct openfile_view, view);
};

struct openfile *openfile_new (const char *);
void openfile_lock (const struct openfile *);
void openfile_unlock (const struct openfile *);
struct openfile_view *openfile_view_new (struct openfile *, size_t);
void openfile_view_destroy (struct openfile_view *);
void openfile_destroy (struct openfile *);
void openfile_view_read (struct openfile_view *, off_t);

#endif /* _FILE_H */
