#include "file.h"

PTR_LIST (struct openfile, file);

struct openfile *
openfile_new (const char *path)
{
  struct openfile *new;
  FILE *fp;

  if ((fp = fopen (path, "rb")) == NULL)
    return NULL;
  
  if ((new = calloc (1, sizeof (struct openfile))) == NULL)
  {
    fclose (fp);
    return NULL;
  }

  if ((new->path = strdup (path)) == NULL)
  {
    fclose (fp);
    free (new);

    return NULL;
  }

  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    free (new->path);
    
    fclose (fp);

    free (new);

    return NULL;
  }
  
  new->fp   = fp;
  
  fseek (fp, 0, SEEK_END);

  new->size = ftell (fp);

  rewind (fp);

  if (PTR_LIST_APPEND (file, new) == -1)
  {
    SDL_DestroyMutex (new->lock);
    
    free (new->path);

    fclose (fp);

    free (new);

    return NULL;
  }
  
  return new;
}

void
openfile_lock (const struct openfile *file)
{
  SDL_MutexP (file->lock);
}

void
openfile_unlock (const struct openfile *file)
{
  SDL_MutexV (file->lock);
}

struct openfile_view *
openfile_view_new (struct openfile *file, size_t size)
{
  struct openfile_view *new;

  if ((new = malloc (sizeof (struct openfile_view))) == NULL)
    return NULL;

  if ((new->buffer = calloc (1, size)) == NULL)
  {
    free (new);

    return NULL;
  }

  new->openfile = file;

  if (PTR_LIST_APPEND (file->view, new) == -1)
  {
    free (new->buffer);
    free (new);

    return NULL;
  }

  return new;
}

void
openfile_view_destroy (struct openfile_view *view)
{
  free (view->buffer);
  free (view);
}

void
openfile_destroy (struct openfile *file)
{
  int i;

  openfile_lock (file);
  
  for (i = 0; i < file->view_count; ++i)
    if (file->view_list[i] != NULL)
      openfile_view_destroy (file->view_list[i]);

  if (file->view_list != NULL)
    free (file->view_list);

  free (file->path);

  fclose (fp);

  openfile_unlock (file);

  SDL_DestroyMutex (file->lock);

  free (file);
}

void
openfile_view_read (struct openfile_view *view, off_t offset)
{
  openfile_lock (view->lock);

  fseek (view->fp, offset, SEEK_SET);
  
  view->buffer_fill = fread (view->buffer, 1, view->buffer_size, view->fp);

  view->offset = offset;
  
  openfile_unlock (view->unlock);
}

