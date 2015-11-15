#include <draw.h>
#include <util.h>

#include "event.h"
#include "widget.h"
#include "window.h"

#include "primitives.h"

struct simtk_window_properties *
simtk_window_properties_new (const char *title)
{
  struct simtk_window_properties *new;

  if ((new = malloc (sizeof (struct simtk_window_properties))) == NULL)
    return NULL;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    free (new);

    return NULL;
  }
  
  if ((new->title = strdup (title)) == NULL)
  {
    SDL_DestroyMutex (new->lock);
    
    free (new);

    return NULL;
  }

  new->title_foreground = SIMTK_WINDOW_DEFAULT_TITLE_FOREGROUND;
  new->title_background = SIMTK_WINDOW_DEFAULT_TITLE_BACKGROUND;

  new->title_inactive_foreground = SIMTK_WINDOW_DEFAULT_TITLE_INACTIVE_FOREGROUND;
  new->title_inactive_background = SIMTK_WINDOW_DEFAULT_TITLE_INACTIVE_BACKGROUND;

  new->params_changed   = 0;

  new->opaque           = NULL;
  
  return new;
}

void
simtk_window_properties_lock (const struct simtk_window_properties *prop)
{
  SDL_mutexP (prop->lock);
}


void
simtk_window_properties_unlock (const struct simtk_window_properties *prop)
{
  SDL_mutexV (prop->lock);
}

void
simtk_window_properties_destroy (struct simtk_window_properties *prop)
{
  simtk_window_properties_lock (prop);
  
  free (prop->title);
  
  simtk_window_properties_unlock (prop);

  SDL_DestroyMutex (prop->lock);
  
  free (prop);
}

void *
simtk_window_get_opaque (const struct simtk_widget *widget)
{
  struct simtk_window_properties *prop;

  prop = simtk_window_get_properties (widget);

  return prop->opaque;
}

void
simtk_window_set_opaque (struct simtk_widget *widget, void *opaque)
{
  struct simtk_window_properties *prop;

  prop = simtk_window_get_properties (widget);

  prop->opaque = opaque;
}


int
simtk_widget_is_window (struct simtk_widget *widget)
{
  return simtk_widget_is_class (widget, "SimtkWidget->Window");
}

struct simtk_window_properties *
simtk_window_get_properties (const struct simtk_widget *widget)
{
  struct simtk_window_properties *prop;

  simtk_widget_lock (widget);
  
  prop  = simtk_widget_get_opaque (widget);

  simtk_widget_unlock (widget);

  return prop;
}

/* FIXME: fill a buffer, don't return the title buffer
   directly */
char *
simtk_window_get_title (struct simtk_widget *widget)
{
  struct simtk_window_properties *prop;

  prop = simtk_window_get_properties (widget);

  return prop->title;
}

static void
__simtk_window_render_frame (struct simtk_widget *widget)
{
  struct simtk_window_properties *prop;

  prop = simtk_window_get_properties (widget);

  simtk_widget_fbox (widget, 1,	1, widget->width - 2, 10,
	simtk_widget_is_focused (widget) ? prop->title_background : prop->title_inactive_background);
  
  simtk_widget_render_string_cpi (widget, cpi_disp_font_from_display (display_from_simtk_widget (widget)), 2, 2,
				  simtk_widget_is_focused (widget) ? prop->title_foreground : prop->title_inactive_foreground,
				  simtk_widget_is_focused (widget) ? prop->title_background : prop->title_inactive_background,
				  prop->title);

  simtk_widget_switch_buffers (widget);
}

struct simtk_container *
__simtk_window_get_body_container (struct simtk_widget *widget)
{
  if (widget->container_list != NULL)
      return widget->container_list[0];

  return NULL;
}

