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


/* Includes */
#define _DEFAULT_SOURCE
#include <unistd.h>

#include <stdio.h>   /* Always a good idea. */

extern void exit(int status);
#define EXIT_FAILURE 1 
#include <string.h>


#include "gl.h"   /* OpenGL SC header */
extern void glTexCoord2f( GLfloat s, GLfloat t );
#define GL_QUADS                0x0007
#define glTexParameterf(x,y,z) glTexParameteri(x, y, (GLint)(z + 0.5f))
#define glPixelTransferf(x,z) glPixelTransferi(x, (GLint)(z + 0.5f))
#define glTexEnvf(x,y,z) glTexEnvi(x, y, (GLint)(z + 0.5f))


#include <EGL/egl.h>
#include "matrix.h"  /* Prototypes */
#include "matrix1.h" /* Font data */

/* Global Variables */
#define screen_height 900
#define screen_width 1440
#define text_y 50 /*70*/
/* text_y * screen_width / screen_height */
#define text_x 80 /*112*/
unsigned char speeds[text_x]; /* Speed of each column (0-2) */

typedef struct {
   unsigned char num;   /* Character number (0-59) */
   unsigned char alpha; /* Alpha modifier */
} t_glyph;
t_glyph glyphs[sizeof(t_glyph) * (text_x * text_y)];

int pic_fade=0;            /* Makes all chars lighter/darker */
int rain_intensity=2;      /* Intensity of digital rain */

#ifndef GLUT
Display                 *dpy;
Window                  root;
XVisualInfo             *vi;
XWindowAttributes       gwa;
XSetWindowAttributes    swa;
Window                  win;
XEvent                  xev;
EGLDisplay  egl_display;
EGLContext  egl_context;
EGLSurface  egl_surface;
#endif


static unsigned int g_seed;

/* Used to seed the generator. */
void srand(int seed) {
    g_seed = seed;
}

/* Compute a pseudorandom integer.
 * Output value in range [0, 32767] */
int rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16)&0x7FFF;
}

int main(int argc,char **argv) 
{
   int i=0;

   EGLConfig  ecfg;
   EGLint     num_config;

   struct timeval tv1, tv2;

   srand(0xACE1u/*time(NULL)*/);

   /* Set up X Window stuff */
   dpy = XOpenDisplay(NULL);
   if(dpy == NULL) {
      fprintf(stderr, "Can't connect to X server\n");
      exit(EXIT_FAILURE);
   }
   swa.event_mask = KeyPressMask;

   /* Create window, and map it */
   win = XCreateWindow(dpy, DefaultRootWindow(dpy),
       0, 0, screen_width, screen_height, 0, 0, InputOutput,
       CopyFromParent, CWEventMask | CWOverrideRedirect, &swa);
   XMapRaised(dpy, win);
   XStoreName(dpy, win, "Matrixgl");

   /* Set up egl */
   eglBindAPI(EGL_OPENGL_API);
   egl_display  =  eglGetDisplay( (EGLNativeDisplayType) dpy ); 
   if ( egl_display == EGL_NO_DISPLAY ) {
      printf("Got no EGL display.\n");
      return 1;
   }
   if ( !eglInitialize( egl_display, NULL, NULL ) ) {
      printf("Unable to initialize EGL\n");
      return 1;
   }
   if ( !eglChooseConfig( egl_display, NULL, &ecfg, 1, &num_config ) ) {
      printf ( "Failed to choose config\n");
      return 1;
   }
   if ( num_config != 1 ) {
      printf("Didn't get exactly one config\n");
      return 1;
   }
   egl_surface = eglCreateWindowSurface ( egl_display, ecfg, win, NULL );
   if ( egl_surface == EGL_NO_SURFACE ) {
      printf("Unable to create EGL surface\n");
      return 1;
   }
   egl_context = eglCreateContext ( egl_display, ecfg, EGL_NO_CONTEXT, NULL );
   if ( egl_context == EGL_NO_CONTEXT ) {
      printf("Unable to create EGL context\n");
      return 1;
   }
   eglMakeCurrent( egl_display, egl_surface, egl_surface, egl_context );

   gettimeofday (&tv1, NULL);

   /* Initializations */
   for (i=0; i<text_x*text_y; i++) {
      glyphs[i].alpha = 253;
      glyphs[i].num   = rand()%60;
   }

   /* Init the light tables */
   for (i=0; i<500;i++) {
      make_change();
      scroll();
   }

   /* Set up column speeds */
   for(i=0;i<text_x;i++) {
      speeds[i]=rand()&1;
      /* If the column on the left is the same speed, go faster */
      if (i && speeds[i]==speeds[i-1]) speeds[i]=2;
   }

   gettimeofday (&tv2, NULL);
   printf("init time: %ld ms\n", ((tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec)/1000));

   gettimeofday (&tv1, NULL);

   ourInit();
   cbResizeScene(screen_width, screen_height);

   gettimeofday (&tv2, NULL);
   printf("gfx init time: %ld ms\n", ((tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec)/1000));

   while(1) {

      /* Check events */
      if (XCheckWindowEvent(dpy, win, KeyPressMask, &xev)) {
         cbKeyPressed(get_ascii_keycode(&xev), 0, 0);
      }

      gettimeofday (&tv1, NULL);
      /* Render frame */
      cbRenderScene();
      glFlush();

      gettimeofday (&tv2, NULL);
      printf("elapsed time: %ld ms\n", ((tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec)/1000));

      usleep(100000);
   } 
   return 0;
}

