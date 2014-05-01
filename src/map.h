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
void filemap_jump_to_offset (struct filemap *, uint32_t);
void filemap_destroy (struct filemap *);

#endif /* _MAP_H */
