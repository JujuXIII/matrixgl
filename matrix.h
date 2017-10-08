/* matrixgl - Cross-platform matrix screensaver
 *
 * Copyright (C) 2003 Alexander Zolotov, Eugene Zolotov
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014 Vincent Launchbury
 *
 * See AUTHORS for a full list of contributors.
 *
 * -------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  US
 */

#ifndef MATRIX_H
#define MATRIX_H

static void draw_char(int mode, long num, GLubyte light, char x, char y, char z);
static void draw_flare(char x,char y, char z);
static void draw_text1(void);
static void draw_text2(void);
static void scroll(void);
static void make_change(void);
static void cbRenderScene(void);
static void cbKeyPressed(unsigned char key, int x, int y);
static void cbResizeScene(int width, int height);
static void ourInit(void);
static char get_ascii_keycode(XEvent *ev);

#define clamp(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

#endif /* MATRIX_H */

