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
#include "console.h"
#include "hexview.h"
#include "map.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define STRING_SIZE 4

PTR_LIST_EXTERN (struct filemap, map);

simtk_widget_t *entry;
simtk_widget_t *console;
simtk_widget_t *consolewindow;

void simtk_widget_make_draggable (simtk_widget_t *);

char *motd =
    "\n"
    "     Welcome to Vix - v0.1.2  \n"
    "     (c) 2014 Gonzalo J. Carracedo (BatchDrake)\n"
    "\n"
    "WHAT'S NEW:\n"
    "More bugfixes. I removed the scripting support because it seems that\n"
    "nobody will use it at all. I also made some changes in window dragging,\n"
    "now dragging is restricted to title bar (and the whole window using\n"
    "Ctrl+Drag). Window dragging using the Ctrl key doesn't work in this,\n"
    "release, I need to fix a few more things regarding event cascading first.\n\n"

    "Type `help' and press ENTER to get a list of available commands\n\n";

int
vix_console_onfocus (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  /* Reject the focus */
  
  simtk_widget_focus (entry);

  simtk_widget_bring_front (entry);
  
  return HOOK_RESUME_CHAIN;
}

void
vix_help (void)
{
  const char *helptext =
    "Available commands:\n"
    "  help                 This help\n"
    "  files                List opened files\n"
    "  go fileno off        Jump to offset\n"
    "  hilbert fileno power Open 24bpp Hilbert curve display\n"
    "  search fileno string Search all occurrences of string <string>\n"
    "  open path            Open file\n\n";
  
  scputs (console, helptext);
}

void
vix_list_files ()
{
  int i;
  int n = 0;;
  
  scputs (console, " Num  Size       Offset           Path (base)\n");
  scputs (console, "-----------------------------------------------\n");
  
  for (i = 0; i < map_count; ++i)
    if (map_list[i] != NULL)
    {
      scprintf (console, " %3d  %-9d  %-16x %s (%p)\n", i, map_list[i]->size, map_list[i]->offset, map_list[i]->path, map_list[i]->base);
      ++n;
    }

  scprintf (console, "Total %d\n\n", n); 
}

void
vix_go (arg_list_t *al)
{
  uint64_t offset;
  int id;
  
  if (al->al_argc != 3)
  {
    scprintf (console, "%s: wrong number of arguments\n", al->al_argv[0]);
    scprintf (console, "Usage:\n");
    scprintf (console, "   %s fileno offset\n\n", al->al_argv[0]);
    scprintf (console, "`fileno' is the file number as found executing the `filelist' command\n");
    scprintf (console, "`offset' is the displacement within the file where we want to jump, it can be expressed in decimal (i.e. 756423), octal (0312) or hexadecimal (0x43abf1a)\n\n");

    return;
  }

  if (!sscanf (al->al_argv[2], "%lli", &offset))
  {
    scprintf (console, "%s: cannot understand `%s' as an offset\n", al->al_argv[0], al->al_argv[2]);
    return;
  }

  if (!sscanf (al->al_argv[1], "%i", &id))
  {
    scprintf (console, "%s: malformed file number `%s'\n", al->al_argv[0], al->al_argv[1]);
    return;
  }

  if (id < 0 || id >= map_count)
  {
    scprintf (console, "%s: file #%d doesn't exist\n", al->al_argv[0], id);
    return;
  }

  if (map_list[id] == NULL)
  {
    scprintf (console, "%s: file #%d is closed\n", al->al_argv[0], id);
    return;
  }
  
  scprintf (console, "Jumping to 0x%llx...\n", offset);
  
  filemap_jump_to_offset (map_list[id], offset);
}

void
vix_hilbert (arg_list_t *al)
{
  int power;
  int id;
  
  if (al->al_argc != 3)
  {
    scprintf (console, "%s: wrong number of arguments\n", al->al_argv[0]);
    scprintf (console, "Usage:\n");
    scprintf (console, "   %s fileno power\n\n", al->al_argv[0]);
    scprintf (console, "`fileno' is the file number as found executing the `filelist' command\n");
    scprintf (console, "`power' specifies the size of the Hilbert view as the power of two. Min value for power is %d (%dx%d display) and max is %d (%dx%d) \n\n", HILBERT_MIN_POWER, 1 << HILBERT_MIN_POWER, 1 << HILBERT_MIN_POWER, HILBERT_MAX_POWER, 1 << HILBERT_MAX_POWER, 1 << HILBERT_MAX_POWER);

    return;
  }

  if (!sscanf (al->al_argv[2], "%i", &power))
  {
    scprintf (console, "%s: cannot understand `%s' as a power\n", al->al_argv[0], al->al_argv[2]);
    return;
  }

  if (power < HILBERT_MIN_POWER || power > HILBERT_MAX_POWER)
  {
    scprintf (console, "%s: power must be between %d and %d\n", HILBERT_MIN_POWER, HILBERT_MAX_POWER);
    return;
  }
  
  if (!sscanf (al->al_argv[1], "%i", &id))
  {
    scprintf (console, "%s: malformed file number `%s'\n", al->al_argv[0], al->al_argv[1]);
    return;
  }

  if (id < 0 || id >= map_count)
  {
    scprintf (console, "%s: file #%d doesn't exist\n", al->al_argv[0], id);
    return;
  }

  if (map_list[id] == NULL)
  {
    scprintf (console, "%s: file #%d is closed\n", al->al_argv[0], id);
    return;
  }
  
  filemap_open_hilbert (map_list[id], power);
}

