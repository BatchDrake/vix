#include <stdarg.h>
#include <stdio.h>

#include <draw.h>

#include <simtk/simtk.h>

#include "consoleview.h"
#include "vix.h"

struct simtk_consoleview_properties *
simtk_consoleview_properties_new (void)
{
  struct simtk_consoleview_properties *new;

  if ((new = calloc (1, sizeof (struct simtk_consoleview_properties))) == NULL)
    return NULL;

  if ((new->lock = SDL_CreateMutex ()) == NULL)
  {
    free (new);

    return NULL;
  }
  
  new->fg = SIMTK_CONSOLEVIEW_DEFAULT_FG;
  new->bg = SIMTK_CONSOLEVIEW_DEFAULT_BG;

  return new;
}

void
simtk_consoleview_properties_lock (const struct simtk_consoleview_properties *prop)
{
  SDL_mutexP (prop->lock);
}


void
simtk_consoleview_properties_unlock (const struct simtk_consoleview_properties *prop)
{
  SDL_mutexV (prop->lock);
}

void
simtk_consoleview_properties_destroy (struct simtk_consoleview_properties *prop)
{
  SDL_DestroyMutex (prop->lock);
  
  free (prop);
}

struct simtk_consoleview_properties *
simtk_consoleview_get_properties (struct simtk_widget *consoleview)
{
  return (struct simtk_consoleview_properties *) simtk_textview_get_opaque (consoleview);
}

static inline void
__scroll (struct simtk_widget *consoleview)
{
  struct simtk_textview_properties *prop = simtk_textview_get_properties (consoleview);
  struct simtk_consoleview_properties *cprop = simtk_consoleview_get_properties (consoleview);
  
  int i;
  
  memmove (prop->text, prop->text + prop->cols, prop->cols * (prop->rows - 1));
  memmove (prop->fore, prop->fore + prop->cols, prop->cols * (prop->rows - 1) * sizeof (uint32_t));
  memmove (prop->back, prop->back + prop->cols, prop->cols * (prop->rows - 1) * sizeof (uint32_t));

  for (i = 0; i < prop->cols; ++i)
  {
    prop->text[(prop->rows - 1) * prop->cols + i] = 0;
    prop->fore[(prop->rows - 1) * prop->cols + i] = cprop->fg;
    prop->back[(prop->rows - 1) * prop->cols + i] = cprop->bg;
  }
}

static inline void
__cputchar (struct simtk_widget *consoleview, char c)
{
  struct simtk_textview_properties *prop = simtk_textview_get_properties (consoleview);
  struct simtk_consoleview_properties *cprop = simtk_consoleview_get_properties (consoleview);
  
  int new_x = cprop->x;
  int new_y = cprop->y;
  int putchar = 0;
  
  switch (c)
  {
  case '\r':
    new_x = 0;
    break;

  case '\n':
    new_x = 0;
    
  case '\v':
    ++new_y;
    break;

  case '\t':
    new_x = (1 + ((new_x) >> 3)) << 3;
    break;
    
  case '\b':
    if (--new_x < 0)
      new_x = 0;
    break;
    
  default:
    ++putchar;
    ++new_x;
  }

  if (putchar)
    simtk_textview_set_text (consoleview, cprop->x, cprop->y, cprop->fg, cprop->bg, &c, 1);
  
  if (new_x >= prop->cols)
  {
    new_x = 0;
    ++new_y;
  }

  if (new_y >= prop->rows)
  {
    new_y = prop->rows - 1;
    
    __scroll (consoleview);
  }

  cprop->x = new_x;
  cprop->y = new_y;
}

static inline void
__cputs (struct simtk_widget *consoleview, const char *s)
{
  int i;

  for (i = 0; i < strlen (s); ++i)
    __cputchar (consoleview, s[i]);
}

static inline void
__cprintf (struct simtk_widget *consoleview, const char *fmt, ...)
{
  static char buf[256];
  va_list ap;

  va_start (ap, fmt);
  
  vsnprintf (buf, sizeof (buf) - 1, fmt, ap);

  __cputs (consoleview, buf);
  
  va_end (ap);
}

static inline void
__clear (struct simtk_widget *consoleview)
{
  struct simtk_textview_properties *prop     = simtk_textview_get_properties (consoleview);
  struct simtk_consoleview_properties *cprop = simtk_consoleview_get_properties (consoleview);
  char space = '\0';
  
  int i, j;

  for (j = 0; j < prop->rows; ++j)
    for (i = 0; i < prop->cols; ++i)
      simtk_textview_set_text (consoleview, i, j, cprop->fg, cprop->bg, &space, 1);
}

static void
simtk_consoleview_setup (struct simtk_widget *console)
{
  static char nullpage[4096];
  
  __clear (console);
  __cputs (console, "Welcome to the ViX-Guile console\n\n");
  __cputs (console, "This is a LISP interpreter. Execute (help) to get a list on available commands\n\n");
  __cputs (console, "vix:> \n");
}

struct simtk_widget *
simtk_consoleview_new (struct simtk_container *cont, int x, int y, int rows, int cols)
{
  struct simtk_widget *new;
  struct cpi_disp_font *font;
  struct simtk_consoleview_properties *prop;

  if ((prop = simtk_consoleview_properties_new ()) == NULL)
    return NULL;
  
  if ((new = simtk_textview_new (cont, x, y, rows, cols)) == NULL)
  {
    simtk_consoleview_properties_destroy (prop);
    
    return NULL;
  }

  simtk_textview_set_opaque (new, prop);
  
  simtk_widget_inheritance_add (new, "ConsoleView");

  simtk_consoleview_setup (new);

  return new;
}

