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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <vix.h>
#include <simtk/simtk.h>

#include <libguile.h>

extern struct simtk_widget *entry;
extern struct simtk_widget *console;
extern struct simtk_widget *consolewindow;

SCM_DEFINE (vix_echo, "vix-echo", 1, 0, 0,
            (SCM text),
            "Output text to the console window.")
{
  char *string;

  if ((string = scm_to_locale_string (text)) != NULL)
  {
    scprintf (console, "%s\n", string);
    free (string);
  }
  
  return SCM_BOOL_T;
}

static void *
__vix_init (void *ignored)
{
#include "init.x"
}

void
vix_scripting_init (void)
{
  scm_with_guile (__vix_init, NULL);
}

static void *
__vix_try_eval_file (void *path)
{
  if (!scm_to_bool (scm_c_primitive_load ((const char *) path)))
    scprintf (console, "Cannot load `%s'\n", path);
}

int
vix_scripting_directory_init (const char *dirpath)
{
  DIR *dir;
  struct dirent *ent;
  char *fullpath;
  
  if ((dir = opendir (dirpath)) == NULL)
    return -1;

  while ((ent = readdir (dir)) != NULL)
  {
    if (strcmp (ent->d_name + strlen (ent->d_name) - 4,
		".scm") == 0)
    {
      if ((fullpath = strbuild ("%s/%s", dirpath, ent->d_name)) != NULL)
      {
	scprintf (console, "Loading %s..\n", fullpath);

	scm_with_guile (__vix_try_eval_file, fullpath);
	
	free (fullpath);
      }
    }
  }

  closedir (dir);
  
  return 0;
}
