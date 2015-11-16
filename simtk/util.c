#include <string.h>

#include <draw.h>
#include <util.h>

#include "widget.h"

int drag_flag;
int drag_start_x;
int drag_start_y;
int moves;

static int
simtk_widget_onmousemove (enum simtk_event_type type,
	  simtk_widget_t *widget,
	  struct simtk_event *event)
{
  if (drag_flag && !simtk_is_drawing ())
  { 
    simtk_widget_move (widget,
		       event->x + widget->x - drag_start_x,
		       event->y + widget->y - drag_start_y);
  

    simtk_set_redraw_pending ();
  }
  
  return HOOK_RESUME_CHAIN;
}

static int
simtk_widget_onmousedown (enum simtk_event_type type,
	  simtk_widget_t *widget,
	  struct simtk_event *event)
{
  drag_flag = 1;
  
  drag_start_x = event->x;
  drag_start_y = event->y;

  return HOOK_RESUME_CHAIN;
}


static int
simtk_widget_onmouseup (enum simtk_event_type type,
	  simtk_widget_t *widget,
	  struct simtk_event *event)
{
  if (drag_flag)
  {
    drag_flag = 0;

    simtk_set_redraw_pending ();
  }
  
  return HOOK_RESUME_CHAIN;
}


/* Add lostfocus */
void
simtk_widget_setup_draggable (simtk_widget_t *widget)
{
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEMOVE, simtk_widget_onmousemove);
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEDOWN, simtk_widget_onmousedown);
  simtk_event_connect (widget, SIMTK_EVENT_MOUSEUP,   simtk_widget_onmouseup);
}
