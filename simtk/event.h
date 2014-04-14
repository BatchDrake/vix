#ifndef _SIMTK_EVENT_H
#define _SIMTK_EVENT_H

enum simtk_event_type
{
  SIMTK_EVENT_CREATE,
  SIMTK_EVENT_DESTROY,
  SIMTK_EVENT_REDRAW,
  SIMTK_EVENT_MOUSEDOWN,
  SIMTK_EVENT_MOUSEUP,
  SIMTK_EVENT_MOUSEMOVE,
  SIMTK_EVENT_KEYDOWN,
  SIMTK_EVENT_KEYUP,
  SIMTK_EVENT_FOCUS,
  SIMTK_EVENT_BLUR,
  SIMTK_EVENT_MAX
};

struct simtk_event
{
  int button;
  int x, y;
};

struct simtk_container;
struct simtk_widget;

void simtk_container_event_cascade (struct simtk_container *, enum simtk_event_type, struct simtk_event *);
void simtk_event_loop (struct simtk_container *);
void simtk_widget_trigger_create (struct simtk_widget *);
void simtk_container_trigger_create_all (struct simtk_container *);
#endif /* _SIMTK_EVENT_H */
