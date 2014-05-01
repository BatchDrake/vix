#ifndef _SIMTK_EVENT_H
#define _SIMTK_EVENT_H

#define SIMTK_HEARTBEAT_DELAY_MS 250

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
  SIMTK_EVENT_HEARTBEAT,
  SIMTK_EVENT_SUBMIT,
  
  /* Don't add events beyond this line */
  SIMTK_EVENT_MAX
};

#define SIMTK_KBD_MOD_CTRL     1
#define SIMTK_KBD_MOD_ALT      2
#define SIMTK_KBD_MOD_SHIFT    4
#define SIMTK_KBD_MOD_CAPSLOCK 8

struct simtk_event
{
  int button;
  int character;
  int mod;
  int x, y;
};

struct simtk_container;
struct simtk_widget;

void simtk_container_event_cascade (struct simtk_container *, enum simtk_event_type, struct simtk_event *);
void simtk_event_loop (struct simtk_container *);
void simtk_widget_trigger_create (struct simtk_widget *);
void simtk_container_trigger_create_all (struct simtk_container *);
void simtk_widget_focus (struct simtk_widget *);
int  simtk_in_event_loop (void);

#endif /* _SIMTK_EVENT_H */