int
simtk_window_follow_redraw (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
{
  simtk_container_update_offset (__simtk_window_get_body_container (widget), widget->parent, widget->x + 1, widget->y + SIMTK_WINDOW_MIN_TITLE_HEIGHT + 2);
  
  simtk_redraw_container (__simtk_window_get_body_container (widget), event->button);

  return HOOK_RESUME_CHAIN;
}

int
simtk_window_create (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
{
  __simtk_window_render_frame (widget);

  simtk_sort_widgets (__simtk_window_get_body_container (widget));
  
  simtk_container_trigger_create_all (__simtk_window_get_body_container (widget));

  return HOOK_RESUME_CHAIN;
}

void
simtk_window_destroy (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
{
  simtk_window_properties_destroy (simtk_window_get_properties (widget));  
}

struct simtk_container *
simtk_window_get_body_container (struct simtk_widget *widget)
{
  struct simtk_container *cont;

  simtk_widget_lock (widget);

  cont = __simtk_window_get_body_container (widget);

  simtk_widget_unlock (widget);

  return cont;
}

int
simtk_window_focus_change (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
{
  struct simtk_window_properties *prop;

  prop = simtk_window_get_properties (widget);

  simtk_window_properties_lock (prop);

  __simtk_window_render_frame (widget);

  simtk_container_trigger_create_all (__simtk_window_get_body_container (widget));
  
  simtk_window_properties_unlock (prop);
  
  return HOOK_RESUME_CHAIN;
}

static void
simtk_window_generic_cascade (enum simtk_event_type type, struct simtk_widget *widget, struct simtk_event *event)
{
  struct simtk_container *cont;

  cont = simtk_window_get_body_container (widget);

  /* Correction for mouse events */
  event->y -= SIMTK_WINDOW_MIN_TITLE_HEIGHT + 2;
  
  simtk_container_event_cascade (cont, type, event);
}

static void
__simtk_window_connect_everything (struct simtk_widget *widget)
{
  simtk_event_connect (widget, SIMTK_EVENT_CREATE, simtk_window_create);
  simtk_event_connect (widget, SIMTK_EVENT_REDRAW, simtk_window_follow_redraw);
  simtk_event_connect (widget, SIMTK_EVENT_DESTROY, simtk_window_destroy);
  simtk_event_connect (widget, SIMTK_EVENT_FOCUS, simtk_window_focus_change);
  simtk_event_connect (widget, SIMTK_EVENT_BLUR, simtk_window_focus_change);
  simtk_event_connect (widget, SIMTK_EVENT_KEYDOWN, simtk_window_generic_cascade);
  simtk_event_connect (widget, SIMTK_EVENT_KEYUP, simtk_window_generic_cascade);
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEDOWN, simtk_window_generic_cascade);
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEUP, simtk_window_generic_cascade);
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEMOVE, simtk_window_generic_cascade);
  simtk_event_connect (widget, SIMTK_EVENT_HEARTBEAT, simtk_window_generic_cascade);
}

struct simtk_widget *
simtk_window_new (struct simtk_container *cont, int x, int y, int width, int height, const char *title)
{
  struct simtk_widget *new;
  struct simtk_window_properties *prop;
  struct simtk_container *win_cont;
  
  if (height < (SIMTK_WINDOW_MIN_HEIGHT))
    height = SIMTK_WINDOW_MIN_HEIGHT;
  
  if ((new = simtk_widget_new (cont, x, y, width, height)) == NULL)
    return NULL;

  if (simtk_widget_inheritance_add (new, "Window") == -1)
  {
    simtk_widget_destroy (new);
    
    return NULL;
  }
  
  if ((prop = simtk_window_properties_new (title)) == NULL)
  {
    simtk_widget_destroy (new);

    return NULL;
  }

  simtk_widget_set_opaque (new, prop);

  if ((win_cont = simtk_container_new (1, SIMTK_WINDOW_MIN_TITLE_HEIGHT + 1, width - 2, height - SIMTK_WINDOW_MIN_TITLE_HEIGHT - 3)) == NULL)
  {
    simtk_widget_destroy (new);

    return NULL;
  }

  simtk_container_set_display (win_cont, simtk_container_get_display (cont));

  win_cont->container_widget = new;

  if (__simtk_widget_add_container (new, win_cont) == -1)
  {
    simtk_container_destroy (win_cont);

    simtk_widget_destroy (new);

    return NULL;
  }

  __simtk_window_connect_everything (new);

  __simtk_window_render_frame (new);
   
  return new;
}

