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
#include <map.h>

#include <libguile.h>

PTR_LIST_EXTERN (struct filemap, map);

SCM vix_open_file_hook;

SCM_DEFINE (vix_file_list, "vix-file-list", 0, 0, 0,
	    (void),
	    "Returns a list of opened file numbers.")
{
  SCM result = SCM_EOL;
  int i;

  for (i = 0; i < map_count; ++i)
    if (map_list[i] != NULL)
      result = scm_cons (scm_from_int (i), result);

  return result;
}


SCM_DEFINE (vix_file_exists, "vix-file-exists?", 1, 0, 0,
	    (SCM id),
	    "Returns true if the specified file number is valid.")
{
  SCM result = SCM_BOOL_F;
  int _id;
  
  SCM_ASSERT_TYPE (scm_is_integer (id), id, SCM_ARG1, "vix-file-exists?", "integer");

  _id = scm_to_int (id);
  
  if (_id >= 0 && _id < map_count)
    if (map_list[_id] != NULL)
      result = SCM_BOOL_T;
  
  return result;
}


SCM_DEFINE (vix_file_path, "vix-file-path", 1, 0, 0,
	    (SCM id),
	    "Returns the path of an opened file.")
{
  SCM result = SCM_BOOL_F;
  int _id;
  
  SCM_ASSERT_TYPE (scm_is_integer (id), id, SCM_ARG1, "vix-file-path", "integer");

  _id = scm_to_int (id);
  
  if (_id >= 0 && _id < map_count)
    if (map_list[_id] != NULL)
      result = scm_from_utf8_string (map_list[_id]->path);
  
  return result;
}

SCM_DEFINE (vix_file_size, "vix-file-size", 1, 0, 0,
	    (SCM id),
	    "Returns the size of an opened file.")
{
  SCM result = SCM_BOOL_F;
  int _id;
  
  SCM_ASSERT_TYPE (scm_is_integer (id), id, SCM_ARG1, "vix-file-path", "integer");

  _id = scm_to_int (id);
  
  if (_id >= 0 && _id < map_count)
    if (map_list[_id] != NULL)
      result = scm_from_int (map_list[_id]->size);
	
  return result;
}


SCM_DEFINE (vix_get_bytes, "vix-get-bytes", 3, 0, 0,
	    (SCM id, SCM start, SCM length),
	    "Returns a bytevector from an opened file.")
{
  SCM result;

  uint64_t _start;
  uint64_t _length;
  int _id;
  
  SCM_ASSERT_TYPE (scm_is_integer (id), id, SCM_ARG1, "vix-get-bytes", "integer");
  SCM_ASSERT_TYPE (scm_is_integer (start), start, SCM_ARG2, "vix-get-bytes", "integer");
  SCM_ASSERT_TYPE (scm_is_integer (length), length, SCM_ARG3, "vix-get-bytes", "integer");

  result = scm_c_make_bytevector (0);

  _id = scm_to_int (id);
  _start = scm_to_int (start);
  _length = scm_to_int (length);
  
  if (_id >= 0 && _id < map_count)
    if (map_list[_id] != NULL)
    {
      if (_start + _length > map_list[_id]->size)
	_length = map_list[_id]->size - _start;

      if (_length > 0)
      {
	result = scm_c_make_bytevector (_length);

	memcpy (SCM_BYTEVECTOR_CONTENTS (result), map_list[_id]->base + _start, _length);
      }
    }
  
  return result;
}

SCM_DEFINE (vix_mark_region, "vix-mark-region", 5, 0, 0,
	    (SCM id, SCM start, SCM length, SCM name, SCM color),
	    "Marks a region on an opened file.")
{
  SCM result = SCM_BOOL_F;
  uint64_t _start;
  uint64_t _length;
  uint32_t _color;
  char *_name;
  int _id;
  
  SCM_ASSERT_TYPE (scm_is_integer (id), id, SCM_ARG1, "vix-mark-region", "integer");
  SCM_ASSERT_TYPE (scm_is_integer (start), start, SCM_ARG2, "vix-mark-region", "integer");
  SCM_ASSERT_TYPE (scm_is_integer (length), length, SCM_ARG3, "vix-mark-region", "integer");
  SCM_ASSERT_TYPE (scm_is_string (name), name, SCM_ARG4, "vix-mark-region", "string");
  SCM_ASSERT_TYPE (scm_is_integer (color), color, SCM_ARG5, "vix-mark-region", "integer");

  _id = scm_to_int (id);
  _start = scm_to_int (start);
  _length = scm_to_int (length);
  _color = scm_to_int (color);
  _name = scm_to_locale_string (name);

  if (_id >= 0 && _id < map_count)
    if (map_list[_id] != NULL)
      result = simtk_bitview_mark_region (map_list[_id]->hwid, _name, _start, _length, _color, OPAQUE (0)) == -1 ? SCM_BOOL_F : SCM_BOOL_T;
  
  return result;
}

static void *
__vix_init_file (void *ignored)
{
  #include "map.x"
}

static void *
__vix_open_file_hook_run (void *data)
{
  scm_run_hook (vix_open_file_hook, scm_cons (scm_from_int ((int) data), SCM_EOL));
}

void
vix_open_file_hook_run (int num)
{
  scm_with_guile (__vix_open_file_hook_run, (void *) num);
}

void
vix_scripting_init_filemap (void)
{
  vix_open_file_hook = scm_make_hook (scm_from_int (1));

  scm_c_define ("vix-open-file-hook", vix_open_file_hook);
}
