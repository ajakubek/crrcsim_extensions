/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 *   Copyright (C) 2000, 2001 Jan Kansky (original author)
 *   Copyright (C) 2004, 2005, 2006, 2008, 2009 Jan Reucker
 *   Copyright (C) 2005, 2008 Jens Wilhelm Wulf
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

#ifndef CRRC_SCENERY_H
#define CRRC_SCENERY_H

#include "../mod_math/CVector.h"
#include "../include_gl.h"
#include "../mod_misc/SimpleXMLTransfer.h"
#include "crrc_sky.h"
#include <plib/sg.h>
#include <vector>
#include "winddata3D.h"

#define HEIGHTMAP_SIZE_X  (64)
#define HEIGHTMAP_SIZE_Z  (64)

//~ #define SCENERY_SIZE_X    (2048.0)
//~ #define SCENERY_SIZE_Z    (2048.0)
//~ #define SCENERY_OFFSET_X  (SCENERY_SIZE_X / 2)
//~ #define SCENERY_OFFSET_Z  (SCENERY_SIZE_Z / 2)

typedef struct
{
  float north;
  float east;
  float height;
  std::string name;
} T_Position;

typedef std::vector<T_Position> T_PosnArray;


/** \brief Abstract base class for scenery classes
 *
 *  This base class defines the public interface common to
 *  all scenery-drawing classes. It also reads common
 *  information from the scenery file (sky initialization,
 *  player and start positions, ...)
 */
class Scenery
{
  public:
    /**
     *  Initialization from XML description
     */
    Scenery(SimpleXMLTransfer *xml);

    /**
     *  Destructor
     */
    virtual ~Scenery();

    /**
     *  Draw the scenery
     *
     *  \param current_time current time in ms (for animation effects)
     */
    virtual void draw(double current_time) = 0;

    /**
     *  Setup the sky from the XML branch stored as "sky_params".
     */
    virtual void setupSky();
  
    /**
     *  Draw the sky
     */
    virtual void drawSky(sgVec3 *campos, double current_time);
    
    /**
     *  Get player position
     */
    virtual CVector getPlayerPosition(int num = 0);
    virtual CVector getStartPosition(int num = 0);
    virtual int getNumStartPosition();
  
    /**
     *  Get the height at a distinct point.
     *  \param x x coordinate
     *  \param z z coordinate
     *  \return terrain height at this point in ft
     */
    virtual float getHeight(float x, float z) = 0;
    
    /**
     *  Get the default wind direction specified in the scenery file.
     */
    inline float getDefaultWindDirection(void) {return flDefaultWindDirection;};

    /**
     *  Get the default wind speed specified in the scenery file.
     */
    inline float getDefaultWindSpeed(void) {return flDefaultWindSpeed;};

    /**
     *  get height and plane equation at x|z
     *  \param x x coordinate
     *  \param z z coordinate
     *  \param tplane this is where the plane equation will be stored
     *  \return terrain height at this point in ft
     */
    virtual float getHeightAndPlane(float x, float z, float tplane[4]) = 0;
    
  /*
  get  wind on  directions  at position  X_cg, Y_cg,Z_cg
  */
  virtual void getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
      float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity)=0;;

    /**
     *  Get an ID code for this location or scenery type
     */
    virtual int getID() = 0;
    
    /**
     *  Get the name of this location as specified in the
     *  XML scenery description file.
     *
     *  \return scenery name
     */
    inline const char *getName()  {return name.c_str();};

    /**
     *  locations:
     *  DAVIS          CRRC Davis flying field in Sudbry, Mass
     *  MEDFIELD       CRRC Medfield flying site in Medfield, Mass
     *  CAPE_COD       Slope Soaring at Cape Cod, Massachusetts 
     *  XML_HMAP       read from scenery file (old height-map format)
     *  NULL_RENDERER  empty renderer
     *
     *  \todo Everything that depends on these constants should
     *  get its information from the scenery file instead of
     *  using hard-coded stuff. In the end these constants should
     *  only be used as an argument to the ctor of BuiltinScenery.
     */
    enum { DAVIS=1, MEDFIELD=2, CAPE_COD=3, XML_HMAP=4, NULL_RENDERER=5,
           MODEL_BASED=6, PHOTO=7 };

  protected:
    float flDefaultWindSpeed;       ///< default wind speed from scenery file in ft/s
    float flDefaultWindDirection;   ///< default wind direction from scenery file in degrees
    std::string name;               ///< name of this location, from XML description
  
  private:
    /**
     *  Read a set of positions into a position array
     */
    int parsePositions(SimpleXMLTransfer *tag, T_PosnArray& pa, bool default_on_empty = true);
  
    T_PosnArray views;
    T_PosnArray starts;
    
    SkyRenderer       *theSky;      ///< pointer to the sky class
    SimpleXMLTransfer *sky_params;  ///< sky parameters from scenery file
};


