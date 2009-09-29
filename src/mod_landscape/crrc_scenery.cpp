/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 *   Copyright (C) 2000, 2001 Jan Kansky (original author)
 *   Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Jan Reucker
 *   Copyright (C) 2004, 2005, 2006, 2008 Jens Wilhelm Wulf
 *   Copyright (C) 2005 Chris Bayley
 *   Copyright (C) 2005 Lionel Cailler
 *   Copyright (C) 2006 Todd Templeton
 *   Copyright (C) 2009 Joel Lienard
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

/** \file crrc_scenery.cpp
 *  This file defines a "scenery" class which contains all data
 *  and methods to construct and draw the landscape.
 */

//#define DRAW_SQUARES
#define DRAW_TRIANGLES

#include <crrc_config.h>

#include "crrc_scenery.h"
#include "../crrc_main.h"
#include "../ImageLoaderTGA.h"
#include "../mod_misc/SimpleXMLTransfer.h"
#include "../config.h"
#include "../mod_misc/filesystools.h"
#include "../crrc_graphics.h"
#include "../crrc_system.h"
#include <string>
#include <iostream>
using namespace std;

#include "../include_gl.h"
const float FEET2METERS=0.3048;

/**
 *  A list of XML attributes that contain the
 *  skybox filenames, in the correct order for
 *  the call to the SkyBox ctor.
 */
static const char *boxtex_attribs[] =
{
  "north.filename",
  "south.filename",
  "west.filename",
  "east.filename",
  "up.filename",
  "down.filename"
};


/** Load a scenery from a file
 *
 *  Loads the scenery described by the given scenery file.
 *  fname must contain the full path information and point
 *  to an XML scenery description file.
 *
 *  \param fname scenery file (with full path)
 *  \return Pointer to new scenery on success, NULL on error
 */



Scenery* loadScenery(const char *fname)
{
  Scenery* new_scenery = NULL;
  SimpleXMLTransfer* xml = NULL;

  // try to open the specified file
  try
  {
    SimpleXMLTransfer* tag;
    xml = new SimpleXMLTransfer(fname);

    // Take a look at the version number of the config file.
    int nVer = xml->attributeAsInt("version", 1);
    if (nVer < 3)
    {
      throw (nVer);
    }

    // parse the <scene> tag to determine which kind of
    // subclass we need to set up
    tag = xml->getChild("scene");
    std::string type = tag->attribute("type", "not specified");

    if (type == "built-in")
    {
      // create one of the pre-defined sceneries
      std::string variant = tag->attribute("variant", "not specified");

      if (variant == "DAVIS")
      {
        new_scenery = new BuiltinSceneryDavis(xml);
      }
      else if (variant == "CAPE_COD")
      {
        new_scenery = new BuiltinSceneryCapeCod(xml);
      }
      else if (variant == "NULL")
      {
        new_scenery = new BuiltinSceneryNull(xml);
      }
      else // "not specified" or other unknown variant
      {
        fprintf(stderr, "Unknown built-in variant %s in %s\n", variant.c_str(), fname);
        fprintf(stderr, "Defaulting to DAVIS\n");
        new_scenery = new BuiltinSceneryDavis(xml);
      }
    }
    else if (type == "model-based")
    {
      new_scenery = new ModelBasedScenery(xml);
    }
    else // "not specified" or other unknown type
    {
    }
  }
  catch (XMLException e)
  {
    std::string s = "XMLException: ";
    s += e.what();
    fprintf(stderr, "%s\n", s.c_str());
    new_scenery = NULL;
  }
  catch (int v)
  {
    std::string s = "Incompatible Scenery Version : ";
    fprintf(stderr, "%s%d\n", s.c_str(),v);
    new_scenery = NULL;
  }

  if (xml != NULL)
  {
    delete xml;
  }

  return new_scenery;
}



/**
 *  Constructor of the base class
 */
Scenery::Scenery(SimpleXMLTransfer *xml)
    : name("unknown")
{
  theSky = NULL;
  flDefaultWindSpeed = 0.0f;
  flDefaultWindDirection = 0.0f;

  if (xml != NULL)
  {
    SimpleXMLTransfer *tag;

    // generic scenery information
    name = xml->getChild("name", true)->getContentString();
    std::cout << "Loading scenery \"" << name << "\"" << std::endl;

    // player positions (view points)
    tag = xml->getChild("views");
    parsePositions(tag, views);

    // starting positions
    try
    {
      tag = xml->getChild("start");
    }
    catch (XMLException e)
    {
      tag=0;
    }
    if (tag) parsePositions(tag, starts);


    // read default settings
    tag = xml->getChild("default.wind", true);
    flDefaultWindSpeed = tag->attributeAsDouble("velocity", 7.0f);
    flDefaultWindDirection = tag->attributeAsDouble("direction", 270.0f);

    // make a copy of the sky parameters and set up the sky
    sky_params = new SimpleXMLTransfer(xml->getChild("sky", true));
    if (cfgfile->getInt("video.enabled", 1))
      setupSky();
  }
}


/**
 *  Destructor of the base class
 */
Scenery::~Scenery()
{
  delete theSky;
  delete sky_params;
}


/**
 *  Setup the sky from the XML branch stored as "sky_params".
 *
 *  Creates the sky from parameters read from the scenery file
 *  and additional config file parameters. An existing sky
 *  object wil be deleted.
 */
