#include <string.h>

#include <draw.h>
#include <util.h>

#include "event.h"
#include "widget.h"

struct simtk_container *root;

struct simtk_container *
simtk_container_new (int x, int y, int width, int height)
{
  struct simtk_container *new;

  if ((new = calloc (1, sizeof (struct simtk_container))) == NULL)
    return NULL;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    free (new);
    return NULL;
  }
  
  new->x = x;
  new->y = y;

  new->width  = width;
  new->height = height;

  return new;
}

void
simtk_container_lock (const struct simtk_container *cont)
{
  SDL_mutexP (cont->lock);
}


void
simtk_container_unlock (const struct simtk_container *cont)
{
  SDL_mutexV (cont->lock);
}

void
simtk_container_set_background (struct simtk_container *cont, const char *path)
{
  simtk_container_lock (cont);

  if (cont->background != NULL)
    draw_free (cont->background);
  
  cont->background = draw_from_bmp (path);
  cont->background_dirty = 1;
  
  simtk_container_unlock (cont);
}

void
simtk_container_destroy (struct simtk_container *cont)
{
  int i;

  simtk_container_lock (cont);

  if (cont->background != NULL)
    draw_free (cont->background);
  
  for (i = 0; i < cont->widget_count; ++i)
    if (cont->widget_list[i] != NULL)
      __simtk_widget_destroy (cont->widget_list[i], 0);

  if (cont->widget_list != NULL)
    free (cont->widget_list);

  simtk_container_unlock (cont);
  
  SDL_DestroyMutex (cont->lock);

  free (cont);
}

void
simtk_container_set_display (struct simtk_container *cont, display_t *disp)
{
  cont->disp = disp;
}

display_t *
simtk_container_get_display (struct simtk_container *cont)
{
  return cont->disp;
}

int
simtk_init_from_display (display_t *disp)
{
  if ((root = simtk_container_new (0, 0, disp->width, disp->height)) == NULL)
    return -1;

  root->screen_end_x = disp->width;
  root->screen_end_y = disp->height;
  
  simtk_container_set_display (root, disp);

  return 0;
}

struct simtk_container *
simtk_get_root_container (void)
{
  return root;
}

static int
__simtk_widget_compare_zorder (const void *a, const void *b)
{
  const struct simtk_widget *wa, *wb;

  wa = *((const struct simtk_widget **) a);
  wb = *((const struct simtk_widget **) b);

  if (wa == NULL)
    return HUGE_Z;
  else if (wb == NULL)
    return -HUGE_Z;
  
  return wa->z - wb->z;
}

void
__simtk_sort_widgets (struct simtk_container *cont)
{
  int i;
  
  qsort (cont->widget_list, cont->widget_count, sizeof (struct widget_list *), __simtk_widget_compare_zorder);

  for (i = 0; i < cont->widget_count; ++i)
    if (cont->widget_list[i] != NULL)
    {
      simtk_widget_lock (cont->widget_list[i]);

      cont->widget_list[i]->z = i;
      cont->widget_list[i]->next = (i < (cont->widget_count - 1)) ? cont->widget_list[i + 1] : NULL;

      simtk_widget_unlock (cont->widget_list[i]);
    }
}

void
simtk_sort_widgets (struct simtk_container *cont)
{
  int i;

  simtk_container_lock (cont);
  
  __simtk_sort_widgets (cont);

  simtk_container_unlock (cont);
}

void
simtk_widget_get_absolute (struct simtk_widget *widget, int *x, int *y)
{
  simtk_widget_lock (widget);
  
  *x = widget->x + widget->parent->x;
  *y = widget->y + widget->parent->y;

  simtk_widget_unlock (widget);
}

void
simtk_widget_bring_front (struct simtk_widget *widget)
{
  int must_sort = 0;
  
  simtk_widget_lock (widget);
  
  if (widget->next != NULL)
  {
    widget->z = HUGE_Z;

    must_sort = 1;
  }

  simtk_widget_unlock (widget);

  if (must_sort)
  {
    simtk_sort_widgets (widget->parent);
    
    simtk_container_make_dirty (widget->parent);
  }
}

