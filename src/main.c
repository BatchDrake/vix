/*
 * main.c: entry point for vix
 * Creation date: Tue Sep 10 13:50:07 2013
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <vix.h>
#include <simtk/simtk.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


#define STRING_SIZE 4

static inline int
is_ascii (char c)
{
  return c >= ' ' && c < '\x7f';
}

int
higlight_text (struct simtk_widget *widget, char *buf, int size)
{
  int last_text = -1;
  int p = 0;
  int i;
  int h = 0;
  
  for (i = 0; i < size; ++i)
  {
    if (is_ascii (buf[i]))
    {
      if (last_text == -1)
      {
	last_text = i;
	p = 1;
      }
      else
	++p;
    }
    else
    {
      if (last_text != -1)
      {
	if (p >= STRING_SIZE)
	  simtk_textview_set_text (widget, last_text, 0, OPAQUE (0x00ffff), 0xff000000, buf + last_text, p);
	++h;
	p = 0;
	last_text = -1;
      }
    }
  }

  return h;
}

/*

XXXX:XXXX | xx xx xx xx xx xx xx xx  xx xx xx xx xx xx xx xx | aaaaaaaaaaaaaaaa
 */

int
draw_a (enum simtk_event_type type,
	  struct simtk_widget *widget,
	  struct simtk_event *event)
{
  int i, j;

  for (j = 0; j < widget->height; ++j)
    for (i = 0; i < widget->width; ++i)
      simtk_widget_pset (widget, i, j, OPAQUE (((i & 1) ^ (j & 1)) ? 0x202020 : 0x404040));
  
  simtk_widget_render_string_cpi (widget, cpi_disp_font_from_display (display_from_simtk_widget (widget)), widget->width / 2 - 88, widget->height / 2,
				  OPAQUE (0xffbfbfbf),
				  0,
				  "This window is vagina.");

  simtk_widget_switch_buffers (widget);
  
  return HOOK_RESUME_CHAIN;
}


int
draw_b (enum simtk_event_type type,
	  struct simtk_widget *widget,
	  struct simtk_event *event)
{

  simtk_widget_render_string_cpi (widget, cpi_disp_font_from_display (display_from_simtk_widget (widget)), widget->width / 2 - 88, widget->height / 2,
				  OPAQUE (GREEN (0xff)),
				  0,
				  "This window is buttsex.");

  simtk_widget_switch_buffers (widget);
  
  return HOOK_RESUME_CHAIN;
}

int drag_flag;
int drag_start_x;
int drag_start_y;
int moves;