/** \brief Class for creating and drawing the terrain.
 *
 *
 */
class BuiltinScenery : public Scenery
{
  public:
    /**
     *  Constructor that initializes the object from
     *  a SimpleXMLTransfer object.
     */
    BuiltinScenery(SimpleXMLTransfer *xml, bool boIsNullRenderer = false);

    /**
     *  Constructor for old XML_HMAP format.
     *  \todo Make it work properly or delete it.
     */
    BuiltinScenery(const char *mapfile);
  
    /**
     *  Destructor
     */
    ~BuiltinScenery();

    /**
     *  Draw the scenery
     */
    virtual void draw(double current_time) = 0;

    // Draw normal vectors for debugging purposes
    void draw_normals(float length);
  
    /**
     *  Set texturing mode.
     *
     *  \todo Does this still make sense? Not used right now.
     *  Switching on the textures won't work if they were turned
     *  off while the object was created. Make it work or
     *  delete it.
     */
    void setTextures(bool yesno);
  
    /** 
     *  Get the terrain height at (x|z). Must be implemented
     *  by the child classes.
     *  \param x x coordinate
     *  \param z z coordinate
     *  \return terrain height at this point in ft
     */
    virtual float getHeight(float x, float z) = 0;
    
    /**
     *  Get height and plane equation of the terrain at (x|z).
     *  Must be implemented by the child classes.
     *  \param x x coordinate
     *  \param z z coordinate
     *  \param tplane this is where the plane equation will be stored
     *  \return terrain height at this point in ft
     */
    virtual float getHeightAndPlane(float x, float z, float tplane[4]) = 0;


    /**
     *  Get an ID code for this location or scenery type
     *  Must be implemented by the child classes.
     */
    virtual int getID() = 0;

    /*
  get  wind on  directions  at position  X_cg, Y_cg,Z_cg
  */
  virtual void getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
      float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity)=0;
    /**/
protected:
    int use_textures;
    GLUquadricObj *quadric; ///\todo remove dependencies to GLUT

    void setup_drawing_state();
    void restore_drawing_state();
    
  private:
    void calculate_normals();
    void compile_display_list();
  
    unsigned int list;
  
    float size_x;
    float size_z;
    float offset_x;
    float offset_z;
    float alt_min;
    float alt_max;
    CVector model_start;
    CVector player;

    float height[HEIGHTMAP_SIZE_X][HEIGHTMAP_SIZE_Z];
    CVector normal[HEIGHTMAP_SIZE_X][HEIGHTMAP_SIZE_Z];
};


/** \brief Class for rendering the built-in Davis field
 *
 *  This class renders the original built-in Davis field
 *  scenery.
 */
class BuiltinSceneryDavis : public BuiltinScenery
{
  public:
    BuiltinSceneryDavis(SimpleXMLTransfer *xml);
    ~BuiltinSceneryDavis();

    /** 
     *  Get the terrain height at (x|z).
     *  \param x x coordinate
     *  \param z z coordinate
     *  \return terrain height at this point in ft
     */
    float getHeight(float x, float z);
    
    /**
     *  Get height and plane equation of the terrain at (x|z).
     *  \param x x coordinate
     *  \param z z coordinate
     *  \param tplane this is where the plane equation will be stored
     *  \return terrain height at this point in ft
     */
    float getHeightAndPlane(float x, float z, float tplane[4]);

