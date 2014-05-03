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

#ifndef _RBTREE_H
#define _RBTREE_H

#include <stdint.h>

enum rbtree_node_color
{
  RB_RED,
  RB_BLACK
};

typedef struct rbtree
{
  struct rbtree_node *root;
  struct rbtree_node *first, *last;

  void *node_dtor_data;
  
  void (*node_dtor) (void *, void *);
}
rbtree_t;

struct rbtree_node
{
  enum rbtree_node_color color;
  int64_t key;

  rbtree_t *owner;
  
  /* General tree structure */
  struct rbtree_node *parent, *left, *right;

  /* List view, to quickly iterate among nodes */
  struct rbtree_node *prev, *next;

  void *data;
};

rbtree_t *rbtree_new (void);
void rbtree_set_dtor (rbtree_t *, void (*) (void *, void *), void *);
void rbtree_debug (rbtree_t *, FILE *);
int  rbtree_insert (rbtree_t *, int64_t, void *);
void rbtree_clear (rbtree_t *);
void rbtree_destroy (rbtree_t *);

#endif /* _RBTREE_H */