int
c_window_onmousemove (enum simtk_event_type type,
	  struct simtk_widget *widget,
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

int
c_window_onmousedown (enum simtk_event_type type,
	  struct simtk_widget *widget,
	  struct simtk_event *event)
{
  drag_flag = 1;
  
  drag_start_x = event->x;
  drag_start_y = event->y;

  return HOOK_RESUME_CHAIN;
}


int
c_window_onmouseup (enum simtk_event_type type,
	  struct simtk_widget *widget,
	  struct simtk_event *event)
{
  if (drag_flag)
  {
    drag_flag = 0;

    simtk_set_redraw_pending ();
  }
  
  return HOOK_RESUME_CHAIN;
}

int
draw_c (enum simtk_event_type type,
	  struct simtk_widget *widget,
	  struct simtk_event *event)
{
  simtk_widget_fbox (widget, 0, 0, widget->width - 1, widget->height - 1, 0x7f7f5200);
  
  simtk_widget_render_string_cpi (widget, cpi_disp_font_from_display (display_from_simtk_widget (widget)), 1, 1,
				  0,
				  OPAQUE (0xffa500),
				  "This is a text test that somehow overflows the widget width but who cares because I start wherever I want and blah blaj");

  simtk_widget_render_string_cpi (widget, cpi_disp_font_from_display (display_from_simtk_widget (widget)), widget->width / 2 - 88, widget->height / 2,
				  OPAQUE (0xffffff),
				  0x7f7f5200,
				  "This window is dildos.");

  simtk_widget_switch_buffers (widget);
  
  return HOOK_RESUME_CHAIN;
}

#define HEX_MINIMAL 0x80
#define ASCII_MINIMAL 0x80

void
render_hexdump (struct simtk_widget *widget,
                void *addr,
                void *vaddr,
                int row,
                size_t size)
{
  int i;
  char buffer[11];
  int common_y = 0;
  int this_x = 0;
  uint8_t byte;
  
  struct simtk_textview_properties *prop = simtk_textview_get_properties (widget);
  
  for (i = 0; i < size; ++i)
  {
    if (this_x == row || !i)
    {
      if (i)
        if (common_y++ >= prop->rows)
          break;
      
      this_x = 0;
      
      sprintf (buffer, "%04x:%04x ", (uint32_t) vaddr >> 16, (uint32_t) vaddr & 0xffff);
      
      simtk_textview_set_text (widget, 0, common_y, OPAQUE (0xff0000), 0x80000000, buffer, 10);
    }

    sprintf (buffer, "%02x ", byte = *((uint8_t *) addr));

    simtk_textview_set_text (widget, 3 * this_x + 10 , common_y, OPAQUE (RGB ((byte + HEX_MINIMAL) * 0xff / (0xff + HEX_MINIMAL), (byte + HEX_MINIMAL) * 0xa5 / (0xff + HEX_MINIMAL), 0)), 0x80000000, buffer, 3);
    simtk_textview_set_text (widget, 10 + 3 * row + this_x, common_y, OPAQUE (RGB (0, (byte + ASCII_MINIMAL) * 0xff / (0xff + HEX_MINIMAL), 0)), 0x80000000, addr++, 1);

    ++this_x;
    ++vaddr;
  }

  simtk_textview_render_text (widget);
    
  simtk_widget_switch_buffers (widget);
}

int
main (int argc, char *argv[], char *envp[])
{
  display_t *disp;
  struct simtk_widget *a, *b, *c;
  struct simtk_widget *window, *hwind, *vwind;
  int fd;
  void *addr;
  
  if (argc != 2)
  {
    fprintf (stderr, "Usage:\n\t%s <file>\n", argv[0]);

    exit (EXIT_FAILURE);
  }

  if ((fd = open (argv[1], O_RDONLY)) == -1)
  {
    fprintf (stderr, "%s: %s: %s\n", argv[0], argv[1], strerror (errno));

    exit (EXIT_FAILURE);
  }

  if ((addr = mmap (NULL, 1024 * 100, PROT_READ, MAP_PRIVATE, fd, 0)) == (caddr_t) -1)
  {
    fprintf (stderr, "%s: %s: %s\n", argv[0], argv[1], strerror (errno));

    exit (EXIT_FAILURE);
  }

  
  if ((disp = display_new (1024, 768)) == NULL)
    exit (EXIT_FAILURE);

  if (simtk_init_from_display (disp) == -1)
  {
    fprintf (stderr, "%s: no memory to alloc simtk root container\n", argv[0]);
    exit (EXIT_FAILURE);
  }

  window = simtk_window_new (simtk_get_root_container (), 10, 10, 640 - 8 * 5 - 4, 480, "Hexadecimal dump");
  hwind  = simtk_window_new (simtk_get_root_container (), 20, 20, 258, 512, "Bit rows view");
  vwind  = simtk_window_new (simtk_get_root_container (), 30, 30, 1026, 128, "Byte columns view");
  
/*  window = simtk_window_new (simtk_get_root_container (), 25, 25, 640, 480, "Super container window");*/

  c = simtk_textview_new (simtk_window_get_body_container (window),
                          1, 1, 80, 80);

/*  simtk_widget_set_background (a, (ARGB (0xc0, 0x00, 0x00, 0x00)));
  simtk_widget_set_background (c, (ARGB (0xc0, 0x00, 0x00, 0x00))); */

  // simtk_event_connect (c, SIMTK_EVENT_CREATE, draw_c);
//  simtk_event_connect (b, SIMTK_EVENT_CREATE, draw_b);
  //simtk_event_connect (a, SIMTK_EVENT_CREATE, draw_c);

  simtk_event_connect (window, SIMTK_EVENT_MOUSEMOVE, c_window_onmousemove);
  simtk_event_connect (window, SIMTK_EVENT_MOUSEDOWN, c_window_onmousedown);
  simtk_event_connect (window, SIMTK_EVENT_MOUSEUP,   c_window_onmouseup);

  render_hexdump (c, addr, NULL, 16, 4096);

  b = simtk_bitview_new (simtk_window_get_body_container (vwind), 0, 0, 128, 64 * 16, SIMTK_HORIZONTAL, SIMTK_VERTICAL, addr, 100 * 1024, 512);

  struct simtk_bitview_properties *prop;
  
  simtk_event_connect (vwind, SIMTK_EVENT_MOUSEMOVE, c_window_onmousemove);
  simtk_event_connect (vwind, SIMTK_EVENT_MOUSEDOWN, c_window_onmousedown);
  simtk_event_connect (vwind, SIMTK_EVENT_MOUSEUP,   c_window_onmouseup);

  prop = simtk_bitview_get_properties (b);

  prop->background = OPAQUE (0);
  
  b = simtk_bitview_new (simtk_window_get_body_container (hwind), 0, 0, 512, 64 * 4, SIMTK_HORIZONTAL, SIMTK_HORIZONTAL, addr, 100 * 1024, 512);

  simtk_event_connect (hwind, SIMTK_EVENT_MOUSEMOVE, c_window_onmousemove);
  simtk_event_connect (hwind, SIMTK_EVENT_MOUSEDOWN, c_window_onmousedown);
  simtk_event_connect (hwind, SIMTK_EVENT_MOUSEUP,   c_window_onmouseup);

  prop = simtk_bitview_get_properties (b);

  prop->color_lsb = OPAQUE (0x4f3300);
  prop->color_msb = OPAQUE (0xbf7b00);
  
  prop->background = OPAQUE (0);
  
  simtk_container_set_background (simtk_get_root_container (), "background.bmp");
  simtk_event_loop (simtk_get_root_container ());
  
  return 0;
}

