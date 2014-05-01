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

PTR_LIST (struct filemap, map);

#define STRING_SIZE 4

struct simtk_widget *entry;
struct simtk_widget *console;
struct simtk_widget *consolewindow;

void simtk_widget_make_draggable (struct simtk_widget *);

char *motd =
  "\n"
  "     Welcome to Vix - v0.1  \n"
  "     (c) 2014 Gonzalo J. Carracedo (BatchDrake)\n"
  "\n"
  "WHAT'S NEW:\n"
  "As you can already see, this console Window :D\n"
  "Before you ask, this console won't be the primary way to interact\n"
  "with Vix. Its sole purpose is to have a debug window, a place to\n"
  "run scripts, evaluate expressions and test new features. In the\n"
  "future, Vix will have menus, icons and keyboard shortcuts, which\n"
  "I hope it will be much nicer than typing commands in this window\n\n."
  "Other important feature is the Text Entry Widget, the one you\n"
  "see in the bottom of this window. You can type (obvious), do some basic\n"
  "line editing, selectig (arrow keys + shift), copying and pasting.\n"
  "I shall warn you, I've coded this during some spare time I found during\n"
  "lunch breaks and it's still buggy (there's some weird behavior when\n"
  "selecting from the start to the end of the line), etc.\n\n"
  "Also, there's a glitch when opening windows once the event loop has started,\n"
  "somehow window titles are not displayed until they got focus, hope to have\n"
  "that fixed soon. And I got back the fast scroll speed of biteye.\n\n"
  "Finally, you can get a list of commands by typing `help' and pressing ENTER\n\n"
  "Have fun! :)\n\n";

int
vix_console_onfocus (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
{
  /* Reject the focus */
  
  simtk_widget_focus (entry);
  
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
vix_open (arg_list_t *al)
{
  struct filemap *map;
  
  if (al->al_argc != 2)
  {
    scprintf (console, "%s: wrong number of arguments\n", al->al_argv[0]);
    scprintf (console, "Usage\n");
    scprintf (console, "  %s <path>\n\n", al->al_argv[0]);
  }

  if ((map = filemap_new (simtk_get_root_container (), al->al_argv[1])) == NULL)
  {
    scprintf (console, "%s: cannot open view for %s: %s\n", al->al_argv[0], al->al_argv[1], strerror (errno));
    
    exit (EXIT_FAILURE);
  }
  
  if (PTR_LIST_APPEND_CHECK (map, map) == -1)
  {
    scprintf (console, "%s: out of memory while registering file map\n", al->al_argv[0]);
    
    exit (EXIT_FAILURE);
  }
}

int
vix_console_onsubmit (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
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
        else
          scprintf (console, "Unknown command `%s'\n", al->al_argv[0]);
      }
      
      free_al (al);
    }

  simtk_entry_clear (widget);
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
  struct filemap *map;
  
  int i;
  
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

  if (vix_console_open (simtk_get_root_container ()) == -1)
  {
    fprintf (stderr, "%s: cannot create Vix console window!\n", argv[0]);
    exit (EXIT_FAILURE);
  }
  

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