void Scenery::setupSky()
{
  if (theSky != NULL)
  {
    delete theSky;
    theSky = NULL;
  }

  std::string sky_type = sky_params->attribute("type", "original");

  if (sky_type == "original")
  {
    // create a CRRCSkyDome
    std::string texture = sky_params->attribute("texture", "");
    float radius = (float)sky_params->getDouble("radius", 8000.0);
    if (texture != "")
    {
      // find full path for the texture
      texture = FileSysTools::getDataPath(texture);
    }
    theSky = new CRRCSkyDome(texture.c_str(), radius);
  }
  else if (sky_type == "box")
  {
    // create a SkyBox
    char *textures[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
    std::string current_texture;

    float size = (float)sky_params->getDouble("size", 10.0);
    SimpleXMLTransfer *tex = sky_params->getChild("textures", true);

    // get up to six texture names for the box faces
    for (int i = 0; i < 6; i++)
    {
      current_texture = tex->getString(boxtex_attribs[i], "");
      if (current_texture != "")
      {
        textures[i] = new char[current_texture.length() + 1];
        strcpy(textures[i], current_texture.c_str());
      }
    }

    // get the desired texture offset from the main config file
    // (as it is related to the OpenGL implementation rather
    // than the scenery itself)
    float texoffset = cfgfile->getDouble("video.skybox.texture_offset", 0.0);

    theSky = new SkyBox((const char**)textures, size, texoffset);

    for (int i = 0; i < 6; i++)
    {
      delete [] textures[i];
    }
  }
  else if (sky_type == "pano")
  {
    // create a CRRCPanoDome
    std::string texture = sky_params->attribute("texture", "");
    float radius = (float)sky_params->getDouble("radius", 10.0);
    if (texture != "")
    {
      // find full path for the texture
      texture = FileSysTools::getDataPath(texture);
    }
    theSky = new CRRCPanoDome(texture.c_str(), radius);
  }
  else
  {
    // unknown type, create a default CRRCSkyDome
    fprintf(stderr, "Unknown sky type \"%s\" in scenery file, using default.\n", sky_type.c_str());
    theSky = new CRRCSkyDome(FileSysTools::getDataPath("textures/clouds.rgb").c_str(),
                             8000.0);
  }
}


/**
 *  Look at the children of \c tag and read those with
 *  the name "position" into an array of positions.
 *
 *  \param tag node of the XML file containing the "position" children
 *  \param pa  reference to the position array
 *  \param default_on_empty add default 0/0/0 position if no position was
 *         found in the XML file?
 *  \return    number of positions added to the array
 */
int Scenery::parsePositions(SimpleXMLTransfer *tag, T_PosnArray& pa, bool default_on_empty)
{
  int ccount;       // child count
  int pcount = 0;   // position count

  ccount = tag->getChildCount();
  for (int i = 0; i < ccount; i++)
  {
    // parse all children of the <views> tag, accept only <position> tags
    SimpleXMLTransfer *view = tag->getChildAt(i);
    if (view->getName() == "position")
    {
      T_Position pos;
      pos.north = (-1)*view->attributeAsDouble("north", 0.0);
      pos.east  = view->attributeAsDouble("east", 0.0);
      pos.height = view->attributeAsDouble("height", 0.0);
      pos.name  = view->attribute("name", "no_name");
      pa.push_back(pos);
      pcount++;
    }
  }

  if ((pcount == 0) && (default_on_empty))
  {
    T_Position pos;
    pos.north     = 0.0;
    pos.east     = 0.0;
    pos.height     = 0.0;
    pos.name  = "empty_default";
    pa.push_back(pos);
    pcount++;
  }
  return pcount;
}


/**
 *  Draw thw sky
 *
 *  Sky rendering is done by the base class. The sky
 *  is implemented by a separate hierarchy of classes,
 *  but is configured by the scenery file. Therefore
 *  the scenery class maintains the pointer to the
 *  sky object and provides this method that in effect
 *  only calls the sky's render method.
 *
 *  \param campos position of the camera in world coordinates
 *  \param current_time current time in ms (for animation effects)
 */
void Scenery::drawSky(sgVec3 *campos, double current_time)
{
  if (theSky != NULL)
  {
    theSky->update(campos, current_time);

    // The SkyRenderers only implement the preDraw() method:
    theSky->preDraw();
  }
}


/**
 *  Get one of the specified player positions of the
 *  scenery.
 *
 *  \param num viewpoint index
 *  \return viewpoint coordinates
 *
 *  \todo right now, we always return position #0, because
 *        there's no way to select other positions.
 */
CVector Scenery::getPlayerPosition(int num)
{
  CVector pos;
  double x,y,z;
  x = views[0].east;
  y = views[0].height + getHeight(views[0].north,views[0].east); //////////////////////////
  z = views[0].north;
  //std::cout << "**GetPlayerPosition " << x <<" " << y <<" "<< z << endl;
  pos.set(x,y,z);
  return pos;
}
/**
 *  Get the number of start positions of the
 *  scenery.
 */
int Scenery::getNumStartPosition()
{
  int num;
  num = starts.size();
  //std::cout << "**GetNumStartPosition " << num << endl;
  return num;
}
/**
 *  Get one of the specified start positions of the
 *  scenery.
 *
 *  \param num start point index
 *  \return point coordinates
 *
 *  \todo right now, we always return position #0, because
 *        there's no way to select other positions.
 */
CVector Scenery::getStartPosition(int num)
{
  CVector pos;
  double x,y,z;
  if ( starts.size() !=0 )
  {
    x = starts[0].east;
    y = starts[0].height + getHeight(starts[0].north,starts[0].east);
    z = starts[0].north;
  }
  else x=y=z=0;
  //std::cout << "**GetStartPosition " << x <<" " << y <<" "<< z << endl;
  pos.set(x,y,z);
  return pos;
}

/** \brief Initialize one of the default locations.
 *
 *  This constructor initializes the original
 *  locations CAPE_COD and DAVIS.
 */
BuiltinScenery::BuiltinScenery(SimpleXMLTransfer *xml, bool boIsNullRenderer)
    : Scenery(xml), use_textures(1), quadric(NULL)
{
  if (!boIsNullRenderer)
  {
    use_textures = (cfgfile->getInt("video.textures.fUse_textures", 1) && cfgfile->getInt("video.enabled", 1));
    if (cfgfile->getInt("video.enabled", 1))
      quadric = gluNewQuadric();
  }
}


/** \brief Generate terrain from XML description file.
 *
 *  This constructor takes all terrain information from a terrain
 *  description file (which may reference other files).
 *
 *  \todo Make it work properly or delete it.
 */
BuiltinScenery::BuiltinScenery(const char *mapfile)
    : Scenery(NULL), use_textures(1)
{
  int x;
  int z;
  int scale;
  string tgafile;

  SimpleXMLTransfer *conf;

  try
  {
    conf = new SimpleXMLTransfer(mapfile);

    size_x = conf->getDouble("dimension.size.x");
    size_z = conf->getDouble("dimension.size.z");
    cout << "Terrain size is " << size_x << " x " << size_z << " world units." << endl;

    offset_x = conf->getDouble("dimension.offset.x");
    offset_z = conf->getDouble("dimension.offset.z");
    cout << "Terrain origin is located at " << offset_x << " | " << offset_z << endl;
    tgafile = conf->getString("heightmap.filename");

    alt_max = conf->getDouble("dimension.height.max");
    alt_min = conf->getDouble("dimension.height.min");

    player.x = conf->getDouble("player.position.x");
    player.y = conf->getDouble("player.position.y");
    player.z = conf->getDouble("player.position.z");

    model_start.x = conf->getDouble("model.position.x");
    model_start.y = conf->getDouble("model.position.y");
    model_start.z = conf->getDouble("model.position.z");
  }
  catch (XMLException e)
  {
    cerr << "Caught XMLException: " << endl;
    cerr << e.what() << endl;
    cerr << "while reading " << mapfile << endl;
    cerr << "Sorry." << endl;
    crrc_exit (CRRC_EXIT_FAILURE,"");
  }

  // open TGA image containing the height information
  CImageLoaderTGA *tgaImage = new CImageLoaderTGA(tgafile.c_str());
  if (tgaImage->status != TGA_OK)
  {
    cout << "Error loading " << tgafile << endl;
  }
  else
  {
    cout << "Successfully opened "<< tgafile << endl;
  }


  scale = (int)(tgaImage->width / HEIGHTMAP_SIZE_X);


  // generate height map from image
  float vscale = alt_max - alt_min;
  for (x = 0; x < HEIGHTMAP_SIZE_X; x++)
  {
    for (z = 0; z < HEIGHTMAP_SIZE_Z; z++)
    {
      unsigned char raw_data = *(tgaImage->imageData
                                 + z*scale*tgaImage->width
                                 + x*scale);
      if (raw_data == 0)
      {
        // To get a sharp coast line, avoid height = 0. Use a
        // value below sea level instead.
        height[x][z] = alt_min - 2.0;
      }
      else
      {
        height[x][z] = (vscale / 256.0) * (float)raw_data;
      }
    }
  }

  // calculate normals for lighting
  calculate_normals();
  if (cfgfile->getInt("video.enabled", 1))
    compile_display_list();

  cout << "Done." << endl;
  delete(tgaImage);
}

/** \brief Destroy scenery object.
 *
 *
 */
BuiltinScenery::~BuiltinScenery()
{
  if (glIsList(list))
  {
    glDeleteLists(list, 1);
  }

  if (quadric != NULL)
  {
    gluDeleteQuadric(quadric);
  }
}

/** \brief Choose drawing mode.
 *
 *  \param yesno If set to true, use textures for drawing.
 */
void BuiltinScenery::setTextures(bool yesno)
{
  use_textures = yesno;
}

/** \brief Calculate the normals for all vertices in a height map.
 *
 *  This method calculates an array of normals which corresponds to
 *  the height array. The four intersecting square edges at each vertex
 *  are used to calculate four normal vectors per vertex. Adding
 *  all four vectors and normalizing it again leads to a single
 *  mean normal vector.
 */
void BuiltinScenery::calculate_normals()
{
  CVector v1, v2, v3, v4;
  CVector n1, n2, n3, n4;
  int x, z;

  float zoom = size_x / HEIGHTMAP_SIZE_X;

  for (x = 1; x < HEIGHTMAP_SIZE_X - 1; x++)
  {
    for (z = 1; z < HEIGHTMAP_SIZE_Z - 1; z++)
    {
      v1.set(1*zoom, (height[x+1][z] - height[x][z]), 0);
      v2.set(0, (height[x][z-1] - height[x][z]), -1*zoom);
      v3.set(-1*zoom, (height[x-1][z] - height[x][z]), 0);
      v4.set(0, (height[x][z+1] - height[x][z]), 1*zoom);

      n1 = v1 * v2;
      //cout << "n1 " << n1.x << " " << n1.y << " " << n1.z << endl;
      n2 = v2 * v3;
      //cout << "n2 " << n2.x << " " << n2.y << " " << n2.z << endl;
      n3 = v3 * v4;
      //cout << "n3 " << n3.x << " " << n3.y << " " << n3.z << endl;
      n4 = v4 * v1;
      //cout << "n4 " << n4.x << " " << n4.y << " " << n4.z << endl;

      normal[x][z] = n1 + n2 + n3 + n4;
      normal[x][z].normalize();
    }
  }
}

/** \brief Draws normal vectors as lines.
 *
 *  This method is for debugging purposes only. It draws each
 *  normal as a short line.
 */
void BuiltinScenery::draw_normals(float length)
{
  float zoom = size_x / HEIGHTMAP_SIZE_X;
  float xoff = offset_x;
  float zoff = offset_z;
  float xpos, zpos;
  int x, z;

  for (x = 0; x < HEIGHTMAP_SIZE_X; x++)
  {
    for (z = 0; z < HEIGHTMAP_SIZE_Z; z++)
    {
      xpos = x*zoom - xoff;
      zpos = z*zoom - zoff;
      glBegin(GL_LINES);
      glColor3f(1.0, 0.0, 0.0);
      glVertex3f(xpos, height[x][z], zpos);
      glVertex3f( xpos + length * normal[x][z].x,
                  height[x][z] + length * normal[x][z].y,
                  zpos + length * normal[x][z].z);
      //glVertex3f(zoom*x + zoom*height[x][y] + normal[x][y].x, zoom*y + zoom*height[x][y] + normal[x][y].y, zoom*height[x][y] + normal[x][y].z);
      glEnd();
    }
  }

}

/** \brief Compiles the terrain data into a static display list.
 *
 *  For better performance it is not advisable to do any
 *  calculations again and again for each frame. By compiling
 *  all drawing commands into a list all scaling and lookup
 *  calculations only have to be done once.
 */
void BuiltinScenery::compile_display_list()
{
  int x;
  int z;
  float xpos, zpos;

  float zoom = size_x / HEIGHTMAP_SIZE_X;
  float xoff = offset_x;
  float zoff = offset_z;

  GLfloat mat_grass[]={0.458, .657,0.031, 1};
  GLfloat no_mat[]={0.0,0.0,0.0,0.0};
  GLfloat mat_water[]={.502, 0.650,0.792, 1};
  GLfloat no_shininess[]={0.0};

  list = glGenLists(1);
  glNewList(list, GL_COMPILE);
  glPolygonMode(GL_FRONT, GL_LINES);

  // draw ocean
  glMaterialfv(GL_FRONT,GL_AMBIENT,mat_water);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_water);
  glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
  glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
  glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
  glColor3f(0.0f, 0.0f, 1.0f);

  glBegin(GL_QUADS);
  glNormal3f(0.0, 1.0, 0.0);
  glVertex3f(-10000, 1.0, -10000.0);
  glNormal3f(0.0, 1.0, 0.0);
  glVertex3f(10000, 1.0, -10000.0);
  glNormal3f(0.0, 1.0, 0.0);
  glVertex3f(10000.0, 1.0, 10000.0);
  glNormal3f(0.0, 1.0, 0.0);
  glVertex3f(-10000.0, 1.0, 10000.0);
  glEnd();

#ifdef DRAW_TRIANGLES
  // draw scenery
  glBegin(GL_TRIANGLES);
  glMaterialfv(GL_FRONT,GL_AMBIENT,mat_grass);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_grass);
  glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
  glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
  glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
  glColor3f(0.5f, 1.0f, 0.5f);

  for (z = 1; z < (HEIGHTMAP_SIZE_Z-2); z++)
  {
    for (x = 1; x < (HEIGHTMAP_SIZE_X-2); x++)
    {
      //glColor3f(0.5f, 1.0f, 0.5f);
      xpos = x*zoom - xoff;
      zpos = z*zoom - zoff;

      glNormal3f(normal[x][z].x, normal[x][z].y, normal[x][z].z);
      glVertex3f(xpos, height[x][z], zpos);

      xpos = (x+1)*zoom - xoff;
      zpos = z*zoom - zoff;
      glNormal3f(normal[x+1][z].x, normal[x+1][z].y, normal[x+1][z].z);
      glVertex3f(xpos, height[x+1][z], zpos);

      xpos = x*zoom - xoff;
      zpos = (z+1)*zoom - zoff;
      glNormal3f(normal[x][z+1].x, normal[x][z+1].y, normal[x][z+1].z);
      glVertex3f(xpos, height[x][z+1], zpos);
      //glColor3f(0.5f, 1.0f, 0.5f);
      xpos = x*zoom - xoff;
      zpos = (z+1)*zoom - zoff;

      glNormal3f(normal[x][z+1].x, normal[x][z+1].y, normal[x][z+1].z);
      glVertex3f(xpos, height[x][z+1], zpos);

      xpos = (x+1)*zoom - xoff;
      zpos = z*zoom - zoff;
      glNormal3f(normal[x+1][z].x, normal[x+1][z].y, normal[x+1][z].z);
      glVertex3f(xpos, height[x+1][z], zpos);

      xpos = (x+1)*zoom - xoff;
      zpos = (z+1)*zoom - zoff;
      glNormal3f(normal[x+1][z+1].x, normal[x+1][z+1].y, normal[x+1][z+1].z);
      glVertex3f(xpos, height[x+1][z+1], zpos);
      //draw_normal(x, y);
    }
  }
  glEnd();
