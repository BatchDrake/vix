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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/io.h>

#include <util.h>
#include "cpi.h"
/* #include "ega9.h" */
#include "pearl-m68k.h"

static inline int 
in_bounds (cpi_handle_t *handle, int sz)
{
  if (sz < 0 || sz >= handle->cpi_file_size)
    return 0;
    
  return 1;
}

int 
cpi_map_codepage (cpi_handle_t *handle, const char *path)
{
  memset (handle, 0, sizeof (cpi_handle_t));
  
  if (path == NULL)
  {
    handle->cpi_fd = -1;
    handle->cpi_file_size = sizeof (bin2c_pearl_m68k_cpi_data);
    handle->cpi_file = bin2c_pearl_m68k_cpi_data;
  }
  else
  {
    if ((handle->cpi_fd = open (path, O_RDONLY)) == -1)
    {
      perror ("cpi_map_codepage: open");
      return -1;
    }
    
    if ((handle->cpi_file_size = lseek (handle->cpi_fd, 0, SEEK_END)) == -1)
    {
      perror ("cpi_map_codepage: lseek");
      close (handle->cpi_fd);
      return -1;
    }
    
    if ((handle->cpi_file =
      mmap (NULL, handle->cpi_file_size, PROT_READ, MAP_PRIVATE, 
      handle->cpi_fd, 0)) == 
      (caddr_t) -1)
    {
      perror ("cpi_map_codepage: mmap");
      close (handle->cpi_fd);
      return -1;
    }
  }
  
  handle->cpi_file_header = (struct cpi_header *) handle->cpi_file;
  
  if (memcmp (handle->cpi_file_header->tag, CPI_TAG, 8) != 0 &&
      memcmp (handle->cpi_file_header->tag, CPI_TAG_NT, 8) != 0)
  {
    ERROR ("cpi_map_codepage: invalid file (not a CPI file)\n");
    munmap (handle->cpi_file, handle->cpi_file_size);
    handle->cpi_file = NULL;
    close (handle->cpi_fd);
    return -1;
  }
  
  handle->font_is_nt = 
    memcmp (handle->cpi_file_header->tag, CPI_TAG_NT, 8) == 0;
  return 0;
}

struct cpi_entry *
cpi_get_page (cpi_handle_t *handle, short cp)
{
  struct cpi_entry *entry;
  int entry_count;
  int i;
  long p;
  
  struct cpi_font_info *info;
  struct cpi_disp_font *font;
  
  
  if (handle->cpi_file == NULL)
  {
    ERROR ("cpi_get_page: no previously CPI file selected\n");
    return NULL;
  }
  
  entry_count = 0;
  
  for (entry = (struct cpi_entry *) (handle->cpi_file + 
    (handle->font_is_nt ? 0x19 : handle->cpi_file_header->info_off + 2));
       entry_count < handle->cpi_file_header->entry_no;
       )
  {
    if (!in_bounds (handle, (long) entry - (long) handle->cpi_file))
    {
      ERROR ("cpi_get_page: entry 0x%lx (%d) out of bounds!\n",
        (long) entry - (long) handle->cpi_file, entry_count);
        
      return NULL;
    }
    
    if (entry->device_type == 1 && entry->codepage == cp)
      return entry;
    
    printf ("%d omissible\n", entry->codepage);
    
    entry_count++;
    
    if (handle->font_is_nt)
    {
      info = (struct cpi_font_info *) 
        ((void *) entry + sizeof (struct cpi_entry));
      p = (long) info + sizeof (struct cpi_font_info);
      
      for (i = 0; i < info->font_no; i++)
      {
        if (!in_bounds (handle, p - (long) handle->cpi_file))
        {
          ERROR ("cpi_get_page: font 0x%lx (%d) out of bounds!\n", 
            p, i);
          return NULL;
        }
        
        font = (struct cpi_disp_font *) p;
        
        p += sizeof (struct cpi_disp_font) + 
          BITS2BYTES (font->chars * font->rows * font->cols);
      }
      
      entry = (struct cpi_entry *) p;
    }
    else
    {
      printf ("Next entry is at 0x%08x\n", entry->next_entry);
      entry = (struct cpi_entry *) (handle->cpi_file + entry->next_entry);
    }
  }
  
  return NULL;
}

struct cpi_disp_font *
cpi_get_disp_font (cpi_handle_t *handle, 
  struct cpi_entry *entry, int rows, int cols)
{
  int i;
  long p;
  struct cpi_font_info *info;
  struct cpi_disp_font *font;
  
  if (!in_bounds (handle, entry->font_info_ptr))
  {
    ERROR ("cpi_get_disp_font: font info out of bounds!\n");
    return NULL;
  }
  
  info = (struct cpi_font_info *) ((handle->font_is_nt ? 
    (void *) entry + sizeof (struct cpi_entry) : 
      handle->cpi_file + entry->font_info_ptr));
  p = (long) info + sizeof (struct cpi_font_info);
  
  for (i = 0; i < info->font_no; i++)
  {
    if (!in_bounds (handle, p - (long) handle->cpi_file))
    {
      ERROR ("cpi_get_disp_font: font 0x%lx (%d) out of bounds!\n", 
        p, i);
      return NULL;
    }
    
    font = (struct cpi_disp_font *) p;
    
    if (font->cols == cols && font->rows == rows)
      return font;
        
    p += sizeof (struct cpi_disp_font) + 
      BITS2BYTES (font->chars * font->rows * font->cols);
  }
  
  return NULL;
}

struct glyph *
cpi_get_glyph (struct cpi_disp_font *font, short glyph)
{
  long relative;
  
  if (glyph < 0 || glyph >= font->chars)
    return NULL;
  
  relative = BITS2BYTES (glyph * font->rows * font->cols);
  relative += (long) font + sizeof (struct cpi_disp_font);
  
  return (struct glyph *) relative;
}


void cpi_puts  (struct cpi_disp_font *font,
                int width, int height,
                int x, int y,
                void *buf,
                int wordsize,
                int transpbg,
                unsigned int fc,
                unsigned int bc,
                const char *text)
{
  int i, j;
  int n;
  int len;
  int charlength;
  int p;
  char *bytebuf;
    
  struct glyph *glyph;
  
  if (font->cols != 8)
  {
    ERROR ("cpi_puts: weird font type (cols != 8)\n");
    return;
  }
  
  bytebuf = buf;
  
  len = strlen (text);
  
  if (font->cols * len + x > width)
    len = (width - x) / font->cols;
  
  charlength = font->rows;
  
  if (charlength + y > height)
    charlength = height - y;
  
  for (n = 0; n < len; n++)
  {
    if ((glyph = cpi_get_glyph (font, (unsigned char) text[n])) == NULL)
      continue;
        
    for (j = 0; j < charlength; j++)
      for (i = 0; i < 8; i++)
        for (p = 0; p < wordsize; p++)
          if (glyph->bits[j] & (1 << (7 - i)))
            bytebuf[p + wordsize * (( (j + y) ) * width + i + x + n * 8)] =
              fc >> (p * 8);
          else if (!transpbg)
            bytebuf[p + wordsize * (( (j + y) ) * width + i + x + n * 8)] =
              bc >> (p * 8);
  }
}

void
cpi_unmap (cpi_handle_t *handle)
{
  /* FIXME: FIX THIS! */
}


 
