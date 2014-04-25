/*
 * main.c: entry point for vix
 * Creation date: Tue Sep 10 13:50:07 2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <vix.h>
#include <simtk/simtk.h>
#include "hexview.h"
#include "map.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


#define STRING_SIZE 4

int
main (int argc, char *argv[], char *envp[])
{
  display_t *disp;
  struct filemap *map;
  int i;
  
  if (argc < 2)
  {
    fprintf (stderr, "Usage:\n\t%s <files>\n", argv[0]);

    exit (EXIT_FAILURE);
  }
  if ((disp = display_new (1024, 768)) == NULL)
    exit (EXIT_FAILURE);

  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL / 4);
  
  if (simtk_init_from_display (disp) == -1)
  {
    fprintf (stderr, "%s: no memory to alloc simtk root container\n", argv[0]);
    exit (EXIT_FAILURE);
  }
  
  simtk_container_set_background (simtk_get_root_container (), "/usr/lib/vix/background.bmp");

  for (i = 1; i < argc; ++i)
    if ((map = filemap_new (simtk_get_root_container (), argv[i])) == NULL)
    {
      fprintf (stderr, "%s: cannot open view for %s: %s\n", argv[0], argv[i], strerror (errno));

      exit (EXIT_FAILURE);
    }

  
  simtk_event_loop (simtk_get_root_container ());
  
  return 0;
}