#endif

#ifdef DRAW_SQUARES
  glBegin(GL_QUADS);
  for (z = 1; z < (HEIGHTMAP_SIZE_Z-2); z++)
  {
    for (x = 1; x < (HEIGHTMAP_SIZE_X-2); x++)
    {
      //glColor3f(0.5f, 1.0f, 0.5f);
      xpos = x*zoom - xoff;
      zpos = z*zoom - zoff;

      if (height[x][z] < 0.0001)
      {
        glMaterialfv(GL_FRONT,GL_AMBIENT,mat_water);
        glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_water);
        glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
        glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
        glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
        glColor3f(0.0f, 0.0f, 1.0f);
      }
      else
      {
        glMaterialfv(GL_FRONT,GL_AMBIENT,mat_grass);
        glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_grass);
        glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
        glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
        glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
        glColor3f(0.5f, 1.0f, 0.5f);
      }

      glVertex3f(xpos, height[x][z], zpos);
      glNormal3f(normal[x][z].x, normal[x][z].y, normal[x][z].z);

      xpos = (x+1)*zoom - xoff;
      zpos = z*zoom - zoff;
      glVertex3f(xpos, height[x+1][z], zpos);
      //glNormal3f(normal[x+1][y].x, normal[x+1][y].y, normal[x+1][y].z);

      xpos = (x+1)*zoom - xoff;
      zpos = (z+1)*zoom - zoff;
      glVertex3f(xpos, height[x+1][z+1], zpos);
      //glNormal3f(normal[x+1][y+1].x, normal[x+1][y+1].y, normal[x+1][y+1].z);

      xpos = x*zoom - xoff;
      zpos = (z+1)*zoom - zoff;
      glVertex3f(xpos, height[x][z+1], zpos);
      //glNormal3f(normal[x][y+1].x, normal[x][y+1].y, normal[x][y+1].z);
    }
  }
  glEnd();
#endif

  glEndList();
}

/// \todo these defines are currently defined here and in crrc_graphics.cpp.
///       In the end these values should come from the scenery file, and
///       the Scenery base class must offer an interface to get it.
#define LIGHT_X (1000.0f / 1.72)
#define LIGHT_Y (8000.0f / 1.72)
#define LIGHT_Z (-1000.0f / 1.72)
void BuiltinScenery::setup_drawing_state()
{
  GLfloat mat_specular[]={1.0,1.0,1.0,1.0};
  GLfloat mat_diffuse[]={0.3,0.3,0.3,1.0};
  GLfloat light_position[]={0.0,LIGHT_X,LIGHT_Y,LIGHT_Z};
  GLfloat white_light[]={1.0,1.0,1.0,1.0};

  glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
  glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,mat_diffuse);
  glLightfv(GL_LIGHT0,GL_POSITION,light_position);
  glLightfv(GL_LIGHT0,GL_DIFFUSE,white_light);
  glLightfv(GL_LIGHT0,GL_SPECULAR,white_light);
  glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_COLOR);
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_BLEND);
  glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,white_light);
  glDisable(GL_COLOR_MATERIAL);

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_NORMALIZE);
  glDisable(GL_CULL_FACE);

  // viewing transformation
  gluLookAt(player_pos->x, player_pos->y, player_pos->z,
            looking_pos.x, looking_pos.y, looking_pos.z,
            0.0, 1.0, 0.0);

}

void BuiltinScenery::restore_drawing_state()
{
  glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
  glDisable(GL_CULL_FACE);
  glDisable(GL_TEXTURE_2D);
}


/****************************************************************************/
/* The built-in DAVIS scenery                                               */
/****************************************************************************/
BuiltinSceneryDavis::BuiltinSceneryDavis(SimpleXMLTransfer *xml)
    : BuiltinScenery(xml), location(Scenery::DAVIS)
{
  std::cout << "Initializing scenery DAVIS from ";
  std::cout << xml->getSourceDescr() << std::endl;
  if (use_textures)
  {
    int use_mipmaps = cfgfile->getInt("video.textures.fUse_mipmaps", 1);
    read_textures(xml);
    groundTexture = make_texture( ground_texture, GL_RGBA, GL_ALPHA,
                                  ground_texture_width,
                                  ground_texture_height, use_mipmaps);
    free(ground_texture);
    grassTexture = make_texture(grass_texture, GL_RGBA, GL_RGBA,
                                grass_texture_width,
                                grass_texture_height, use_mipmaps);
    free(grass_texture);
    grassSideTexture = make_texture(grass_side_texture,
                                    GL_RGBA, GL_RGBA,
                                    grass_side_texture_width,
                                    grass_side_texture_height,
                                    use_mipmaps);
    free(grass_side_texture);
    grassTopTexture = make_texture( grass_top_texture,
                                    GL_RGBA, GL_RGBA,
                                    grass_top_texture_width,
                                    grass_top_texture_height,
                                    use_mipmaps);
    free(grass_top_texture);
    easternViewTexture = make_texture(eastern_view_texture,
                                      GL_RGBA, GL_RGBA,
                                      eastern_view_texture_width,
                                      eastern_view_texture_height,
                                      false);
    free(eastern_view_texture);
    netreesTexture = make_texture(netrees_texture, GL_RGBA, GL_RGBA,
                                  netrees_texture_width,
                                  netrees_texture_height,
                                  false);
    free(netrees_texture);
    dirtTexture = make_texture( dirt_texture, GL_RGBA, GL_RGBA,
                                dirt_texture_width,
                                dirt_texture_height, false);
    free(dirt_texture);
    outhouseTexture = make_texture( outhouse_texture,
                                    GL_RGBA, GL_RGBA,
                                    outhouse_texture_width,
                                    outhouse_texture_height,
                                    false);
    free(outhouse_texture);
    freqTexture = make_texture( freq_texture, GL_RGBA, GL_RGBA,
                                freq_texture_width,
                                freq_texture_height, false);
    free(freq_texture);
    pineTexture = make_texture( pine_texture, GL_RGBA, GL_RGBA,
                                pine_texture_width,
                                pine_texture_height, false);
    free(pine_texture);
    decidTexture = make_texture(decid_texture, GL_RGBA, GL_RGBA,
                                decid_texture_width,
                                decid_texture_height, false);
    free(decid_texture);
  }
}


/**
 *  The destructor
 */
BuiltinSceneryDavis::~BuiltinSceneryDavis()
{
  clear_textures();
}


void BuiltinSceneryDavis::read_textures(SimpleXMLTransfer *xml)
{
  SimpleXMLTransfer* tex;
  int err = 0;

  try
  {
    tex = xml->getChild("scene.textures");

    grass_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("grass.file")).c_str(),
                                &grass_texture_width,
                                &grass_texture_height);
    if (grass_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "grass.file");
      err++;
    }

    grass_side_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("grasss.file")).c_str(),
                                     &grass_side_texture_width,
                                     &grass_side_texture_height);
    if (grass_side_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "grasss.file");
      err++;
    }

    grass_top_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("grasst.file")).c_str(),
                                    &grass_top_texture_width,
                                    &grass_top_texture_height);
    if (grass_top_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "grasst.file");
      err++;
    }

    pine_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("pine.file")).c_str(),
                               &pine_texture_width,
                               &pine_texture_height);
    if (pine_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "pine.file");
      err++;
    }

    decid_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("decid.file")).c_str(),
                                &decid_texture_width,
                                &decid_texture_height);
    if (decid_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "decid.file");
      err++;
    }

    eastern_view_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("eastern.file")).c_str(),
                                       &eastern_view_texture_width,
                                       &eastern_view_texture_height);
    if (eastern_view_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "eastern.file");
      err++;
    }

    netrees_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("netrees.file")).c_str(),
                                  &netrees_texture_width,
                                  &netrees_texture_height);
    if (netrees_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "netrees.file");
      err++;
    }

    dirt_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("dirt.file")).c_str(),
                               &dirt_texture_width,
                               &dirt_texture_height);
    if (dirt_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "dirt.file");
      err++;
    }

    outhouse_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("outhouse.file")).c_str(),
                                   &outhouse_texture_width,
                                   &outhouse_texture_height);
    if (outhouse_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "outhouse.file");
      err++;
    }

    freq_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("freq.file")).c_str(),
                               &freq_texture_width,
                               &freq_texture_height);
    if (freq_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "freq.file");
      err++;
    }
    ground_texture_width  = 256;
    ground_texture_height = 256;
    ground_texture=read_bwimage(FileSysTools::getDataPath(tex->getString("ground.file")).c_str(),
                                &ground_texture_width,
                                &ground_texture_height);
    if (ground_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "ground.file");
      err++;
    }
  }
  catch (XMLException e)
  {
    std::string s = "read_textures(): XMLException: ";
    s += e.what();
    fprintf(stderr, "%s\n", s.c_str());
    crrc_exit(CRRC_EXIT_FAILURE, s.c_str());
  }


  if (err > 0)
  {
    fprintf(stderr, "Error loading textures, aborting\n");
    crrc_exit(CRRC_EXIT_FAILURE,
#ifdef WIN32
              "An error occured while trying to load the scenery\n"
              "textures. See stderr.txt for more information."
#else
              "An error occured while trying to load the scenery\n"
              "textures. See stderr for more information."
#endif
             );
  }

}

/**
 *  Delete all loaded textures
 */
void BuiltinSceneryDavis::clear_textures()
{
  if (glIsTexture(groundTexture))
    glDeleteTextures(1, &groundTexture);
  if (glIsTexture(grassTexture))
    glDeleteTextures(1, &grassTexture);
  if (glIsTexture(grassSideTexture))
    glDeleteTextures(1, &grassSideTexture);
  if (glIsTexture(grassTopTexture))
    glDeleteTextures(1, &grassTopTexture);
  if (glIsTexture(pineTexture))
    glDeleteTextures(1, &pineTexture);
  if (glIsTexture(decidTexture))
    glDeleteTextures(1, &decidTexture);
  if (glIsTexture(easternViewTexture))
    glDeleteTextures(1, &easternViewTexture);
  if (glIsTexture(netreesTexture))
    glDeleteTextures(1, &netreesTexture);
  if (glIsTexture(dirtTexture))
    glDeleteTextures(1, &dirtTexture);
  if (glIsTexture(outhouseTexture))
    glDeleteTextures(1, &outhouseTexture);
  if (glIsTexture(freqTexture))
    glDeleteTextures(1, &freqTexture);
}


/** \brief Get the terrain height at a given location
 *
 *  \param x x-coordinate
 *  \param y y-coordinate
 *  \return height (z-coordinate) at (xy)
 */