    /**
     *  Get an ID code for this location or scenery type
     */
    int getID() {return location;};
    /*
  get  wind on  directions  at position  X_cg, Y_cg,Z_cg
  */
   void getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
      float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity);

    /**
     *  Draw the scenery
     *  \param current_time current time in ms (for animation effects)
     */
    void draw(double current_time);

    
  private:
    void read_textures(SimpleXMLTransfer *xml);
    void clear_textures();
    int location;

    unsigned char *ground_texture;
    int ground_texture_width;
    int ground_texture_height;
    unsigned char *grass_texture;
    int grass_texture_width;
    int grass_texture_height;
    unsigned char *grass_side_texture;
    int grass_side_texture_width;
    int grass_side_texture_height;
    unsigned char *grass_top_texture;
    int grass_top_texture_width;
    int grass_top_texture_height;
    unsigned char *eastern_view_texture;
    int eastern_view_texture_width;
    int eastern_view_texture_height;
    unsigned char *netrees_texture;
    int netrees_texture_width;
    int netrees_texture_height;
    unsigned char *dirt_texture;
    int dirt_texture_width;
    int dirt_texture_height;
    unsigned char *outhouse_texture;
    int outhouse_texture_width;
    int outhouse_texture_height;
    unsigned char *freq_texture;
    int freq_texture_width;
    int freq_texture_height;
    unsigned char *pine_texture;
    int pine_texture_width;
    int pine_texture_height;
    unsigned char *decid_texture;
    int decid_texture_width;
    int decid_texture_height;
    GLuint groundTexture;          // GL ground texture handle
    GLuint grassTexture;          // GL grass texture handle
    GLuint grassSideTexture;          // GL grass Side texture handle
    GLuint grassTopTexture;          // GL grass Top texture handle
    GLuint pineTexture;          // GL grass texture handle
    GLuint decidTexture;          // GL grass texture handle
    GLuint easternViewTexture;          // Looking to the east
    GLuint netreesTexture;          // Looking to the east
    GLuint dirtTexture;          // Looking to the east
    GLuint outhouseTexture;          // Looking to the east
    GLuint freqTexture;          // Looking to the east
};


/** \brief Class for rendering the built-in Cape Cod slope
 *
 *  This class renders the original built-in Cape Cod slope
 *  scenery.
 */
class BuiltinSceneryCapeCod : public BuiltinScenery
{
  public:
    BuiltinSceneryCapeCod(SimpleXMLTransfer *xml);
    ~BuiltinSceneryCapeCod();

    /** 
     *  Get the terrain height at (x|z).
     *  \param x x coordinate
     *  \param z z coordinate
     *  \return terrain height at this point in ft
     */
    float getHeight(float x, float z);
    
    /**
     *  Get height and plane equation of the terrain at (x|z).
     *  \param x x coordinate
     *  \param z z coordinate
     *  \param tplane this is where the plane equation will be stored
     *  \return terrain height at this point in ft
     */
    float getHeightAndPlane(float x, float z, float tplane[4]);

    /**
     *  Get an ID code for this location or scenery type
     */
    int getID() {return location;};

     /*
  get  wind on  directions  at position  X_cg, Y_cg,Z_cg
  */
   void getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
      float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity);
  
   /**
     *  Draw the scenery
     */
    void draw(double current_time);
    
  private:
    int location;
    void read_textures(SimpleXMLTransfer *xml);
    void clear_textures();

    unsigned char *water_texture;
    int water_texture_width;
    int water_texture_height;
    unsigned char *beachsand_texture;
    int beachsand_texture_width;
    int beachsand_texture_height;
    unsigned char *scrub_texture;
    int scrub_texture_width;
    int scrub_texture_height;
    unsigned char *scrubedge_texture;
    int scrubedge_texture_width;
    int scrubedge_texture_height;
    unsigned char *south_texture;
    int south_texture_width;
    int south_texture_height;
    unsigned char *hilledge_texture;
    int hilledge_texture_width;
    int hilledge_texture_height;
    unsigned char *waves_texture;
    int waves_texture_width;
    int waves_texture_height;
    unsigned char *sand_texture;
    int sand_texture_width;
    int sand_texture_height;
    GLuint waterTexture;
    GLuint beachsandTexture;
    GLuint scrubTexture;
    GLuint scrubedgeTexture;
    GLuint southTexture;
    GLuint hilledgeTexture;
    GLuint wavesTexture;
    GLuint sandTexture;
};


