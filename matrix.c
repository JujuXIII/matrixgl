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
#define glTexEnvf(x,y,z) glTexEnvi(x, y, (GLint)(z + 0.5f))


#include <EGL/egl.h>
#include "matrix.h"  /* Prototypes */
#include "fonts.h"

#define MYGLES

#define screen_height 900
#define screen_width 1440

#if 1
#include "images_90.h"
/* Global Variables */
#define VIEW_ANGLE 45.0f
#define VIEW_DEPTH -89.0f  
#define rtext_x 90
#define text_y 70
/* text_y * screen_width / screen_height */
#define text_x 112
#else
#include "images_80.h"
/* Global Variables */
#define VIEW_ANGLE 45.0f
#define VIEW_DEPTH -68.0f  
#define rtext_x 80
#define text_y 50
/* text_y * screen_width / screen_height */
#define text_x 80
#endif

unsigned char speeds[text_x]; /* Speed of each column (0-2) */

typedef struct {
   unsigned char num;   /* Character number (0-59) */
   unsigned char alpha; /* Alpha modifier */
   unsigned char z;     /* Cached Z coordinate */
} t_glyph;
t_glyph glyphs[sizeof(t_glyph) * (text_x * text_y)];

long timer=40;             /* Controls pic fade in/out */
int pic_fade=0;            /* Makes all chars lighter/darker */
int classic=0;             /* classic mode (no 3d) */
int rain_intensity=2;      /* Intensity of digital rain */

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

#ifdef MYGLES
GLfloat texcoord2f_array[(text_x * text_y) * 2 * 4];
GLint vertex3i_array[(text_x * text_y) * 3 * 4];
GLubyte color4ub_array[(text_x * text_y) * 4 * 4];
static int pos4f=0;
static int pos3f=0;
static int pos2f=0;
static int posnb=0;
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
      glyphs[i].num   = (rand()%50)+10;
      glyphs[i].z     = 0;
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
      glFinish();

      gettimeofday (&tv2, NULL);
      printf("elapsed time: %ld ms\n", ((tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec)/1000));

      usleep(50000);
   } 
   return 0;
}

/* Draw character #num on the screen. */
static void draw_char(int mode, long num, GLubyte light, char x, char y, char z)
{
   /* The font texture is a grid of 10x6 characters. Texture coords are
    * normalized to [0,1] and (s,t) is the top-left texel of the character #num. */
   float s = (float)(num%10) / 10;
   float t = 1 - (float)(num/10)/6;

#ifdef MYGLES
   /*printf("%3d x %3d = %d\n",ix,iy,pos4f);*/
   if(mode==1)
   {
    color4ub_array[pos4f+0] = color4ub_array[pos4f+4] = color4ub_array[pos4f+8]  = color4ub_array[pos4f+12] = 0;
    color4ub_array[pos4f+1] = color4ub_array[pos4f+5] = color4ub_array[pos4f+9]  = color4ub_array[pos4f+13] = 180;
    color4ub_array[pos4f+2] = color4ub_array[pos4f+6] = color4ub_array[pos4f+10] = color4ub_array[pos4f+14] = 0;
    color4ub_array[pos4f+3] = color4ub_array[pos4f+7] = color4ub_array[pos4f+11] = color4ub_array[pos4f+15] = light;
   }
   else
   {
    color4ub_array[pos4f+0] = color4ub_array[pos4f+4] = color4ub_array[pos4f+8]  = color4ub_array[pos4f+12] = 255;
    color4ub_array[pos4f+1] = color4ub_array[pos4f+5] = color4ub_array[pos4f+9]  = color4ub_array[pos4f+13] = 255;
    color4ub_array[pos4f+2] = color4ub_array[pos4f+6] = color4ub_array[pos4f+10] = color4ub_array[pos4f+14] = 255;
    color4ub_array[pos4f+3] = color4ub_array[pos4f+7] = color4ub_array[pos4f+11] = color4ub_array[pos4f+15] = light;
   }
    pos4f+=16;

    texcoord2f_array[pos2f+0] = s;     texcoord2f_array[pos2f+1] = t; 
    texcoord2f_array[pos2f+2] = s+0.1; texcoord2f_array[pos2f+3] = t; 
    texcoord2f_array[pos2f+4] = s+0.1; texcoord2f_array[pos2f+5] = t-0.166; 
    texcoord2f_array[pos2f+6] = s;     texcoord2f_array[pos2f+7] = t-0.166; 
    pos2f+=8;

    vertex3i_array[pos3f+0] = x;   vertex3i_array[pos3f+1]  = y;   vertex3i_array[pos3f+2]  = z;
    vertex3i_array[pos3f+3] = x+1; vertex3i_array[pos3f+4]  = y;   vertex3i_array[pos3f+5]  = z;
    vertex3i_array[pos3f+6] = x+1; vertex3i_array[pos3f+7]  = y-1; vertex3i_array[pos3f+8]  = z;
    vertex3i_array[pos3f+9] = x;   vertex3i_array[pos3f+10] = y-1; vertex3i_array[pos3f+11] = z;
    pos3f+=12;

    posnb++;
#else
   if(mode==1)
      glColor4ub(0,180,0,light);
   else
      glColor4ub(255, 255, 255, light);

   glTexCoord2f(s,     t);       glVertex3i(x,   y,   z);
   glTexCoord2f(s+0.1, t);       glVertex3i(x+1, y,   z);
   glTexCoord2f(s+0.1, t-0.166); glVertex3i(x+1, y-1, z);
   glTexCoord2f(s,     t-0.166); glVertex3i(x,   y-1, z);
#endif
}