float BuiltinSceneryDavis::getHeight(float x, float y)
{
  return -0.1;
}


/** \brief Get the terrain height and plane equation
 *
 *  \param x x-coordinate
 *  \param y y-coordinate
 *  \param tplane terrain plane will be copied to this array
 *  \return height (z-coordinate) at (x|y)
 */
float BuiltinSceneryDavis::getHeightAndPlane(float x_north, float y_east, float tplane[4])
{
  tplane[0] = 0.0;
  tplane[1] = 1.0;
  tplane[2] = 0.0;
  tplane[3] = 0.0;

  return getHeight( x_north,  y_east);
}
/***/
void BuiltinSceneryDavis::getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
    float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity)
{
  float    flWindVel = cfg->wind->getVelocity();
  *x_wind_velocity = -1 * flWindVel * cos(M_PI*cfg->wind->getDirection()/180);
  *y_wind_velocity = -1 * flWindVel * sin(M_PI*cfg->wind->getDirection()/180);
  *z_wind_velocity = 0.;
}


/** \brief Draw the terrain.
 *
 *  This method should be called once per frame.
 */
void BuiltinSceneryDavis::draw(double current_time)
{
  GLfloat no_mat[]={0.0,0.0,0.0,0.0};
  GLfloat mat_ground[]={.878, .859, .745, 1};
  GLfloat mat_grass[]={0.458, .657,0.031, 1};
  //GLfloat dark_grass_color[]={0.474,0.521,0.272,1};
  GLfloat mat_pine_trees[]={0.37, 0.37,0.37, 1};
  GLfloat mat_deciduous_trees[]={0.517, .561,0.325, 1};
  GLfloat mat_tall_grass[]={0.533,0.6,0.427, 1};
  GLfloat mat_side_tall_grass[]={0.633,0.7,0.527, 1};
  GLfloat mat_log[]={0.509,0.380,0.321, 1};
  GLfloat mat_handi_house[]={0.343,0.622,0.747, 1};
  GLfloat mat_handi_house_roof[]={1.0,1.0,1.0, 1};
  GLfloat mat_trashcan[]={0.225,0.366,0.186, 1};
  GLfloat mat_trashcan_ring[]={0.325,0.466,0.286, 1};
  //GLfloat mat_groundhaze[]={1,1,1,1.0};
  //GLfloat mat_runway[]={0.20,0.20,0.20,1.0};
  //GLfloat fogColor[4]={0.6,0.6,0.6,1.0};
  GLfloat no_shininess[]={0.0};

  setup_drawing_state();

  if (!use_textures)
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_ground); // Draw parking lot
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_ground);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,-0.1,-50.0);
    glNormal3f(0,1,0);
    glVertex3f(200.0,-0.1,-50.0);
    glNormal3f(0,1,0);
    glVertex3f(200.0,-0.1,-250.0);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,-0.1,-250.0);
    glEnd();
  }
  else
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_ground); // Draw parking lot
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_ground);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,dirtTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(-1.25,-0.625);
    glVertex3f(-100.0,-0.1,-50.0);
    glNormal3f(0,1,0);
    glTexCoord2f(2.5,-0.625);
    glVertex3f(200.0,-0.1,-50.0);
    glNormal3f(0,1,0);
    glTexCoord2f(2.6,-3.125);
    glVertex3f(200.0,-0.1,-250.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-1.25,-3.125);
    glVertex3f(-100.0,-0.1,-250.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }

  if (!use_textures)
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_grass); // Draw field
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);

    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,-0.1,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(400.0,-0.1,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(275.0,-0.1,-50.0);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,-0.1,-50.0);
    glEnd();
  }
  else
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_grass); // Draw field
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,grassTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(-12.5,125);
    glVertex3f(-100.0,-0.1,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(50,125);
    glVertex3f(400.0,-0.1,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(34.375,-6.25);
    glVertex3f(275.0,-0.1,-50.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-12.5,-6.25);
    glVertex3f(-100.0,-0.1,-50.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
  }

  if (use_textures)
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_pine_trees);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_pine_trees);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,easternViewTexture);
    glBegin(GL_QUADS);
    glNormal3f(-1,0,0);
    glTexCoord2f(0,0);
    glVertex3f(6000.0,-20,-900.0);
    glNormal3f(-1,0,0);
    glTexCoord2f(0.98,0);
    glVertex3f(6000.0,-250,5700.0);
    glNormal3f(-1,0,0);
    glTexCoord2f(0.98,0.98);
    glVertex3f(6000.0,175,5700.0);
    glNormal3f(-1,0,0);
    glTexCoord2f(0,0.98);
    glVertex3f(6000.0,405,-900.0);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0);
    glVertex3f(-6000.0,-60,-4700.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.98,0);
    glVertex3f(-6000.0,-40,1900.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.98,0.98);
    glVertex3f(-6000.0,405,1900.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0.98);
    glVertex3f(-6000.0,385,-4700.0);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0);
    glVertex3f(-6000.0,-40,1900.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.98,0);
    glVertex3f(0.0,-40,-6000.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.98,0.98);
    glVertex3f(0.0,405,-6000.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0.98);
    glVertex3f(-6000.0,385,1900.0);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0);
    glVertex3f(6000.0,-20,-900.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.98,0);
    glVertex3f(0.0,-40,-6000.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.98,0.98);
    glVertex3f(0.0,405,-6000.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0.98);
    glVertex3f(6000.0,405,-900.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);


    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_deciduous_trees);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_deciduous_trees);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,netreesTexture);
    glBegin(GL_QUADS);
    glNormal3f(-1,0,0);
    glTexCoord2f(0.98,0);
    glVertex3f(400.0,-10,0.0);
    glNormal3f(-1,0,0);
    glTexCoord2f(0.0,0);
    glVertex3f(300.0,-10,-200.0);
    glNormal3f(-1,0,0);
    glTexCoord2f(0,0.98);
    glVertex3f(300.0,95,-200.0);
    glNormal3f(-1,0,0);
    glTexCoord2f(0.98,0.98);
    glVertex3f(400.0,95,0.0);
    glEnd();

    glBegin(GL_QUADS);
    glTexCoord2f(0.98,0);
    glVertex3f(-400.0,0,-350.0);
    glTexCoord2f(0.0,0);
    glVertex3f(-250.0,0,-550.0);
    glTexCoord2f(0,0.98);
    glVertex3f(-250.0,85,-550.0);
    glTexCoord2f(0.98,0.98);
    glVertex3f(-400.0,85,-350.0);
    glEnd();


    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }


  if (!use_textures)
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_pine_trees); // Draw pine trees
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_pine_trees);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-700.0,-2,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(-700.0,90,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(550.0,90,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(550.0,-2,1000.0);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(550.0,-2,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(550.0,90,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(1000.0,30,950.0);
    glNormal3f(0,1,0);
    glVertex3f(1000.0,-52,950.0);
    glEnd();
  }
  else
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_pine_trees); // Draw pine trees
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_pine_trees);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,pineTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(-7,0);
    glVertex3f(-700.0,-2,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-7,0.99);
    glVertex3f(-700.0,90,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(5.5,0.99);
    glVertex3f(550.0,90,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(5.5,0);
    glVertex3f(550.0,-2,1000.0);
    glEnd();
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(5.5,0);
    glVertex3f(550.0,-2,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(5.5,0.99);
    glVertex3f(550.0,90,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(10,0.99);
    glVertex3f(1000.0,40,950.0);
    glNormal3f(0,1,0);
    glTexCoord2f(10,0);
    glVertex3f(1000.0,-42,950.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }


  if (!use_textures)
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_deciduous_trees);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_deciduous_trees);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);                                // Draw deciduous trees
    glNormal3f(0,1,0);
    glVertex3f(-200.0,-0.1,50.0);
    glNormal3f(0,1,0);
    glVertex3f(-200.0,40,50.0);
    glNormal3f(0,1,0);
    glVertex3f(-200.0,40,300.0);
    glNormal3f(0,1,0);
    glVertex3f(-200.0,-0.1,300.0);
    glEnd();
  }
  else
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_pine_trees);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_pine_trees);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,decidTexture);
    glBegin(GL_QUADS);                                // Draw deciduous trees
    glNormal3f(0,1,0);
    glTexCoord2f(0.01,0.01);
    glVertex3f(-200.0,-0.1,50.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0.01,0.99);
    glVertex3f(-200.0,40,50.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0.99,0.99);
    glVertex3f(-200.0,40,300.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0.99,0.01);
    glVertex3f(-200.0,-0.1,300.0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }

  if (!use_textures)
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,-0.1,-150.0);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,2,-150.0);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,2,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,-0.1,1000.0);
    glEnd();
  }
  else
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,grassSideTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(131.25,0.99);
    glVertex3f(-100.0,-0.1,-150.0);
    glNormal3f(0,1,0);
    glTexCoord2f(131.25,0);
    glVertex3f(-100.0,2,-150.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0,0);
    glVertex3f(-100.0,2,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0,0.99);
    glVertex3f(-100.0,-0.1,1000.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }


  if (!use_textures)
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_tall_grass);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_tall_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,2,-150.0);
    glNormal3f(0,1,0);
    glVertex3f(-300.0,2,-150.0);
    glNormal3f(0,1,0);
    glVertex3f(-300.0,2,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(-100.0,2,1000.0);
    glEnd();
  }
  else
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_tall_grass);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_tall_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,grassTopTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(-10,-5);
    glVertex3f(-100.0,2,-150.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-30,-5);
    glVertex3f(-300.0,2,-150.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-30,100);
    glVertex3f(-300.0,2,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-10,100);
    glVertex3f(-100.0,2,1000.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }


  if (use_textures)                     // Draw bushes around parking lot
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_pine_trees);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_pine_trees);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,decidTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,0,1);
    glTexCoord2f(0.01,0.01);
    glVertex3f(-105.0,-0.2,-250.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.99,0.01);
    glVertex3f(-27.5,-0.2,-250.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.99,0.99);
    glVertex3f(-27.5,7,-250.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.01,0.99);
    glVertex3f(-105,7,-250.0);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(0,0,1);
    glTexCoord2f(0.01,0.01);
    glVertex3f(0.0,-0.2,-250.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.99,0.01);
    glVertex3f(215,-0.2,-250.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.99,0.99);
    glVertex3f(215,10,-250.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.01,0.99);
    glVertex3f(0,10,-250.0);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(1,0,0);
    glTexCoord2f(0.01,0.01);
    glVertex3f(-99.9,-0.2,-145.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.99,0.01);
    glVertex3f(-99.9,-0.2,-250.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.99,0.99);
    glVertex3f(-99.9,5,-250.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.01,0.99);
    glVertex3f(-99.9,5,-145.0);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(0,0,1);
    glTexCoord2f(0.01,0.01);
    glVertex3f(200.0,-0.2,-250.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.99,0.01);
    glVertex3f(200,-0.2,-100.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.99,0.99);
    glVertex3f(200,6,-100.0);
    glNormal3f(0,0,1);
    glTexCoord2f(0.01,0.99);
    glVertex3f(200,10,-250.0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }

  if (!use_textures)
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(275.0,-0.1,-50.0);
    glNormal3f(0,1,0);
    glVertex3f(400.0,-0.1,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(400.0,3,1000.0);
    glNormal3f(0,1,0);
    glVertex3f(275.0,3,-50.0);
    glEnd();
  }
  else
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,grassSideTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(0,0.99);
    glVertex3f(275.0,-0.1,-50.0);
    glNormal3f(0,1,0);
    glTexCoord2f(88,0.99);
    glVertex3f(400.0,-0.1,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(88,0);
    glVertex3f(400.0,3,1000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0,0);
    glVertex3f(275.0,3,-50.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }

  glMaterialfv(GL_FRONT,GL_AMBIENT,mat_log);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_log);
  glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
  glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
  glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
  glPushMatrix();
  glTranslatef(140,0.5,-50.5);
  glRotatef(94,0,1,0);
  gluCylinder(quadric,0.5,0.55,35,5,2);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(97,0.5,-50.5);
  glRotatef(88,0,1,0);
  gluCylinder(quadric,0.5,0.55,35,5,2);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(60,0.5,-50.5);
  glRotatef(90,0,1,0);
  gluCylinder(quadric,0.5,0.55,35,5,2);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(24,0.5,-50.5);
  glRotatef(92,0,1,0);
  gluCylinder(quadric,0.5,0.55,35,5,2);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(-12,0.5,-50.5);
  glRotatef(88,0,1,0);
  gluCylinder(quadric,0.5,0.55,35,5,2);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(-48,0.5,-50.5);
  glRotatef(91,0,1,0);
  gluCylinder(quadric,0.5,0.55,35,5,2);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(-84,0.5,-50.5);
  glRotatef(87,0,1,0);
  gluCylinder(quadric,0.5,0.55,35,5,2);
  glPopMatrix();

  if (use_textures)
  {
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_handi_house);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_handi_house);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,outhouseTexture);
    glBegin(GL_QUADS);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0);
    glVertex3f(33,0,-38);
    glNormal3f(1,0,0);
    glTexCoord2f(1,0);
    glVertex3f(33,0,-34);
    glNormal3f(1,0,0);
    glTexCoord2f(1,1);
    glVertex3f(33,7.5,-34);
    glNormal3f(1,0,0);
    glTexCoord2f(0,1);
    glVertex3f(33,7.5,-38);
    glNormal3f(0,0,1);
    glTexCoord2f(0,0);
    glVertex3f(33,0,-34);
    glNormal3f(0,0,1);
    glTexCoord2f(1,0);
    glVertex3f(37,0,-34);
    glNormal3f(0,0,1);
    glTexCoord2f(1,1);
    glVertex3f(37,7.5,-34);
    glNormal3f(0,0,1);
    glTexCoord2f(0,1);
    glVertex3f(33,7.5,-34);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_handi_house_roof);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_handi_house_roof);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glPushMatrix();
    glTranslatef(35,7.75,-36);
    glScalef(4.25,0.5,4.25);
    drawSolidCube(1);
    glPopMatrix();
  }
  else
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_handi_house);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_handi_house);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glPushMatrix();
    glTranslatef(35,3.75,-36);
    glScalef(1.33,2.5,1.33);
    drawSolidCube(3);
    glPopMatrix();
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_handi_house_roof);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_handi_house_roof);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glPushMatrix();
    glTranslatef(35,7.75,-36);
    glScalef(4.25,0.5,4.25);
    drawSolidCube(1);
    glPopMatrix();
  }

  glMaterialfv(GL_FRONT,GL_AMBIENT,mat_trashcan);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_trashcan);
  glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
  glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
  glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
  glPushMatrix();
  glTranslatef(23,3,-40);
  glRotatef(90,1,0,0);
  gluCylinder(quadric,1.25,1.25,3,10,2);
  glPopMatrix();

  glMaterialfv(GL_FRONT,GL_AMBIENT,mat_trashcan_ring);
  glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_trashcan_ring);
  glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
  glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
  glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
  glPushMatrix();
  glTranslatef(23,1,-40);
  glRotatef(90,1,0,0);
  gluCylinder(quadric,1.3,1.3,0.1,10,2);
  glPopMatrix();
  glPushMatrix();
  glTranslatef(23,2,-40);
  glRotatef(90,1,0,0);
  gluCylinder(quadric,1.3,1.3,0.1,10,2);
  glPopMatrix();


  if (use_textures)   // Draw freq
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_log);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_log);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glPushMatrix();
    glTranslatef(17,1,-33.25);
    glScalef(0.25,2,0.25);
    drawSolidCube(1);
    glPopMatrix();
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_side_tall_grass);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0,0,0,1.0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,freqTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(0,0);
    glVertex3f(15.0,1,-33.0);
    glNormal3f(0,1,0);
    glTexCoord2f(1,0);
    glVertex3f(18.5,1,-33.0);
    glNormal3f(0,1,0);
    glTexCoord2f(1,1);
    glVertex3f(18.5,6,-33.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0,1);
    glVertex3f(15.0,6,-33.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }

  restore_drawing_state();
} // end BuiltinSceneryDavis::draw()


