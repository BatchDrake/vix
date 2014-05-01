/*
  
  Copyright (C) 2014 Gonzalo José Carracedo Carballal
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

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

void
setup_command_entry (struct simtk_container *container)
{
  struct simtk_widget *widget;

  widget = simtk_entry_new (container, 1, container->height - 9, container->width / 8 - 1);
}

int
main (int argc, char *argv[], char *envp[])
{
  display_t *disp;
  struct filemap *map;
  PTR_LIST_LOCAL (struct filemap, map);
  
  int i;
  
  if (argc < 2)
  {
    fprintf (stderr, "Usage:\n\t%s <files>\n", argv[0]);

    exit (EXIT_FAILURE);
  }
  if ((disp = display_new (1024, 768)) == NULL)
    exit (EXIT_FAILURE);

#ifndef SDL2_ENABLED
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL / 4);
#endif
  
  if (simtk_init_from_display (disp) == -1)
  {
    fprintf (stderr, "%s: no memory to alloc simtk root container\n", argv[0]);
    exit (EXIT_FAILURE);
  }
  
  simtk_container_set_background (simtk_get_root_container (), "/usr/lib/vix/background.bmp");

  setup_command_entry (simtk_get_root_container ());

  /* TODO: global list of file maps */
  
  for (i = 1; i < argc; ++i)
  {
    if ((map = filemap_new (simtk_get_root_container (), argv[i])) == NULL)
    {
      fprintf (stderr, "%s: cannot open view for %s: %s\n", argv[0], argv[i], strerror (errno));

      exit (EXIT_FAILURE);
    }

    if (PTR_LIST_APPEND_CHECK (map, map) == -1)
    {
      fprintf (stderr, "%s: out of memory while registering file map\n");

      exit (EXIT_FAILURE);
    }
  }
  
  simtk_event_loop (simtk_get_root_container ());

  for (i = 0; i < map_count; ++i)
    if (map_list[i] != NULL)
      filemap_destroy (map_list[i]);

  if (map_list != NULL)
    free (map_list);
  
  return 0;
}