/* Draw character #num on the screen. */
static void draw_char(int mode, long num, float light, float x, float y)
{
   /* The font texture is a grid of 10x6 characters. Texture coords are
    * normalized to [0,1] and (s,t) is the top-left texel of the character
    * #num. The division by 7 ensures that rows don't evenly line up. */
   float s = (float)(num%10) / 10;
   float t = 1 - (float)(num/10)/7;

   if(mode==1)
      glColor4f(0.0,0.7,0.0,light/255);
   else
      glColor4f(1.0, 1.0, 1.0, light/255);

   glTexCoord2f(s,     t);       glVertex2f(x,   y  );
   glTexCoord2f(s+0.1, t);       glVertex2f(x+1, y  );
   glTexCoord2f(s+0.1, t-0.166); glVertex2f(x+1, y-1);
   glTexCoord2f(s,     t-0.166); glVertex2f(x,   y-1);
}

/* Draw flare around white characters */
static void draw_flare(float x,float y)
{
   glColor4f(0,9.4,0.3,.75);

   glTexCoord2f(0,    0);    glVertex2f(x-1, y+1);
   glTexCoord2f(0.75, 0);    glVertex2f(x+2, y+1);
   glTexCoord2f(0.75, 0.75); glVertex2f(x+2, y-2);
   glTexCoord2f(0,    0.75); glVertex2f(x-1, y-2);
}

/* Draw green or white text on screen */
static void draw_text1(void)
{
   int x, y, i=0;

   /* For each character, from top-left to bottom-right of screen */
   for (y=text_y/2; y>-text_y/2; y--) {
      for (x=-text_x/2; x<text_x/2; x++, i++) {
         /* Highlight visible characters directly above a black stream */
         if (y!=text_y/2 && glyphs[i-text_x].alpha && !glyphs[i].alpha) {
            /* White character */
            draw_char(2, glyphs[i].num, 127.5, x, y);
         }
         else
         {
            /* Green character */
            draw_char(1, glyphs[i].num, glyphs[i].alpha, x, y);
         }
      }
   }
}

/* Draw flares for each column */
static void draw_text2(void)
{
   int x, y, i=0;

   /* For each character from top-left to bottom-right of screen,
    * excluding the bottom-most row. */
   for (y=text_y/2-1; y>-text_y/2; y--) {
      for (x=-text_x/2; x<text_x/2; x++, i++) {
         /* Highlight visible characters directly above a black stream */
         if (glyphs[i].alpha && !glyphs[i+text_x].alpha) {
            draw_flare(x, y);
         }
      }
   }
}

static void scroll(void)
{
   int i, speed, col=0;
   static char odd=0;

   /* Only scroll the slowest columns every second scroll() */
   odd = !odd;

   /* Scroll columns */
   for (speed=odd; speed <= 2; speed++) {
      for (i=text_x*text_y-1; i>=text_x; i--) {
         if (speeds[col] >= speed) glyphs[i].alpha=glyphs[i-text_x].alpha;
         if (++col >=text_x) col=0;
      }
   }

   /* Clear top line in light table */
   for(i=0; i<text_x; i++) glyphs[i].alpha=253;

   /* Make black bugs in top line */
   for(col=0,i=(text_x*text_y)/2; i<(text_x*text_y); i++) {
      if (glyphs[i].alpha==255) glyphs[col].alpha=glyphs[col+text_x].alpha>>1;
      if (++col >=text_x) col=0;
   }
}