void
simtk_widget_focus (struct simtk_widget *widget)
{
  struct simtk_widget *old;
  struct simtk_event ign = {0};
  
  simtk_container_lock (widget->parent);

  old = widget->parent->current_widget;
  
  widget->parent->current_widget = widget;

  simtk_container_unlock (widget->parent);

  /* Signal the involved widgets. Which order is the best? */
  trigger_hook (widget->event_hooks, SIMTK_EVENT_FOCUS, &ign);
  trigger_hook (old->event_hooks, SIMTK_EVENT_BLUR, &ign);
}

int
__simtk_widget_add_container (struct simtk_widget *widget, struct simtk_container *cont)
{
  return PTR_LIST_APPEND_CHECK (widget->container, cont);
}

struct simtk_widget *
simtk_widget_new (struct simtk_container *cont, int x, int y, int width, int height)
{
  struct simtk_widget *new;

  if ((new = calloc (1, sizeof (struct simtk_widget))) == NULL)
    return NULL;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    free (new);
    
    return NULL;
  }
  
  if ((new->inheritance = strdup ("SimtkWidget")) == NULL)
  {
    SDL_DestroyMutex (new->lock);
    
    free (new);

    return NULL;
  }
  
  new->parent = cont;
  new->x = x;
  new->y = y;
  new->width  = width;
  new->height = height;
  new->z      = cont->current_zorder++;
  new->current_buff = 0;
  new->blurred_border_color = DEFAULT_BLURD_BORDER;
  new->focused_border_color = DEFAULT_FOCUS_BORDER;

  new->background = DEFAULT_BACKGROUND;
  new->foreground = DEFAULT_FOREGROUND;

  if ((new->buffers[0] = calloc (new->width * new->height, sizeof (uint32_t))) == NULL)
  {
    SDL_DestroyMutex (new->lock);
    
    free (new->inheritance);
    
    free (new);

    return NULL;
  }

  if ((new->buffers[1] = calloc (new->width * new->height, sizeof (uint32_t))) == NULL)
  {
    free (new->buffers[0]);
    
    SDL_DestroyMutex (new->lock);
    
    free (new->inheritance);
    
    free (new);

    return NULL;
  }

  
  if ((new->event_hooks = hook_bucket_new (SIMTK_EVENT_MAX)) == NULL)
  {
    free (new->buffers[0]);
    free (new->buffers[1]);
    
    SDL_DestroyMutex (new->lock);
    
    free (new->inheritance);

    free (new);
    
    return NULL;
  }

  simtk_container_lock (cont);
  
  if (PTR_LIST_APPEND_CHECK (cont->widget, new) == -1)
  {
    simtk_container_unlock (cont);

    free (new->buffers[0]);
    free (new->buffers[1]);

    SDL_DestroyMutex (new->lock);
    
    hook_bucket_free (new->event_hooks);

    free (new->inheritance);
    
    free (new);

    return NULL;
  }
  
  simtk_container_unlock (cont);
  
  cont->current_widget = new;

  simtk_set_redraw_pending ();
  
  return new;
}

void
simtk_container_make_dirty (struct simtk_container *container)
{
  container->dirty = 1;

  if (container->container_widget != NULL)
    simtk_widget_make_dirty (container->container_widget);
}

void
simtk_widget_make_dirty (struct simtk_widget *widget)
{
  widget->dirty = 1;

  simtk_container_make_dirty (widget->parent);
}

void
simtk_widget_switch_buffers (struct simtk_widget *widget)
{
  simtk_widget_lock (widget);

  widget->current_buff = !widget->current_buff;

  widget->switched = 1;
  
  simtk_widget_make_dirty (widget);

  simtk_widget_unlock (widget);

  simtk_set_redraw_pending ();
}

void
simtk_widget_lock (const struct simtk_widget *widget)
{
  SDL_mutexP (widget->lock);
}

void
simtk_widget_unlock (const struct simtk_widget *widget)
{
  SDL_mutexV (widget->lock);
}

void
simtk_widget_move (struct simtk_widget *widget, int x, int y)
{
  simtk_widget_lock (widget);
  
  widget->x = x;
  widget->y = y;
  widget->parent->background_dirty = 1;
  
  simtk_widget_unlock (widget); 
}

int
simtk_widget_is_focused (struct simtk_widget *widget)
{
  return widget->parent->current_widget == widget;
}

