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

#include <draw.h>

#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "region.h"

struct file_region *
file_region_new (const char *name, uint64_t start, uint64_t length, uint32_t fgcolor, uint32_t bgcolor)
{
  struct file_region *new;
  int i;
  double t;
  uint8_t r, g, b;
  
  if ((new = calloc (1, sizeof (struct file_region))) == NULL)
    return NULL;

  if ((new->name = strdup (name)) == NULL)
  {
    free (new);
    return NULL;
  }

  new->start = start;
  new->length = length;

  for (i = 0; i < 8; ++i)
  {
    t = (double) i / 7.0;

    r = 0.5 * (1 + t) * G_RED (fgcolor);
    g = 0.5 * (1 + t) * G_GREEN (fgcolor);
    b = 0.5 * (1 + t) * G_BLUE (fgcolor);
    
    new->colors[i] = ARGB (0xff, r, g, b); 
  }
  
  new->bgcolor = bgcolor;

  return new;
}

void
file_region_destroy (struct file_region *region)
{
  free (region->name);
  free (region);
}

void
file_region_dtor (void *region, void *data)
{
  file_region_destroy ((struct file_region *) region);
}

rbtree_t *
file_region_tree_new (void)
{
  rbtree_t *result;

  if ((result = rbtree_new ()) == NULL)
    return NULL;

  rbtree_set_dtor (result, file_region_dtor, NULL);
}

int
file_region_register (rbtree_t *tree, const char *name, uint64_t start, uint64_t length, uint32_t fgcolor, uint32_t bgcolor)
{
  struct file_region *new;

  if ((new = file_region_new (name, start, length, fgcolor, bgcolor)) == NULL)
    return -1;

  if (rbtree_insert (tree, start, new) == -1)
  {
    file_region_destroy (new);
    return -1;
  }

  return 0;
}

struct rbtree_node *
file_region_find (rbtree_t *tree, uint64_t offset)
{
  return rbtree_search (tree, offset, RB_LEFTWARDS);
}