/* Draw flare around white characters */
static void draw_flare(char x,char y,char z)
{
#ifdef MYGLES
   /*printf("%3d x %3d = %d\n",ix,iy,pos4f);*/
    color4ub_array[pos4f+0] = color4ub_array[pos4f+4] = color4ub_array[pos4f+8]  = color4ub_array[pos4f+12] = 230;
    color4ub_array[pos4f+1] = color4ub_array[pos4f+5] = color4ub_array[pos4f+9]  = color4ub_array[pos4f+13] = 100;
    color4ub_array[pos4f+2] = color4ub_array[pos4f+6] = color4ub_array[pos4f+10] = color4ub_array[pos4f+14] = 75;
    color4ub_array[pos4f+3] = color4ub_array[pos4f+7] = color4ub_array[pos4f+11] = color4ub_array[pos4f+15] = 190;
    pos4f+=16;

    texcoord2f_array[pos2f+0] = 0;    texcoord2f_array[pos2f+1] = 0; 
    texcoord2f_array[pos2f+2] = 0.75; texcoord2f_array[pos2f+3] = 0; 
    texcoord2f_array[pos2f+4] = 0.75; texcoord2f_array[pos2f+5] = 0.75; 
    texcoord2f_array[pos2f+6] = 0;    texcoord2f_array[pos2f+7] = 0.75; 
    pos2f+=8;

    vertex3i_array[pos3f+0] = x-1; vertex3i_array[pos3f+1]  = y+1; vertex3i_array[pos3f+2]  = z;
    vertex3i_array[pos3f+3] = x+2; vertex3i_array[pos3f+4]  = y+1; vertex3i_array[pos3f+5]  = z;
    vertex3i_array[pos3f+6] = x+2; vertex3i_array[pos3f+7]  = y-2; vertex3i_array[pos3f+8]  = z;
    vertex3i_array[pos3f+9] = x-1; vertex3i_array[pos3f+10] = y-2; vertex3i_array[pos3f+11] = z;
    pos3f+=12;

    posnb++;
#else
   glColor4ub(230,100,75,190);

   glTexCoord2f(0,    0);    glVertex3i(x-1, y+1, z);
   glTexCoord2f(0.75, 0);    glVertex3i(x+2, y+1, z);
   glTexCoord2f(0.75, 0.75); glVertex3i(x+2, y-2, z);
   glTexCoord2f(0,    0.75); glVertex3i(x-1, y-2, z);
#endif
}