void
simtk_event_connect (struct simtk_widget *widget,
		     enum simtk_event_type event,
		     void *handler)
{
  (void) hook_register (widget->event_hooks, event, handler, widget);
}

void
simtk_widget_set_opaque (struct simtk_widget *widget, void *opaque)
{
  simtk_widget_lock (widget);
  
  widget->opaque = opaque;

  simtk_widget_unlock (widget);
}

void *
simtk_widget_get_opaque (const struct simtk_widget *widget)
{
  void *opaque;

  simtk_widget_lock (widget);

  opaque = widget->opaque;

  simtk_widget_unlock (widget);
  
  return opaque;
}

void
simtk_widget_set_redraw_query_function (struct simtk_widget *widget, int (*child_is_dirty) (struct simtk_widget *))
{
  simtk_widget_lock (widget);

  widget->child_is_dirty = child_is_dirty;

  simtk_widget_unlock (widget);
}

int
simtk_widget_is_dirty (struct simtk_widget *widget)
{
  int itis = widget->dirty;

  if (widget->child_is_dirty != NULL)
    itis = itis && (widget->child_is_dirty) (widget);
  
  return itis;
}

/* Perform a wiser approch with mutexes */
void
__simtk_widget_destroy (struct simtk_widget *widget, int remove_from_parent)
{
  int i;
  struct simtk_event event = {0, 0, 0};  
  struct simtk_container *cont = widget->parent;

  if (remove_from_parent)
  {
    /* Lock container!!! */
    for (i = 0; i < cont->widget_count; ++i)
      if (cont->widget_list[i] == widget)
      {
        cont->widget_list[i] = NULL;
        break;
      }
  }

  /* It's safe to send the destroy signal and cleanup the properties of
   * this widget */

  trigger_hook (widget->event_hooks, SIMTK_EVENT_DESTROY, &event);
  
  for (i = 0; i < widget->container_count; ++i)
    if (widget->container_list[i] != NULL)
      simtk_container_destroy (widget->container_list[i]);

  if (widget->container_list != NULL)
    free (widget->container_list);

  hook_bucket_free (widget->event_hooks);

  free (widget->inheritance);

  SDL_DestroyMutex (widget->lock);

  free (widget->buffers[0]);
  free (widget->buffers[1]);
      
  free (widget);
}

void
simtk_widget_destroy (struct simtk_widget *widget)
{
  __simtk_widget_destroy (widget, 1);
}

int
simtk_widget_inheritance_add (struct simtk_widget *widget, const char *derivated)
{
  char *new_inheritance;

  simtk_widget_lock (widget);
  
  if ((new_inheritance = malloc (strlen (widget->inheritance) + strlen (derivated) + 3)) == NULL)
  {
    simtk_widget_unlock (widget);
    
    return -1;
  }
  
  memcpy (new_inheritance, widget->inheritance, strlen (widget->inheritance));
  memcpy (new_inheritance + strlen (widget->inheritance), "->", 2);
  strcpy (new_inheritance + strlen (widget->inheritance) + 2, derivated);

  free (widget->inheritance);

  widget->inheritance = new_inheritance;

  simtk_widget_unlock (widget);
  
  return 0;
}

int
simtk_widget_is_class (struct simtk_widget *widget, const char *classname)
{
  simtk_widget_lock (widget);
  
  if (strcmp (widget->inheritance, classname) == 0)
  {
    simtk_widget_unlock (widget);
    
    return 1;
  }
  
  if (strlen (widget->inheritance) > strlen (classname) + 2)
    if (strncmp (widget->inheritance, classname, strlen (classname)) == 0 && strncmp (widget->inheritance + strlen (classname), "->", 2) == 0)
    {
      simtk_widget_unlock (widget);
      
      return 1;
    }

  simtk_widget_unlock (widget);
  
  return 0;
}

void
simtk_widget_set_background (struct simtk_widget *widget, uint32_t color)
{
  simtk_widget_lock (widget);
  
  widget->background = color;
  
  simtk_widget_unlock (widget);
}

void
simtk_widget_set_foreground (struct simtk_widget *widget, uint32_t color)
{
  simtk_widget_lock (widget);
  
  widget->foreground = color;

  simtk_widget_unlock (widget);
}
