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

#ifndef _REGION_H
#define _REGION_H

#include <stdint.h>
#include <rbtree.h>

struct file_region
{
  char *name;

  uint64_t start;
  uint64_t length;

  uint32_t fgcolor;
  uint32_t bgcolor;  
};

rbtree_t *file_region_tree_new (void);
int file_region_register (rbtree_t *tree, const char *name, uint64_t start, uint64_t length, uint32_t fgcolor, uint32_t bgcolor);
struct rbtree_node *file_region_find (rbtree_t *, uint64_t);

#endif /* _REGION_H */