static void make_change(void)
{
   int i;
   for (i=0; i<rain_intensity; i++) {
      /* Random character changes */
      int r=rand() % (text_x * text_y);
      glyphs[r].num = rand()%60;

      /* White nodes (1 in 5 chance of doing anything) */
      r=rand() % (text_x * 5);
      if (r<text_x && glyphs[r].alpha!=0) glyphs[r].alpha=255;
   }
}


static void cbRenderScene(void)
{  

   glMatrixMode(GL_MODELVIEW);
   glTranslatef(0.0f,0.0f,-60.0F);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glBindTexture(GL_TEXTURE_2D,1);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glBegin(GL_QUADS); 
     draw_text1();
   glEnd();

   glBindTexture(GL_TEXTURE_2D,2);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glBegin(GL_QUADS);
     draw_text2();
   glEnd();

   make_change();
   scroll();

   glLoadIdentity();
   glMatrixMode(GL_PROJECTION);

   eglSwapBuffers ( egl_display, egl_surface );
}

static void cbKeyPressed(unsigned char key, int x, int y)
{
   switch (key) {
      case 113: case 81: case 27: /* q,ESC */
         exit(0);
   }
}

void gluPerspective(    GLfloat fovy,
     GLfloat aspect,
     GLfloat zNear,
     GLfloat zFar)
{
    /*GLfloat t = tan( (GLfloat)(fovy / 360.0f * 3.14159f) );*/
    GLfloat t = 0.414213f; /* tan calc with fovy = 45 */
    /*GLfloat t = 0.267949f;*/ /* tan calc with fovy = 30 */
    GLfloat fH = t * zNear;
    GLfloat fW = fH * aspect;
    glFrustum( -fW, fW, -fH, fH, zNear, zFar );
}

static void cbResizeScene(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,200.0f);
   glMatrixMode(GL_MODELVIEW);
}

static GLint gluBuild2DMipmaps(    GLenum target,
     GLint internalFormat,
     GLsizei width,
     GLsizei height,
     GLenum format,
     GLenum type,
     void * data)
{

    glTexImage2D(target,0, internalFormat, width, height, 0, format, type, data);

#ifdef AUTO_MIPMAP
    glGenerateMipmap(target);
#elif defined(MANUAL_MIPMAP)
    {
        unsigned char (*data1)[131072]=data;
        unsigned char data2[131072];
        int w,h,d,l;
        GLsizei i1,j1,i2,j2;

        /* generate mimap manually */
        w=width/2;h=height/2;d=2;l=1;
        do
        {
            (w<1)?w=1:w;
            (h<1)?h=1:h;

            for (i1=0,i2=0; i1<width; i1+=d,i2++)
            {
                for (j1=0,j2=0; j1<height; j1+=d,j2++)
                {
                    data2[i2*h+j2]=(*data1)[i1*height+j1];
                }
            }
            glTexImage2D(target,l, internalFormat, w, h, 0, format, type, data2);

            w/=2;h/=2;d*=2;l+=1;
        }
        while(w>0 || h>0);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
#else
    /* no mipmap */
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif

   return 0;
}

static void ourInit(void) 
{
   unsigned char flare[16]={0,0,0,0,0,180,0}; /* Node flare texture */

   /* Create texture mipmaps */
   glBindTexture(GL_TEXTURE_2D,1);
   gluBuild2DMipmaps(GL_TEXTURE_2D,GL_LUMINANCE, 512, 256, GL_LUMINANCE, 
      GL_UNSIGNED_BYTE, (void *)font);

   glBindTexture(GL_TEXTURE_2D,2);
   gluBuild2DMipmaps(GL_TEXTURE_2D,GL_LUMINANCE, 4, 4, GL_LUMINANCE, 
      GL_UNSIGNED_BYTE, (void *)flare);

   /* Use GL_DECAL so texels aren't multipied by the colors on the screen, e.g
    * the black set by calls to glClear(GL_COLOR_BUFFER_BIT) */
   glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);

   glEnable(GL_BLEND);
   glEnable(GL_TEXTURE_2D);
   /*glDisable(GL_LIGHTING);*/
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
}

static char get_ascii_keycode(XEvent *ev)
{
   char key;
   int count = XLookupString((XKeyEvent *)ev, &key, 1, NULL, NULL);

   /* We only care about single-character key presses */
   if (count == 1) {
      return key;
   }

   return 0;
}

