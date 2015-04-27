/*
  
  Copyright (C) 2014 Gonzalo José Carracedo Carballal
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/


/*
 * vix.h: headers, prototypes and declarations for vix
 * Creation date: Tue Sep 10 13:50:07 2013
 */

#ifndef _MAIN_INCLUDE_H
#define _MAIN_INCLUDE_H

#include "../config.h" /* General compile-time configuration parameters */
#include <util.h> /* From util: Common utility library */
#include <wbmp.h> /* From sim-static: Built-in simulation library over SDL */
#include <draw.h> /* From sim-static: Built-in simulation library over SDL */
#include <cpi.h> /* From sim-static: Built-in simulation library over SDL */
#include <ega9.h> /* From sim-static: Built-in simulation library over SDL */
#include <hook.h> /* From sim-static: Built-in simulation library over SDL */
#include <layout.h> /* From sim-static: Built-in simulation library over SDL */
#include <pearl-m68k.h> /* From sim-static: Built-in simulation library over SDL */
#include <pixel.h> /* From sim-static: Built-in simulation library over SDL */

#define VIX_SCRIPTS_DIR VIX_DATA_DIR "/scripts"
#define VIX_BG_PATH     VIX_DATA_DIR "/background.bmp"

void vix_scripting_init (void);
int  vix_scripting_directory_init (const char *);

#endif /* _MAIN_INCLUDE_H */