/** \brief NULL renderer class
 *
 *  This class renders no scenery at all, but implements
 *  a dummy interface for the rendering engine. Use it
 *  e.g. if you want to render a purely skybox-based
 *  "panorama" scenery.
 */
class BuiltinSceneryNull : public BuiltinScenery
{
  public:
    /**
     *  The constructor
     *
     *  \param xml SimpleXMLTransfer from which the base classes will be initialized
     */
    BuiltinSceneryNull(SimpleXMLTransfer *xml);
  
    /**
     *  The destructor
     */
    ~BuiltinSceneryNull();

    /** 
     *  Get the terrain height at (x|z).
     *  \param x x coordinate
     *  \param z z coordinate
     *  \return terrain height at this point in ft
     */
    float getHeight(float x, float z);
    
    /**
     *  Get height and plane equation of the terrain at (x|z).
     *  \param x x coordinate
     *  \param z z coordinate
     *  \param tplane this is where the plane equation will be stored
     *  \return terrain height at this point in ft
     */
    float getHeightAndPlane(float x, float z, float tplane[4]);

    /**
     *  Get an ID code for this location or scenery type
     */
    int getID() {return location;};
     /*
  get  wind on  directions  at position  X_cg, Y_cg,Z_cg
  */
   void getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
      float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity);
  

    /**
     *  Draw the scenery
     */
    void draw(double current_time);
  
  private:
    int   location;   ///< location id
    float height;     ///< constant height of the scenery
};


/**
 *  \brief Class for 3D-model-based sceneries
 *
 */
class ModelBasedScenery : public Scenery
{
  public:
    /**
     *  The constructor
     *
     *  \param xml SimpleXMLTransfer from which the base classes will be initialized
     */
    ModelBasedScenery(SimpleXMLTransfer *xml);
  
    /**
     *  The destructor
     */
    ~ModelBasedScenery();
  
    /**
     *  Draw the scenery
     */
    void draw(double current_time);


    /**
     *  Get the height at a distinct point.
     *  \param x x coordinate
     *  \param z z coordinate
     *  \return terrain height at this point in ft
     */
    float getHeight(float x, float z);


    /**
     *  get height and plane equation at x|z
     *  \param x x coordinate
     *  \param z z coordinate
     *  \param tplane this is where the plane equation will be stored
     *  \return terrain height at this point in ft
     */
    float getHeightAndPlane(float x, float z, float tplane[4]);
    
    /**
     *  Get an ID code for this location or scenery type
     */
    int getID() {return location;};
    /*
  get  wind on  directions  at position  X_cg, Y_cg,Z_cg
  */
    void getWindComponents(double X_cg,double  Y_cg,double  Z_cg,
      float  *x_wind_velocity, float  *y_wind_velocity, float  *z_wind_velocity);
    /**/
  
  private:
    int   location;   ///< location id
    ssgRoot       *SceneGraph;
    ssgTransform  *initial_trans;
  int getHeight_mode;
      //0 :  use ssgLOS ( slow if many triangle)
      //1 :  ssgLOS()s en table (not god)
      //2 : Tiling of surface 
      
  

#define SIZE_GRID_PLANES 150
#define SIZE_CELL_GRID_PLANES 20
  ssgVertexArray* tile_table[SIZE_GRID_PLANES+1][SIZE_GRID_PLANES+1];
  void make_tab_HeightAndPlane(); 
  float tab_HeightAndPlane [SIZE_GRID_PLANES+1][SIZE_GRID_PLANES+1][4];
  float tab_HOT [SIZE_GRID_PLANES+1][SIZE_GRID_PLANES+1];
  float getHeightAndPlane_(float x, float z, float tplane[4]);
  float getHeightAndPlane_t(float x_north, float y_east, float tplane[4]);
  void tiling_terrain(ssgEntity * e, sgMat4 xform=NULL);
#if WINDDATA3D == 1
  int init_wind_data(const char* filename);
  int find_wind_data(float n,float e,float u, float *vx, float *vy, float * vz);
  WindData  * wind_data;
#endif
  float wind_position_coef;
  };


/**
 *  Load a scenery from a file
 */
Scenery* loadScenery(const char *fname);

#endif  // CRRC_SCENERY_H