/****************************************************************************/
/* The built-in CAPE_COD scenery                                            */
/****************************************************************************/
BuiltinSceneryCapeCod::BuiltinSceneryCapeCod(SimpleXMLTransfer *xml)
    : BuiltinScenery(xml), location(Scenery::CAPE_COD)
{
  std::cout << "Initializing scenery CAPE_COD from ";
  std::cout << xml->getSourceDescr() << std::endl;
  if (use_textures)
  {
    int use_mipmaps = cfgfile->getInt("video.textures.fUse_mipmaps", 1);
    read_textures(xml);
    waterTexture = make_texture(water_texture, GL_RGBA, GL_RGBA,
                                water_texture_width,
                                water_texture_height, use_mipmaps);
    free(water_texture);
    beachsandTexture = make_texture(beachsand_texture,
                                    GL_RGBA, GL_RGBA,
                                    beachsand_texture_width,
                                    beachsand_texture_height,
                                    use_mipmaps);
    free(beachsand_texture);
    scrubTexture = make_texture(scrub_texture, GL_RGBA, GL_RGBA,
                                scrub_texture_width,
                                scrub_texture_height, use_mipmaps);
    free(scrub_texture);
    scrubedgeTexture = make_texture(scrubedge_texture, GL_RGBA, GL_RGBA,
                                    scrubedge_texture_width,
                                    scrubedge_texture_height, use_mipmaps);
    free(scrubedge_texture);
    southTexture = make_texture(south_texture, GL_RGBA, GL_RGBA,
                                south_texture_width,
                                south_texture_height, use_mipmaps);
    free(south_texture);
    hilledgeTexture = make_texture( hilledge_texture,
                                    GL_RGBA, GL_RGBA,
                                    hilledge_texture_width,
                                    hilledge_texture_height,
                                    use_mipmaps);
    free(hilledge_texture);
    wavesTexture = make_texture(waves_texture, GL_RGBA, GL_RGBA,
                                waves_texture_width,
                                waves_texture_height, use_mipmaps);
    free(waves_texture);
    sandTexture = make_texture( sand_texture, GL_RGBA, GL_RGBA,
                                sand_texture_width,
                                sand_texture_height, use_mipmaps);
    free(sand_texture);
  }
}
/***************************/
void BuiltinSceneryCapeCod::getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
    float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity)
{

  float    flWindVel = cfg->wind->getVelocity();
  *x_wind_velocity = -1 * flWindVel * cos(M_PI*cfg->wind->getDirection()/180);
  *y_wind_velocity = -1 * flWindVel * sin(M_PI*cfg->wind->getDirection()/180);
  *z_wind_velocity = 0.;

  if (cfg->getDynamicSoaring()==FALSE)
  {
    // windfield for cape cod, sloping
    if ((Y_cg > -140) && (Y_cg < 40))
    {
      double flAdd = 0.4 * flWindVel * pow(sin((40-Y_cg)*M_PI/180),2);

      if (-Z_cg<100)
      {
      }
      else if ((-Z_cg >= 100) && (-Z_cg < 250))
      {
        flAdd *= 1.0 - ((-Z_cg-100)/150) ;
      }

      *z_wind_velocity -= flAdd;
    }
  }
  else
  {
    // windfield for cape cod, dynamic soaring
    if ((Y_cg < 0) && (-Z_cg < 100))
    {
      if (-Z_cg >= 86)
      {
        *y_wind_velocity *= (-Z_cg-93)/7;
      }
      else if (-Z_cg < 86)
      {
        *y_wind_velocity *= -1;
      }
    }
  }
}


/**
 *  The destructor
 */
BuiltinSceneryCapeCod::~BuiltinSceneryCapeCod()
{
  clear_textures();
}


/** \brief Get the terrain height at a given location
 *
 *  \param x x-coordinate
 *  \param y y-coordinate
 *  \return height (z-coordinate) at (x|y)
 */
float BuiltinSceneryCapeCod::getHeight(float x_north, float y_east)
{
  if ( y_east > 0)
  {
    return(100.1);
  }
  else if ((y_east < -100) && (y_east > -150))
  {
    return(0.);
  }
  else if (y_east < -150)
  {
    return(-2);
  }
  else //
  {
    return(100.1 + y_east);
  }
}


/** \brief Get the terrain height and plane equation
 *
 *  \param x x-coordinate
 *  \param y y-coordinate
 *  \param tplane terrain plane will be copied to this array
 *  \return height (z-coordinate) at (x|y)
 */
float BuiltinSceneryCapeCod::getHeightAndPlane(float x_north, float y_east, float tplane[4])
{
  if ( y_east > 0)
  {
    // top
    tplane[0] =    0.0;
    tplane[1] =    1.0;
    tplane[2] =    0.0;
    tplane[3] = -100.1;
  }
  else if ((y_east < -100) && (y_east > -150))
  {
    // beach
    tplane[0] =   0.0;
    tplane[1] =   1.0;
    tplane[2] =   0.0;
    tplane[3] =   0.0;
  }
  else if (y_east < -150)
  {
    // ocean
    tplane[0] =   0.0;
    tplane[1] =   1.0;
    tplane[2] =   0.0;
    tplane[3] =  -2.0;
  }
  else
  {
    // slope
    tplane[0] =   -1.0;
    tplane[1] =    1.0;
    tplane[2] =    0.0;
    tplane[3] = -100.1;
  }
  return getHeight(x_north, y_east);
}