void
vix_search (arg_list_t *al)
{
  int id;
  
  if (al->al_argc != 3)
  {
    scprintf (console, "%s: wrong number of arguments\n", al->al_argv[0]);
    scprintf (console, "Usage:\n");
    scprintf (console, "   %s fileno string\n\n", al->al_argv[0]);
    scprintf (console, "`fileno' is the file number as found executing the `filelist' command\n");
    scprintf (console, "`string' is the string to search\n\n");

    return;
  }

  if (!sscanf (al->al_argv[1], "%i", &id))
  {
    scprintf (console, "%s: malformed file number `%s'\n", al->al_argv[0], al->al_argv[1]);
    return;
  }

  if (id < 0 || id >= map_count)
  {
    scprintf (console, "%s: file #%d doesn't exist\n", al->al_argv[0], id);
    return;
  }

  if (map_list[id] == NULL)
  {
    scprintf (console, "%s: file #%d is closed\n", al->al_argv[0], id);
    return;
  }
  
  
  filemap_search (map_list[id], al->al_argv[2], strlen (al->al_argv[2]));
}


void
vix_open (arg_list_t *al)
{
  struct filemap *map;
  
  if (al->al_argc != 2)
  {
    scprintf (console, "%s: wrong number of arguments\n", al->al_argv[0]);
    scprintf (console, "Usage\n");
    scprintf (console, "  %s <path>\n\n", al->al_argv[0]);
  }

  if (vix_open_file (al->al_argv[1]) == -1)
    scprintf (console, "%s: cannot open %s: %s\n",
	      al->al_argv[0],
	      al->al_argv[1],
	      strerror (errno));
}

int
vix_console_onsubmit (enum simtk_event_type type, simtk_widget_t *widget, struct simtk_event *event)
{
  arg_list_t *al;
  char *line;

  if ((line = simtk_entry_get_text (widget)) != NULL)
    if ((al = split_line (line)) != NULL)
    {
      if (al->al_argc > 0)
      {
        if (strcmp (al->al_argv[0], "help") == 0)
          vix_help ();
        else if (strcmp (al->al_argv[0], "files") == 0)
          vix_list_files ();
        else if (strcmp (al->al_argv[0], "go") == 0)
          vix_go (al);
        else if (strcmp (al->al_argv[0], "open") == 0)
          vix_open (al);
        else if (strcmp (al->al_argv[0], "hilbert") == 0)
          vix_hilbert (al);
	else if (strcmp (al->al_argv[0], "search") == 0)
	  vix_search (al);
        else
          scprintf (console, "Unknown command `%s'\n", al->al_argv[0]);
      }
      
      free_al (al);
    }

  simtk_entry_clear (widget);

  return HOOK_RESUME_CHAIN;
}

int
vix_console_open (struct simtk_container *container)
{
  if ((consolewindow = simtk_window_new (container, 512 - 320, 384 - 212, 640, 425, "Vix console")) == NULL)
    return -1;

  container = simtk_window_get_body_container (consolewindow);
  
  if ((console = simtk_console_new (container, 0, 0, 50, 80, 5000, 80)) == NULL)
    return -1;

  if ((entry = simtk_entry_new (container, 1, container->height - 9, container->width / 8)) == NULL)
    return -1;

  simtk_event_connect (console, SIMTK_EVENT_FOCUS, vix_console_onfocus);
  simtk_event_connect (entry, SIMTK_EVENT_SUBMIT, vix_console_onsubmit);
  
  scprintf (console, motd);

  simtk_widget_make_draggable (consolewindow);
  
  return 0;
}

int
main (int argc, char *argv[], char *envp[])
{
  display_t *disp;  
  int i;
  
  if ((disp = display_new (1024, 768)) == NULL)
    exit (EXIT_FAILURE);

#ifndef SDL2_ENABLED
  SDL_EnableKeyRepeat (SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#endif
  
  if (simtk_init_from_display (disp) == -1)
  {
    fprintf (stderr, "%s: no memory to alloc simtk root container\n", argv[0]);
    exit (EXIT_FAILURE);
  }
  
  simtk_container_set_background (simtk_get_root_container (), VIX_BG_PATH);

  if (vix_console_open (simtk_get_root_container ()) == -1)
  {
    fprintf (stderr, "%s: cannot create Vix console window!\n", argv[0]);
    exit (EXIT_FAILURE);
  }
  
  /* TODO: global list of file maps */
  
  for (i = 1; i < argc; ++i)
    if (vix_open_file (argv[i]) == -1)
      scprintf (console, "error: cannot open %s: %s\n", argv[i], strerror (errno));
  
  simtk_event_loop (simtk_get_root_container ());

  vix_close_all_files ();
  
  return 0;
}

