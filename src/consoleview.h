#ifndef _SRC_CONSOLEVIEW_H
#define _SRC_CONSOLEVIEW_H

#define SIMTK_CONSOLEVIEW_DEFAULT_FG OPAQUE (0xbfbfbf)
#define SIMTK_CONSOLEVIEW_DEFAULT_BG OPAQUE (0x1f2f2f)

struct simtk_consoleview_properties
{
  SDL_mutex *lock;

  int x, y;

  uint32_t fg;
  uint32_t bg;

  void *opaque;
};

struct simtk_widget *simtk_consoleview_new (struct simtk_container *cont, int x, int y, int rows, int cols);

#endif /* _SRC_CONSOLEVIEW_H */