/** \brief Draw the terrain.
 *
 *  This method should be called once per frame.
 */
void BuiltinSceneryCapeCod::draw(double current_time)
{
  static GLfloat no_mat[]={0.0,0.0,0.0,0.0};
  static GLfloat mat_water[]={.502, 0.650,0.792, 1};
  static GLfloat mat_sand[]={.737, 0.694,0.576, 1};
  static GLfloat mat_dirtysand[]={.537, 0.494,0.376, 1};
  static GLfloat mat_scrub[]={.325, 0.349,0.239, 1};
  static GLfloat mat_waves[]={.925, 0.925,0.925, 1};
  static GLfloat no_shininess[]={0.0};
  float shift;

  setup_drawing_state();

  if (!use_textures)
  {
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_water); // Draw ocean
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_water);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-10000.0,-0.1,150.0);
    glNormal3f(0,1,0);
    glVertex3f(10000.0,-0.1,150.0);
    glNormal3f(0,1,0);
    glVertex3f(10000.0,-0.1,10000.0);
    glNormal3f(0,1,0);
    glVertex3f(-10000.0,-0.1,10000.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_dirtysand); // Draw beach
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_dirtysand);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-10000.0,-0.1,100.0);
    glNormal3f(0,1,0);
    glVertex3f(10000.0,-0.1,100.0);
    glNormal3f(0,1,0);
    glVertex3f(10000.0,-0.1,150.0);
    glNormal3f(0,1,0);
    glVertex3f(-10000.0,-0.1,150.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_sand); // Draw hill face
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_sand);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-2000.0,100,0.0);
    glNormal3f(0,1,0);
    glVertex3f(2000.0,100,0.0);
    glNormal3f(0,1,0);
    glVertex3f(2000.0,-0.1,100.0);
    glNormal3f(0,1,0);
    glVertex3f(-2000.0,-0.1,100.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_scrub); // Draw hill top
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_scrub);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-2000.0,100,-10000.0);
    glNormal3f(0,1,0);
    glVertex3f(2000.0,100,-10000.0);
    glNormal3f(0,1,0);
    glVertex3f(2000.0,100,0.0);
    glNormal3f(0,1,0);
    glVertex3f(-2000.0,100,0.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_waves); // Draw beach
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_waves);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glVertex3f(-10000.0,3.5,145.0);
    glNormal3f(0,1,0);
    glVertex3f(10000.0,3.5,145.0);
    glNormal3f(0,1,0);
    glVertex3f(10000.0,3.5,200.0);
    glNormal3f(0,1,0);
    glVertex3f(-10000.0,3.5,200.0);
    glEnd();
  }
  else
  {
    shift = -1*(fmod(current_time/500,(double)water_texture_height))/water_texture_height;

    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_water); // Draw parking lot
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_water);
    glMaterialfv(GL_FRONT,GL_SPECULAR,no_mat);
    glMaterialfv(GL_FRONT,GL_SHININESS,no_shininess);
    glMaterialfv(GL_FRONT,GL_EMISSION,no_mat);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,waterTexture);

    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(-40,-shift);
    glVertex3f(-150.0,-0.1,-10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(40,-shift);
    glVertex3f(-150.0,-0.1,10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(40,40-shift);
    glVertex3f(-10000.0,-0.1,10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-40,40-shift);
    glVertex3f(-10000.0,-0.1,-10000.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_sand); // Draw beach lot
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_sand);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,beachsandTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(0,0);
    glVertex3f(-100.0,-0.1,0.0);
    glNormal3f(0,1,0);
    glTexCoord2f(40,0);
    glVertex3f(-100.0,-0.1,10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(40,1);
    glVertex3f(-150.0,-0.1,10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0,1);
    glVertex3f(-150.0,-0.1,0.0);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(0,0);
    glVertex3f(-100.0,-0.1,0.0);
    glNormal3f(0,1,0);
    glTexCoord2f(40,0);
    glVertex3f(-100.0,-0.1,-10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(40,1);
    glVertex3f(-150.0,-0.1,-10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0,1);
    glVertex3f(-150.0,-0.1,0.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_sand); // Souther horizon
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_sand);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,southTexture);
    glBegin(GL_QUADS);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0);
    glVertex3f(430.0,-10.1,2500.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.99,0);
    glVertex3f(-550.0,-20.1,2500.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.99,0.99);
    glVertex3f(-550.0,100.1,2500.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0.99);
    glVertex3f(430.0,117.1,2500.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_sand); // Souther horizon
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_sand);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,southTexture);
    glBegin(GL_QUADS);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0);
    glVertex3f(430.0,-10.1,-2500.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.99,0);
    glVertex3f(-550.0,-20.1,-2500.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0.99,0.99);
    glVertex3f(-550.0,100.1,-2500.0);
    glNormal3f(1,0,0);
    glTexCoord2f(0,0.99);
    glVertex3f(430.0,117.1,-2500.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_waves); // Draw beach lot
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_waves);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,wavesTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(0,0+-5*shift);
    glVertex3f(-144.0,3.5,-10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(20,0+-5*shift);
    glVertex3f(-144.0,3.5,10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(20,1+-5*shift);
    glVertex3f(-200.0,3.5,10000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(0,1+-5*shift);
    glVertex3f(-200.0,3.5,-10000.0);
    glEnd();


    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_sand); // Hillside
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_sand);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,sandTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,0.707,0.707);
    glTexCoord2f(-128,0);
    glVertex3f(-1.0,98.8,-2000.0);
    glNormal3f(0,0.707,0.707);
    glTexCoord2f(128,0);
    glVertex3f(-1.0,98.8,2000.0);
    glNormal3f(0,0.707,0.707);
    glTexCoord2f(128,30);
    glVertex3f(-100.0,-0.1,2000.0);
    glNormal3f(0,0.707,0.707);
    glTexCoord2f(-128,30);
    glVertex3f(-100.0,-0.1,-2000.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_scrub); // Draw hill top
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_scrub);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,scrubTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(-150,-150);
    glVertex3f(2000.0,100,-2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(150,-150);
    glVertex3f(2000.0,100,2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(150,0);
    glVertex3f(0.0,100,2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-150,0);
    glVertex3f(0.0,100,-2000.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_scrub); // Draw hill top
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_scrub);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,scrubedgeTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(-150,0.98);
    glVertex3f(4.0,100.1,-2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(150,0.98);
    glVertex3f(4.0,100.1,2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(150,0.02);
    glVertex3f(0.0,100.1,2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-150,0.02);
    glVertex3f(0.0,100.1,-2000.0);
    glEnd();

    glMaterialfv(GL_FRONT,GL_AMBIENT,mat_scrub); // Draw hill top
    glMaterialfv(GL_FRONT,GL_DIFFUSE,mat_scrub);
    glColor4f(0,0,0,1.0);
    glBindTexture(GL_TEXTURE_2D,hilledgeTexture);
    glBegin(GL_QUADS);
    glNormal3f(0,1,0);
    glTexCoord2f(-128,1);
    glVertex3f(0.0,100,-2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(128,1);
    glVertex3f(0.0,100,2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(128,0);
    glVertex3f(-2.0,98.,2000.0);
    glNormal3f(0,1,0);
    glTexCoord2f(-128,0);
    glVertex3f(-2.0,98.,-2000.0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
  }

  restore_drawing_state();
} // end draw()


void BuiltinSceneryCapeCod::read_textures(SimpleXMLTransfer *xml)
{
  SimpleXMLTransfer* tex;
  int err = 0;

  try
  {
    tex = xml->getChild("scene.textures");

    water_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("water.file")).c_str(),
                                &water_texture_width,
                                &water_texture_height);
    if (water_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "water.file");
      err++;
    }

    beachsand_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("beachsand.file")).c_str(),
                                    &beachsand_texture_width,
                                    &beachsand_texture_height);
    if (beachsand_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "beachsand.file");
      err++;
    }

    scrub_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("scrub.file")).c_str(),
                                &scrub_texture_width,
                                &scrub_texture_height);
    if (scrub_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "scrub.file");
      err++;
    }

    scrubedge_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("scrubedge.file")).c_str(),
                                    &scrubedge_texture_width,
                                    &scrubedge_texture_height);
    if (scrubedge_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "scrubedge.file");
      err++;
    }

    south_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("south.file")).c_str(),
                                &south_texture_width,
                                &south_texture_height);
    if (south_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "south.file");
      err++;
    }

    hilledge_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("hilledge.file")).c_str(),
                                   &hilledge_texture_width,
                                   &hilledge_texture_height);
    if (hilledge_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "hilledge.file");
      err++;
    }

    waves_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("waves.file")).c_str(),
                                &waves_texture_width,
                                &waves_texture_height);
    if (waves_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "waves.file");
      err++;
    }

    sand_texture=read_rgbimage(FileSysTools::getDataPath(tex->getString("sand.file")).c_str(),
                               &sand_texture_width,
                               &sand_texture_height);
    if (sand_texture == NULL)
    {
      fprintf(stderr, "Can't open %s\n", "sand.file");
      err++;
    }
  }
  catch (XMLException e)
  {
    std::string s = "read_textures(): XMLException: ";
    s += e.what();
    fprintf(stderr, "%s\n", s.c_str());
    crrc_exit(CRRC_EXIT_FAILURE, s.c_str());
  }


  if (err > 0)
  {
    fprintf(stderr, "Error loading textures, aborting\n");
    crrc_exit(CRRC_EXIT_FAILURE,
#ifdef WIN32
              "An error occured while trying to load the scenery\n"
              "textures. See stderr.txt for more information."
#else
              "An error occured while trying to load the scenery\n"
              "textures. See stderr for more information."
#endif
             );
  }

}


/**
 *  Delete all loaded textures
 */
void BuiltinSceneryCapeCod::clear_textures()
{
  if (glIsTexture(waterTexture))
    glDeleteTextures(1, &waterTexture);
  if (glIsTexture(beachsandTexture))
    glDeleteTextures(1, &beachsandTexture);
  if (glIsTexture(scrubTexture))
    glDeleteTextures(1, &scrubTexture);
  if (glIsTexture(scrubedgeTexture))
    glDeleteTextures(1, &scrubedgeTexture);
  if (glIsTexture(southTexture))
    glDeleteTextures(1, &southTexture);
  if (glIsTexture(hilledgeTexture))
    glDeleteTextures(1, &hilledgeTexture);
  if (glIsTexture(wavesTexture))
    glDeleteTextures(1, &wavesTexture);
  if (glIsTexture(sandTexture))
    glDeleteTextures(1, &sandTexture);
}


