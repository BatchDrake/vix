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


#ifndef _MAP_H
#define _MAP_H

#include <stdint.h>
#include <simtk/simtk.h>
#include "hilbert.h"

struct filemap
{
  void    *base;
  char    *path;
  uint32_t size;

  uint32_t offset;
  uint32_t search_offset;
  
  struct simtk_container *container;
  
  simtk_widget_t *hexview, *hexwid;
  simtk_widget_t *vbits, *vwid;
  simtk_widget_t *hbits, *hwid;

  PTR_LIST (simtk_widget_t, hilbert_window);
  PTR_LIST (simtk_widget_t, hilbert_widget);
};

struct filemap *filemap_new (struct simtk_container *, const char *);
void filemap_jump_to_offset (struct filemap *, uint32_t);
void filemap_destroy (struct filemap *);
int  filemap_open_hilbert (struct filemap *, int);

void filemap_search (struct filemap *, const void *, size_t);
void vix_open_file_hook_run (int);
int  vix_open_file (const char *path);
void vix_close_all_files (void);
void vix_scripting_init_filemap (void);

#endif /* _MAP_H */
