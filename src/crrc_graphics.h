/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2007, 2008 Jan Reucker (original author)
 * Copyright (C) 2007, 2008 Jens Wilhelm Wulf
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
  

/** \file crrc_graphics.h
 *
 *  The public declarations for crrc_graphics.cpp.
 */

#ifndef CRRC_GRAPHICS_H
#define CRRC_GRAPHICS_H

#include <plib/ssg.h>
#include "include_gl.h"
#include "config.h"
#include "mod_math/CVector.h"


/**
 *  Struct for storing video buffer depth information.
 */
typedef struct
{
  int red;      ///< bpp of red component
  int green;    ///< bpp of green component
  int blue;     ///< bpp of blue component
  int alpha;    ///< bpp of alpha component
  int depth;    ///< bpp of depth buffer
  int stencil;  ///< bpp of stencil buffer
} T_VideoBitDepthInfo;


// --- global variables defined in crrc_graphics.cpp -------

extern T_VideoBitDepthInfo vidbits;

extern ssgRoot *scene;

extern int window_xsize;  // Size of window in x direction
extern int window_ysize;  // Size of window in y direction

extern float flSloppyCam;

extern CVector looking_pos;

// --- functions defined in crrc_graphics.cpp --------------

int video_setup(int nX, int nY, int nFullscreen);
void setWindowTitleString();

unsigned char * read_bwimage(const char *name, int *w, int *h);
unsigned char * read_rgbimage(const char *name,int *w, int *h);
GLuint make_texture(unsigned char *pixel_data, GLint pixel_format, GLint format,
                    GLsizei width, GLsizei height, bool use_mipmaps);

void initialize_scenegraph();
void initialize_window(T_Config *config);
void display();
void reshape(int w, int h);
void resize_window(int w, int h);
void adjust_zoom(float field_of_view);
void graphics_cleanup();
void graphics_read_config(SimpleXMLTransfer* cf);
void drawSolidCube(GLfloat size);
void dumpGLStackInfo(FILE* pFile);

/**
 * calculate looking direction
 */
void graphics_UpdateCamera(float flDeltaT);
  
ssgContext* getGlobalRenderingContext();

#endif