/****************************************************************************/
/* The built-in NULL_RENDERER                                               */
/****************************************************************************/
BuiltinSceneryNull::BuiltinSceneryNull(SimpleXMLTransfer *xml)
    : BuiltinScenery(xml), location(Scenery::NULL_RENDERER)
{
  height = (float)(xml->getChild("scene.settings.height", true)->getDouble("value", 0.0));
}

float BuiltinSceneryNull::getHeight(float x_north, float y_east)
{
  return height;
}

float BuiltinSceneryNull::getHeightAndPlane(float x_north, float y_east, float tplane[4])
{
  float h = getHeight(x_north, y_east);
  tplane[0] = 0.0;
  tplane[1] = 1.0;
  tplane[2] = 0.0;
  tplane[3] = -h;

  return h;
}
void BuiltinSceneryNull::getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
    float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity)
{
  float    flWindVel = cfg->wind->getVelocity();
  *x_wind_velocity = -1 * flWindVel * cos(M_PI*cfg->wind->getDirection()/180);
  *y_wind_velocity = -1 * flWindVel * sin(M_PI*cfg->wind->getDirection()/180);
  *z_wind_velocity = 0.;
}

void BuiltinSceneryNull::draw(double current_time)
{
  // this is a NULL_RENDERER... nothing to do... maybe
  // only keep the compiler happy...
  current_time = 0.0;

  // All the other built-in sceneries do this GL state switching,
  // so we should mimic this behaviour here
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

BuiltinSceneryNull::~BuiltinSceneryNull()
{
}


/****************************************************************************/
/* Model based scenery                                                      */
/****************************************************************************/
ModelBasedScenery::ModelBasedScenery(SimpleXMLTransfer *xml)
    : Scenery(xml), location(Scenery::MODEL_BASED)
{
  ssgEntity *model = NULL;
  SimpleXMLTransfer *scene = xml->getChild("scene", true);
  getHeight_mode = scene->attributeAsInt("getHeight_mode", 2);
  //std::cout << "----getHeight_mode : " <<  getHeight_mode <<std::endl;
  if ( getHeight_mode==2)
  {
    for ( int i = 0 ; i <= SIZE_GRID_PLANES; i++ )
      for ( int j = 0 ; j <= SIZE_GRID_PLANES; j++ )
      {
        tile_table[i][j] = new ssgVertexArray();
      }
  }
  SceneGraph = new ssgRoot();

  // transform everything from SSG coordinates to CRRCsim coordinates
  initial_trans = new ssgTransform();
  SceneGraph->addKid(initial_trans);
//10.76
  sgMat4 it = {  {1,  0.0,  0.0,   0},
    {0.0,  0.0, -1,   0},
    {0.0,  1,  0.0,   0},
    {0.0,  0.0,  0.0, 1.0}
  };

  initial_trans->setTransform(it);

  // find all "objects" defined in the file
  int num_children = scene->getChildCount();

  for (int cur_child = 0; cur_child < num_children; cur_child++)
  {
    SimpleXMLTransfer *kid = scene->getChildAt(cur_child);
    // only use "object" tags
    if (kid->getName() == "object")
    {
      std::string filename = kid->attribute("filename", "not_specified");
      bool is_terrain = (kid->attributeAsInt("terrain", 0) != 0);

      // PLIB automatically loads the texture file,
      // but it does not know which directory to use.
      // Where is the object file?
      std::string    of  = FileSysTools::getDataPath("objects/" + filename);
      // compile and set relative texture path
      std::string    tp  = of.substr(0, of.length()-filename.length()-1-7) + "textures";
      ssgTexturePath(tp.c_str());

      // load model
      std::cout << "Loading 3D object \"" << of.c_str() << "\"";
      if (is_terrain)
      {
        std::cout << " (part of terrain)";
      }
      std::cout << std::endl;
      model = ssgLoad(of.c_str());
      if (model != NULL)
      {
        // now parse the instances and place the model in the SceneGraph
        for (int cur_instance = 0; cur_instance < kid->getChildCount(); cur_instance++)
        {
          SimpleXMLTransfer *instance = kid->getChildAt(cur_instance);
          if (instance->getName() == "instance")
          {
            sgCoord coord;
            coord.xyz[SG_X] = instance->attributeAsDouble("x", 0.0);
            coord.xyz[SG_Y] = instance->attributeAsDouble("y", 0.0);
            coord.xyz[SG_Z] = instance->attributeAsDouble("z", 0.0);
            coord.hpr[0] = instance->attributeAsDouble("h", 0.0);
            coord.hpr[1] = instance->attributeAsDouble("p", 0.0);
            coord.hpr[2] = instance->attributeAsDouble("r", 0.0);

            std::cout << "  Placing instance at " << coord.xyz[SG_X] << ";" << coord.xyz[SG_Y] << ";" << coord.xyz[SG_Z];
            std::cout << ", orientation " << coord.hpr[0] << ";" << coord.hpr[1] << ";" << coord.hpr[2] << std::endl;
            ssgTransform *trans = new ssgTransform();
            trans->setTransform(&coord);
            if (!is_terrain)
            {//JL. : utile ???
              trans->clrTraversalMaskBits(SSGTRAV_HOT | SSGTRAV_LOS);
            }
            initial_trans->addKid(trans);
            trans->addKid(model);
          }
        }
      }
    }
  }
  /*memorise H of Terrain */
  if ( getHeight_mode==1) make_tab_HeightAndPlane();
  if ( getHeight_mode==2)
  {
    sgMat4 xform;
    sgMakeIdentMat4(xform);
    tiling_terrain(SceneGraph,xform);
  }

  //wind
#if WINDDATA3D == 1
  SimpleXMLTransfer *wind = xml->getChild("wind", true);
  std::string wind_filename = wind->attribute("filename","");
  std::string wind_position_unit = wind->attribute("unit","");
  if (wind_position_unit.compare("m")==0)
        wind_position_coef = FEET2METERS ;
  else
        wind_position_coef = 1;
  std::cout << "wind file name :  " << wind_filename.c_str()<< std::endl;
  if (wind_filename.length() > 0)
  {
    std::cout << "init wind ---------";
    int n = init_wind_data((wind_filename.c_str()));
    std::cout << n << "  points processed" << std::endl;
  }
#endif
}
void ModelBasedScenery::make_tab_HeightAndPlane()
{
  int i,j;
  float x,y,h;
  for (i=0; i < SIZE_GRID_PLANES; i++)
  {
    for (j=0; j < SIZE_GRID_PLANES; j++)
    {
      x = (i- SIZE_GRID_PLANES/2)*SIZE_CELL_GRID_PLANES;
      y = (j- SIZE_GRID_PLANES/2)*SIZE_CELL_GRID_PLANES;
      h=getHeightAndPlane_(x,  y, tab_HeightAndPlane[i][j]);
      tab_HOT [i][j] =h;
    }
  }

}

ModelBasedScenery::~ModelBasedScenery()
{
  if ( getHeight_mode==2)
  {
    for ( int i = 0 ; i <= SIZE_GRID_PLANES; i++ )
      for ( int j = 0 ; j <= SIZE_GRID_PLANES; j++ )
      {
        if (tile_table[i][j]) delete tile_table[i][j];
      }
  }
  delete SceneGraph;
#if WINDDATA3D == 1
  if (wind_data) delete wind_data;
#endif
}

void ModelBasedScenery::draw(double current_time)
{
  ssgCullAndDraw(SceneGraph);
}

float ModelBasedScenery::getHeight(float x_north, float y_east)
{
  return getHeightAndPlane(x_north,  y_east, NULL);
}


float ModelBasedScenery::getHeightAndPlane(float x, float y, float tplane[4])
{
  if ( getHeight_mode==1)
  {
    int i,j;
    float tplane_loc[4];
    float dx, dy, hot;;
    i = (int)(x/SIZE_CELL_GRID_PLANES);
    dx = (x - i*SIZE_CELL_GRID_PLANES)/SIZE_CELL_GRID_PLANES;
    i  += (SIZE_GRID_PLANES/2);
    j = (int)(y/SIZE_CELL_GRID_PLANES);
    dy = (y - j*SIZE_CELL_GRID_PLANES)/SIZE_CELL_GRID_PLANES;
    j  += (SIZE_GRID_PLANES/2);
    if (i<0)
    {
      i=0;
      dx=0;
    }
    if (i >= SIZE_GRID_PLANES)
    {
      i = SIZE_GRID_PLANES-1;
      dx=0;
    }
    if (j<0)
    {
      j=0;
      dy=0;
    }
    if (j >= SIZE_GRID_PLANES)
    {
      j = SIZE_GRID_PLANES-1;
      dy=0;
    }
    sgCopyVec4 (tplane_loc,tab_HeightAndPlane[i][j]);
    if (tplane) sgCopyVec4 (tplane, tplane_loc);
    hot =
      (1-dy)* ( (1-dx)*tab_HOT[i][j]  + dx*tab_HOT[i+1][j]  )
      +  dy * ( (1-dx)*tab_HOT[i][j+1]+ dx*tab_HOT[i+1][j+1]);
    return hot;
  }
  else
  {
    float hot;
    if (getHeight_mode==2)
      hot  = getHeightAndPlane_t(x,  y,  tplane);
    else
      hot = getHeightAndPlane_(x,  y,  tplane);
    return hot;
  }
}

float ModelBasedScenery::getHeightAndPlane_(float x_north, float y_east, float tplane[4])
{
  ssgHit *results ;
  int num_hits ;
  float hot ;   /* H.O.T == Height Of Terrain */
  sgVec3   s;
  sgSetVec3(s, 0.0, 1.0, 0.0);
  sgMat4 m = {  {  1,   0,   0,   0},
                {  0,   1,   0,   0},
                {  0,   0,   1,   0},
                {-y_east , 0, x_north, 1.0}
              };
  num_hits = ssgLOS(SceneGraph, s, m, &results ) ;
#define DEEPEST_HELL  -9999.0
  hot = DEEPEST_HELL ;
  int numero=-1;
  for ( int i = 0 ; i < num_hits ; i++ )
  {
    ssgHit *h = &results [ i ] ;
    float hgt = - h->plane[3] / h->plane[1] ;
    if ( hgt >= hot )
    {
      hot = hgt ;
      numero = i;
    }
  }
  if ( tplane )
  {
    if ( numero >= 0)
    {
      sgCopyVec4 ( tplane, results [ numero ].plane ) ;
      tplane[3] = tplane[3] - tplane[0]*y_east + tplane[2]*x_north;
      if (tplane[1]<0) /*  ??? revoir :  preferable d'orienter correctement les facettes */
        sgNegateVec4 ( tplane , tplane );
      //std::cout << "----plane***" <<  tplane[0] <<"  "<<  tplane[1] <<"  "<<  tplane[2]<<"  "<<  tplane[3] <<std::endl;
    }
    else
    {
      tplane[0] = .0;
      tplane[1] = 1.0;
      tplane[2] = 0.0;
      tplane[3] = -hot;
    }
  }
  return hot;
}
/****/
void ModelBasedScenery::getWindComponents(double X,double  Y,double  Z,
    float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity)
{
  float  flWindVel = cfg->wind->getVelocity();
#if WINDDATA3D == 1
  //import wind data from file
  float x,y,z,vx,vy,vz;
  if (wind_data)
  {
    x = X * wind_position_coef;
    y  = Y * wind_position_coef;
    z  = -Z * wind_position_coef;
    int ret = find_wind_data(x,y,z,&vx,&vy,&vz);
    if (ret)
    {
      *x_wind_velocity = vx * flWindVel;
      *y_wind_velocity = vy * flWindVel;
      *z_wind_velocity = vz * flWindVel;
    }
    else
    {
      *x_wind_velocity = 0.0;
      *y_wind_velocity = 0.0;
      *z_wind_velocity = 0.0;
    }
    //std::cout << "----at"<< x<<"  "<<y<<"  "<< z<<"wind components***" << *x_wind_velocity <<"  "<< *y_wind_velocity <<"  "<<  *z_wind_velocity<<std::endl;
  }
  else
#endif
  {
  //default mode
    *x_wind_velocity = -1 * flWindVel * cos(M_PI*cfg->wind->getDirection()/180);
    *y_wind_velocity = -1 * flWindVel * sin(M_PI*cfg->wind->getDirection()/180);
    *z_wind_velocity = 0;////TODO : simple(?)  vertical wind calculation from Height of Terrain
  }
}
/******/
void ModelBasedScenery::tiling_terrain(ssgEntity * e, sgMat4 xform)
{
  if ( e -> isAKindOf ( ssgTypeBranch() ) )
  {
    ssgBranch *br = (ssgBranch *) e ;
    if ( e -> isA ( ssgTypeTransform() ) )
    {
      sgMat4 xform1;
      ((ssgTransform *)e)->getTransform ( xform1 ) ;
      sgPreMultMat4  ( xform, xform1 ) ;//Pre or Post ???
      /*
      std::cout << "------tranform " << br<< std::endl;
      std::cout << "-------------- " << xform[0][0]<<"  "<< xform[0][1]<<"  "<< xform[0][2]<<"  "<< xform[0][3]<< std::endl;
      std::cout << "-------------- " << xform[1][0]<<"  "<< xform[1][1]<<"  "<< xform[1][2]<<"  "<< xform[1][3]<< std::endl;
      std::cout << "-------------- " << xform[2][0]<<"  "<< xform[2][1]<<"  "<< xform[2][2]<<"  "<< xform[2][3]<< std::endl;
      std::cout << "-------------- " << xform[3][0]<<"  "<< xform[3][1]<<"  "<< xform[3][2]<<"  "<< xform[3][3]<< std::endl;
      */
    }
    //else std::cout << "------branch " << br<< std::endl;
    for ( int i = 0 ; i < br -> getNumKids () ; i++ )
      tiling_terrain ( br -> getKid ( i ), xform) ;
  }
  else if ( e -> isAKindOf ( ssgTypeLeaf() ) )
  {
    //std::cout << "------leaf " << e<< std::endl;
    ssgLeaf  *leaf = (ssgLeaf  *) e ;
    int nt = leaf->getNumTriangles();
    //std::cout << "------n triangles " << nt<< std::endl;
    for ( int i = 0 ; i < nt ; i++ )//pour chaque triangle
    {
      short iv1,iv2,iv3;/*float *v1, *v2, *v3;*/
      sgVec3 v1,v2,v3;
      leaf->getTriangle ( i, &iv1, &iv2,  &iv3 );

      sgCopyVec3 (v1 , leaf->getVertex(iv1));
      sgXformPnt3( v1, xform);
      sgCopyVec3 (v2 , leaf->getVertex(iv2));
      sgXformPnt3( v2, xform);
      sgCopyVec3 (v3 , leaf->getVertex(iv3));
      sgXformPnt3( v3, xform);
      /*
      std::cout << "------triangle " << std::endl;
      std::cout << "-------------- " << v1[0]<<"  "<< v1[1]<<"  "<< v1[2]<<"  "<< std::endl;
      std::cout << "-------------- " << v2[0]<<"  "<< v2[1]<<"  "<< v2[2]<<"  "<< std::endl;
      std::cout << "-------------- " << v3[0]<<"  "<< v3[1]<<"  "<< v3[2]<<"  "<< std::endl;
      */
      //calcule cube englobant
      sgBox box;
      box.empty();
      box.extend(v1);
      box.extend(v2);
      box.extend(v3);
      //std::cout << "Min " << box.min[0]<<", "<<box.min[1]<<", "<<box.min[2]<<" Max"<<box.max[0]<<", "<<box.max[1]<<", "<<box.max[2]<< std::endl;
      //ajoute dans cellules recouvertes
      int save =  1;
      int i1 = (int)(-0.1+box.min[0]/SIZE_CELL_GRID_PLANES) + SIZE_GRID_PLANES/2;
      int i2 = (int)(0.1+box.max[0]/SIZE_CELL_GRID_PLANES) + SIZE_GRID_PLANES/2;
      int j1 = (int)(-0.1+box.min[2]/SIZE_CELL_GRID_PLANES) + SIZE_GRID_PLANES/2;
      int j2 = (int)(0.1+box.max[2]/SIZE_CELL_GRID_PLANES) + SIZE_GRID_PLANES/2;
      if (((i1<0) && (i2<0)) ||((i1>SIZE_GRID_PLANES) && (i2>SIZE_GRID_PLANES)) )save=0;
      if (((j1<0) && (j2<0)) ||((j1>SIZE_GRID_PLANES) && (j2>SIZE_GRID_PLANES)) )save=0;
      if (i1<0) i1=0;
      if (i1>SIZE_GRID_PLANES)i1 = SIZE_GRID_PLANES;
      if (i2<0) i2=0;
      if (i2>SIZE_GRID_PLANES)i2 = SIZE_GRID_PLANES;
      if (j1<0) j1=0;
      if (j1>SIZE_GRID_PLANES)j1 = SIZE_GRID_PLANES;
      if (j2<0) j2=0;
      if (j2>SIZE_GRID_PLANES)j2 = SIZE_GRID_PLANES;
      //std::cout << "cellules i, j  " << i1 <<", "<<i2<<", "<<j1<<","<<j2<< std::endl;
      if (save)
        for ( int i = i1 ; i <= i2; i++ )
          for ( int j = j1 ; j <= j2; j++ )
          {
            //std::cout << "met dans cellule " << i <<", "<<j<<  std::endl;
            tile_table[i][j]->add(v1);
            tile_table[i][j]->add(v2);
            tile_table[i][j]->add(v3);
          }
    }
  }
}
int on_triangle(float x, float y, float *p1,float *p2,float *p3);

float ModelBasedScenery::getHeightAndPlane_t(float x_north, float y_east, float tplane[4])
{
  float h,hot ;   /* H.O.T == Height Of Terrain */

  float   *p1,*p2,*p3;
  int numero=-1;

  int ix = (int)(y_east/SIZE_CELL_GRID_PLANES) + SIZE_GRID_PLANES/2;
  int jy = (int)(-x_north/SIZE_CELL_GRID_PLANES) + SIZE_GRID_PLANES/2;
  if (ix<0) ix=0;
  if (ix>SIZE_GRID_PLANES)ix = SIZE_GRID_PLANES;
  if (jy<0) jy=0;
  if (jy>SIZE_GRID_PLANES)jy = SIZE_GRID_PLANES;
  ssgVertexArray* tile = tile_table[ix][jy];
//std::cout << "utilise cellule " << ix<<", "<<jy<< std::endl;

  int n = tile->getNum();
#define DEEPEST_HELL  -9999.0
  hot = DEEPEST_HELL ;
//std::cout << "cellule nb triangle/cellule*3 i,j "<<tile <<"  " << n <<"  "<< ix <<"  "<< jy << std::endl;
//std::cout << "nord, east" <<x_north<<"  "<< y_east <<"  " << std::endl;

  for ( int i = 0 ; i < n ; i+=3 )
  {
    p1= tile->get(i);
    p2= tile->get(i+1);
    p3= tile->get(i+2);
    /*
    std::cout << "-------------- " << p1[0]<<"  "<< p1[1]<<"  "<< p1[2]<<"  "<< std::endl;
    std::cout << "-------------- " << p2[0]<<"  "<< p2[1]<<"  "<< p2[2]<<"  "<< std::endl;
    std::cout << "-------------- " << p3[0]<<"  "<< p3[1]<<"  "<< p3[2]<<"  "<< std::endl;
    */
    if ( on_triangle(-x_north, y_east, p1, p2, p3))
    {
      sgVec4 plane;
      sgMakePlane ( plane, p1, p2, p3);
      h = -(-plane[2]*x_north + plane[0]*y_east + plane[3] )/ plane[1];
      if (h>hot)
      {
        numero = i;
        hot=h;
        if (tplane) sgCopyVec4(tplane, plane);
      }
      //std::cout << "----plane***" <<  plane[0] <<"  "<<  plane[1] <<"  "<<  plane[2]<<"  "<<  plane[3] << "hot" << hot <<std::endl;
    }
  }
  if ( tplane )
  {
    if ( numero >= 0)
    {
      if (tplane[1]<0) /*  ??? revoir :  preferable d'orienter correctement les facettes */
        sgNegateVec4 ( tplane , tplane );
    }
    else
    {
      tplane[0] = .0;
      tplane[1] = 1.0;
      tplane[2] = 0.0;
      tplane[3] = -hot;
    }
  }
  return hot;
}
/**********************/
//utility routines
typedef struct
{
  float x;
  float y;
} myPoint2D;
float Isleft(myPoint2D p1, myPoint2D p2, myPoint2D p)
{
  return ((p2.x - p1.x) * (p.y - p1.y) - (p.x - p1.x) * (p2.y - p1.y));
}
int on_triangle(float x, float y, float *p1,float *p2,float *p3)
// test si point x,y, dans la projection du trianle p1, p2,p3 sur plan horizontal
{
  int t1,t2,t3,test;
  myPoint2D a,b,c,p;
  p.x= x;//east
  p.y = y;//nord
  //p1,p2,p3 : est, up,sud
  a.x = p1[2];
  a.y = p1[0];
  b.x = p2[2];
  b.y = p2[0];
  c.x = p3[2];
  c.y = p3[0];
  t1 = (Isleft(c,a,p)>=0);
  t2 = (Isleft(b,c,p)>=0);
  t3 = (Isleft(a,b,p)>=0);
  test =  (t1==t2) && (t1==t3);
  return test;
}