/* Draw green or white text on screen */
static void draw_text1(void)
{
   char x, y;
   int i=0, b=0;

   /* For each character, from top-left to bottom-right of screen */
   for (y=text_y/2; y>-text_y/2; y--) {
      for (x=-text_x/2; x<text_x/2; x++, i++) {
         unsigned char light = clamp(glyphs[i].alpha + pic_fade, 0, 255);
         unsigned char depth = 0;

         /* If the coordinate is in the range of the 3D picture, set depth */
         if (x >= -rtext_x/2 && x<rtext_x/2) {
            depth=clamp(pic[b]+(pic_fade-255), 0, 255);
            b++;

            /* Make far-back pixels darker */
            light-=depth; if (light<0) light=0;
         }

         glyphs[i].z = (char)((128-depth/2)-120); /* Map depth (0-128) to coord */

         /* Highlight visible characters directly above a black stream */
         if (y!=text_y/2 && glyphs[i-text_x].alpha && !glyphs[i].alpha) {
            /* White character */
            draw_char(2, glyphs[i].num, 127, x, y, glyphs[i].z);
         }
         else
         {
            /* Green character */
            draw_char(1, glyphs[i].num, light, x, y, glyphs[i].z);
         }
      }
   }
}

/* Draw flares for each column */
static void draw_text2(void)
{
   char x, y;
   int i=0;

   /* For each character from top-left to bottom-right of screen,
    * excluding the bottom-most row. */
   for (y=text_y/2-1; y>-text_y/2; y--) {
      for (x=-text_x/2; x<text_x/2; x++, i++) {
         /* Highlight visible characters directly above a black stream */
         if (glyphs[i].alpha && !glyphs[i+text_x].alpha) {
            draw_flare(x, y, glyphs[i].z);
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

   /* 3D picture transitions */
   if (!classic) {
      timer++;

      if (timer < 250) {
         /* Fading in */
         if ((pic_fade+=3)>255) pic_fade=255;
      } else {
         /* Fading out */
         if ((pic_fade-=3) < 0) pic_fade = 0;
      }

      /* Restart animation */
      if (timer>500) {
         timer=0;
      }
   }
}

static void make_change(void)
{
   int i;

   for (i=0; i<rain_intensity; i++) {
      /* Random character changes */
      int r=rand() % (text_x * text_y);
      glyphs[r].num = (rand()%50)+10;

      /* White nodes (1 in 5 chance of doing anything) */
      r=rand() % (text_x * 5);
      if (r<text_x && glyphs[r].alpha!=0) glyphs[r].alpha=255;
   }
}

static void cbRenderScene(void)
{  

   glMatrixMode(GL_MODELVIEW);
   glTranslatef(0.0f,0.0f,VIEW_DEPTH);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glBindTexture(GL_TEXTURE_2D,1);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);
#ifndef MYGLES
   glBegin(GL_QUADS);
#else
    pos4f = pos3f = pos2f = posnb = 0;
#endif
     draw_text1();
#ifndef MYGLES
   glEnd();
#else
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
    glVertexPointer(3, GL_INT, 0, vertex3i_array);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, color4ub_array);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoord2f_array);

    glDrawArrays(GL_QUADS,0,posnb*4);
 
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
#endif

   glBindTexture(GL_TEXTURE_2D,2);
   glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_REPLACE);
#ifndef MYGLES
   glBegin(GL_QUADS);
#else
    pos4f = pos3f = pos2f = posnb = 0;
#endif
     draw_text2();
#ifndef MYGLES
   glEnd();
#else
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
    glVertexPointer(3, GL_INT, 0, vertex3i_array);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, color4ub_array);
    glTexCoordPointer(2, GL_FLOAT, 0, texcoord2f_array);

    glDrawArrays(GL_QUADS,0,posnb*4);
 
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
#endif

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
   gluPerspective(VIEW_ANGLE,(GLfloat)width/(GLfloat)height,0.1f,200.0f);
   glMatrixMode(GL_MODELVIEW);
}

/*#define MANUAL_MIPMAP*/
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
   /*glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_DECAL);*/

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

