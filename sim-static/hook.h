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
    
#ifndef _MISC_HOOK_H
#define _MISC_HOOK_H

#define HOOK_RESUME_CHAIN 0
#define HOOK_LOCK_CHAIN   1

/* Be careful: delays could be introduced due to sparse allocation. */
struct hook_func
{
  int (*hf_func) (int, void *, void *);
  void *hf_data;
  
  struct hook_func *hf_next;
  struct hook_func *hf_prev;
};

struct hook_bucket
{
  int hb_hook_count;
  
  struct hook_func *hb_hooks[1];
};

struct hook_bucket *hook_bucket_new (int);
int hook_register (struct hook_bucket *, int, int (*) (int, void *, void *),
               void *);
               
void hook_bucket_free (struct hook_bucket *);
int trigger_hook (struct hook_bucket *, int, void *);



#endif /* _MISC_HOOK_H */

