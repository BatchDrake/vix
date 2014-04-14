/*
 *    <one line to give the program's name and a brief idea of what it does.>
 *    Copyright (C) <year>  <name of author>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <stdlib.h>
#include <util.h>
#include <hook.h>

#define ASSERT(x)

struct hook_bucket *
hook_bucket_new (int hooks)
{
  struct hook_bucket *new;
  int i;
  
  if (hooks < 1)
  {
    ERROR ("hooks < 1? wtf\n");
    return NULL;
  }
  
  if ((new = calloc (1, sizeof (struct hook_bucket) + 
                 sizeof (struct hook_func) * (hooks - 1))) == NULL)
    return NULL;
                 
    
  new->hb_hook_count = hooks;
  
  for (i = 0; i < hooks; i++)
    new->hb_hooks[i] = NULL;
    
  return new;
}

inline void
hook_add (struct hook_bucket *bucket, int code, struct hook_func *cb)
{
  ASSERT (bucket != NULL);
  ASSERT (cb != NULL);
  
  if (bucket->hb_hooks[code] != NULL)
    bucket->hb_hooks[code]->hf_prev = cb;
  
  cb->hf_next = bucket->hb_hooks[code];
  cb->hf_prev = NULL;
  
  bucket->hb_hooks[code] = cb;
}

int
hook_register (struct hook_bucket *bucket, int code,
               int (*func) (int, void *, void *),
               void *data)
{
  struct hook_func *cb;
  
  if (!IN_BOUNDS (code, bucket->hb_hook_count))
  {
    ERROR ("code (%d) out of bounds!\n", code);
    return -1;
  }
  
  
  if ((cb = xmalloc (sizeof (struct hook_func))) == NULL)
    return -1;
    
  cb->hf_func = func;
  cb->hf_data = data;
  
  hook_add (bucket, code, cb);
  
  return 0;
}

inline void
hook_func_free (struct hook_bucket *bucket, int code)
{
  struct hook_func *func, *next;
  
  if (!IN_BOUNDS (code, bucket->hb_hook_count))
  {
    ERROR ("code (%d) out of bounds!\n", code);
    return;
  }
  
  
  func = bucket->hb_hooks[code];
  
  while (func != NULL)
  {
    next = func->hf_next;
    free (func);
    func = next;
  }
}

void
hook_bucket_free (struct hook_bucket *bucket)
{
  int i;
  
  for (i = 0; i < bucket->hb_hook_count; i++)
    hook_func_free (bucket, i);
    
  free (bucket);
}

int
trigger_hook_inverse (struct hook_bucket *bucket, int code, void *data)
{
  int acum;
  struct hook_func *func;
    
  if (!IN_BOUNDS (code, bucket->hb_hook_count))
  {
    ERROR ("code (%d) out of bounds!\n", code);
    return -1;
  }
  
  acum = 0;
  
  func = bucket->hb_hooks[code];
  
  while (func != NULL)
  {
    if ((func->hf_func) (code, func->hf_data, data) == HOOK_LOCK_CHAIN)
    {
      acum++; /* What if hf_func fails? */
      break;
    }
    
    acum++;
    func = func->hf_next;
  }
  
  return acum;
}

int
trigger_hook (struct hook_bucket *bucket, int code, void *data)
{
  int acum;
  struct hook_func *func;
    
  if (!IN_BOUNDS (code, bucket->hb_hook_count))
  {
    ERROR ("code (%d) out of bounds!\n", code);
    return -1;
  }
  
  acum = 0;
  
  func = bucket->hb_hooks[code];

  while (func != NULL)
  {
    if (func->hf_next == NULL)
      break;

    func = func->hf_next;
  }
  
  while (func != NULL)
  {
    if ((func->hf_func) (code, func->hf_data, data) == HOOK_LOCK_CHAIN)
    {
      acum++; /* What if hf_func fails? */
      break;
    }
    
    acum++;
    func = func->hf_prev;
  }
  
  return acum;
}

