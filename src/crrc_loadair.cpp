/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2000, 2001 Jan Edward Kansky (original author)
 * Copyright (C) 2005, 2006, 2007, 2008 Jan Reucker
 * Copyright (C) 2005, 2006, 2008 Jens Wilhelm Wulf
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
  
/** \file crrc_loadair.cpp
 * 
 *  Routines dealing with airplane file loading
 */

#include "crrc_loadair.h"

#include <fstream>
#include <iostream>
#include <list>
#include <math.h>

#include <plib/ssg.h>

#include "global.h"
#include "mod_misc/filesystools.h"
#include "crrc_sound.h"
#include "mod_misc/ls_constants.h"
#include "crrc_main.h"
#include "crrc_graphics.h"
#include "mod_fdm/xmlmodelfile.h"
#include "crrc_animation.h"
#include "crrc_ssgutils.h"



CRRCAirplane::CRRCAirplane()
{
  
}

CRRCAirplane::~CRRCAirplane()
{
  for (int i = 0; i < (int)sound.size(); i++)
  {
    Global::soundserver->stopChannel(sound[i]->getChannel());
    delete sound[i];
  }
}


CRRCAnimation* CRRCAirplane::findAnimation(std::string anim_name)
{
  CRRCAnimation *anim = NULL;
  std::vector<CRRCAnimation*>::iterator it;

  for (it = animations.begin(); it != animations.end(); it++)
  {
    if ((*it)->getName() == anim_name)
    {
      anim = *it;
      break;
    }
  }

  return anim;
}


/*****************************************************************************/

CRRCAirplaneLaRCSim::CRRCAirplaneLaRCSim()
{
}


CRRCAirplaneLaRCSim::CRRCAirplaneLaRCSim(SimpleXMLTransfer* xml)
{
  initSound(xml);    
}

CRRCAirplaneLaRCSim::~CRRCAirplaneLaRCSim()
{
}


void CRRCAirplaneLaRCSim::initSound(SimpleXMLTransfer* xml)
{
  SimpleXMLTransfer* cfg = XMLModelFile::getConfig(xml);
  SimpleXMLTransfer* sndcfg = cfg->getChild("sound", true);
  int children = sndcfg->getChildCount();
  int units = sndcfg->getInt("units", 0);
  
  for (int i = 0; i < children; i++)
  {
    SimpleXMLTransfer *child = sndcfg->getChildAt(i);
    std::string name = child->getName();
    
    if (name.compare("sample") == 0)
    {
      T_AirplaneSound *sample;

      // assemble relative path
      std::string soundfile;
      soundfile           = child->attribute("filename");

      // other sound attributes
      int sound_type      = child->getInt("type", SOUND_TYPE_GLIDER);
      double dPitchFactor = child->getDouble("pitchfactor", 0.002);
      double dMaxVolume   = child->getDouble("maxvolume", 1.0);
  
      if (dMaxVolume < 0.0)
      {
        dMaxVolume = 0.0;
      }
      else if (dMaxVolume > 1.0)
      {
        dMaxVolume = 1.0;
      }

  //~ if (cfg->indexOfChild("power") < 0)
    //~ max_thrust = 0;
  //~ else
    //~ max_thrust = 1;
  
      if (soundfile != "")
      {
        // Get full path (considering search paths). 
        soundfile = FileSysTools::getDataPath("sounds/" + soundfile);
      }
      
      // File ok? Use default otherwise.
      if (!FileSysTools::fileExists(soundfile))
        soundfile = FileSysTools::getDataPath("sounds/fan.wav");
    
      std::cout << "soundfile: " << soundfile << "\n";
      //~ std::cout << "max_thrust: " << max_thrust << "\n";
      std::cout << "soundserver: " << Global::soundserver << "\n";
  
      // Only make noise if a sound file is available
      if (soundfile != "" && Global::soundserver != (CRRCAudioServer*)0)
      {        
        std::cout << "Using airplane sound " << soundfile << ", type " << sound_type << ", max vol " << dMaxVolume << std::endl;
        
        if (sound_type == SOUND_TYPE_GLIDER)
        {
          T_GliderSound *glidersound;
          float flMinRelV, flMaxRelV, flMaxDist;
          flMinRelV = (float)child->getDouble("v_min", 1.5);
          flMaxRelV = (float)child->getDouble("v_max", 4.0);
          flMaxDist = (float)child->getDouble("dist_max", 300);
          
          if (units == 1)
          {
            // convert from metric units to ft.
            flMaxDist *= M_TO_FT;
          }
          
          glidersound = new T_GliderSound(soundfile.c_str(), Global::soundserver->getAudioSpec());
          glidersound->setMinRelVelocity(flMinRelV);
          glidersound->setMaxRelVelocity(flMaxRelV);
          glidersound->setMaxDistanceFeet(flMaxDist);
          sample = glidersound;
        }
        else
        {
          sample = new T_EngineSound(soundfile.c_str(), Global::soundserver->getAudioSpec());
        }
                
        sample->setType(sound_type);
        sample->setPitchFactor(dPitchFactor);
        sample->setMaxVolume(dMaxVolume);
        sample->setChannel(Global::soundserver->playSample((T_SoundSample*)sample));
        sound.push_back(sample);
      }
    }
  }
}


/*****************************************************************************/

#ifdef EXPERIMENTAL_STENCIL_SHADOW
int shadowPredrawCallback(ssgState*)
{
  glStencilMask(1);
  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_GREATER, 1, 1);
  glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  return 0;
}

int shadowPostdrawCallback(ssgState*)
{
  glStencilMask(0);
  glDisable(GL_STENCIL_TEST);
  return 0;
}
#endif


/** \brief Create a "shadow" instance of a model
 *
 *  This function operates on a clone of a model and sets all associated
 *  states to a "shadowy" look. If it is called for an entity that has the
 *  "-shadow" attribute set, return true to signal the caller that he has to
 *  remove this entity from the graph.
 *  
 *  \todo If the video context supports stencil buffering, the shadow
 *        should be rendered using the stencil buffer to get a translucent look.
 *
 *  \param ent  A clone of the "real" model
 *  \retval true The current entity has to be removed from the graph
 *  \retval false No action required
 */
bool makeShadow(ssgEntity *ent)
{
  bool boRemoveCurrentEntity = false;
  
  if (ent->isAKindOf(ssgTypeLeaf()))
  {
    ssgLeaf *leaf = (ssgLeaf*)ent;

    SSGUtil::NodeAttributes attr = SSGUtil::getNodeAttributes(leaf);
    if (!attr.castsShadow)
    {
      // no shadow --> remove this entity from all parents
      boRemoveCurrentEntity = true;
    }
    else if (leaf->hasState())
    {
      ssgSimpleState *state = (ssgSimpleState*)leaf->getState();
      state->disable(GL_COLOR_MATERIAL);
      state->disable(GL_TEXTURE_2D);
      state->enable(GL_LIGHTING);
      state->enable(GL_BLEND);
      state->setShadeModel(GL_SMOOTH);
      state->setShininess(0.0f);
      state->setMaterial(GL_EMISSION, 0.0, 0.0, 0.0, 0.0);
      
      #ifdef EXPERIMENTAL_STENCIL_SHADOW
      if (vidbits.stencil)
      {
        state->setMaterial(GL_AMBIENT, 0.0, 0.0, 0.0, 0.1);
        state->setMaterial(GL_DIFFUSE, 0.0, 0.0, 0.0, 0.1);
        state->setMaterial(GL_SPECULAR, 0.0, 0.0, 0.0, 0.1);
        state->setStateCallback(SSG_CALLBACK_PREDRAW, shadowPredrawCallback);
        state->setStateCallback(SSG_CALLBACK_POSTDRAW, shadowPostdrawCallback);
      }
      else
      #endif
      {
        state->setMaterial(GL_AMBIENT, 0.0, 0.0, 0.0, 1.0);
        state->setMaterial(GL_DIFFUSE, 0.0, 0.0, 0.0, 1.0);
        state->setMaterial(GL_SPECULAR, 0.0, 0.0, 0.0, 1.0);
        state->setStateCallback(SSG_CALLBACK_PREDRAW, NULL);
        state->setStateCallback(SSG_CALLBACK_POSTDRAW, NULL);
      }
    }
  }
  else if (ent->isAKindOf(ssgTypeBranch()))
  {
    ssgBranch *branch = (ssgBranch*)ent;

    // continue down the hierarchy
    std::list<ssgLeaf*> ToBeRemoved;
    std::list<ssgLeaf*>::iterator it;
    int kids = branch->getNumKids();
    for (int i = 0; i < kids; i++)
    {
      ssgEntity* currKid = branch->getKid(i);
      if (makeShadow(currKid))
      {
        ToBeRemoved.push_back(static_cast<ssgLeaf*>(currKid));
      }
    }
    for (it = ToBeRemoved.begin(); it != ToBeRemoved.end(); it++)
    {
      SSGUtil::removeLeafFromGraph(*it);
    }
    

  }
  return boRemoveCurrentEntity;
}


/** \brief The constructor.
 *
 *  Loads an airplane model from an xml description. The 3D model
 *  will be added to the specified scenegraph.
 *
 *  \param  xml   XML model description file
 *  \param  graph Pointer to the scenegraph which shall render the model
 */
CRRCAirplaneLaRCSimSSG::CRRCAirplaneLaRCSimSSG(SimpleXMLTransfer* xml, ssgBranch *graph)
  : CRRCAirplaneLaRCSim(xml), initial_trans(NULL), 
    model_trans(NULL), model(NULL),
    shadow(NULL), shadow_trans(NULL)
{
  printf("CRRCAirplaneLaRCSimSSG(xml, branch)\n");
  
  std::string s;      
  s = XMLModelFile::getGraphics(xml)->getString("model");
        
  // plib automatically loads the texture file, but it does not know which directory to use.
  {
    // where is the object file?
    std::string    of  = FileSysTools::getDataPath("objects/" + s);
    // compile and set relative texture path
    std::string    tp  = of.substr(0, of.length()-s.length()-1-7) + "textures";    
    ssgTexturePath(tp.c_str());
    // load model
    model = ssgLoad(of.c_str());
  }

  if (model != NULL)
  {
    // Offset of center of gravity
    CRRCMath::Vector3  pCG;         
    pCG = CRRCMath::Vector3(0, 0, 0);
    if (xml->indexOfChild("CG") >= 0)
    {
      SimpleXMLTransfer* i;
      i = xml->getChild("CG");
      pCG.r[0] = i->attributeAsDouble("x", 0);
      pCG.r[1] = i->attributeAsDouble("y", 0);
      pCG.r[2] = i->attributeAsDouble("z", 0);
      
      if (i->attributeAsInt("units") == 1)
        pCG *= M_TO_FT;
    }
    
    // transform model from SSG coordinates to CRRCsim coordinates
    initial_trans = new ssgTransform();
    model_trans = new ssgTransform();
    graph->addKid(model_trans);
    model_trans->addKid(initial_trans);
    initial_trans->addKid(model);
    
    sgMat4 it = {  {1.0,  0.0,  0.0,  0},
                   {0.0,  0.0, -1.0,  0},
                   {0.0,  1.0,  0.0,  0},
                   {pCG.r[1],  pCG.r[2],  -pCG.r[0],  1.0} };
    
    initial_trans->setTransform(it);

    // add a simple shadow
    shadow = (ssgEntity*)initial_trans->clone(SSG_CLONE_RECURSIVE | SSG_CLONE_GEOMETRY | SSG_CLONE_STATE);
    makeShadow(shadow);
    shadow_trans = new ssgTransform();
    graph->addKid(shadow_trans);
    shadow_trans->addKid(shadow);
    
    // add animations ("real" model only, without shadow)
    initAnimations(xml, model, &Global::inputs, animations);
  }
  else
  {
    std::string msg = "Unable to open airplane model file \"";
    msg += s;
    msg += "\"\nspecified in \"";
    msg += xml->getSourceDescr();
    msg += "\"";

    throw std::runtime_error(msg);
  }

}

CRRCAirplaneLaRCSimSSG::~CRRCAirplaneLaRCSimSSG()
{
  model_trans->getParent(0)->removeAllKids();
}

#ifdef SOME_TESTS
inline void makeEulerZXZRotMat4(sgMat4 mat, double phi, double theta, double psi)
{
  float cpsi = (float)cos(psi);
  float cphi = (float)cos(phi);
  float cthe = (float)cos(theta);
  float spsi = (float)sin(psi);
  float sphi = (float)sin(phi);
  float sthe = (float)sin(theta);
  
  mat[0][0] = cpsi * cphi - cthe * sphi * spsi;
  mat[0][1] = -1 * spsi * cphi - cthe * sphi * cpsi;
  mat[0][2] = sthe * sphi;
  mat[0][3] = SG_ZERO;
  
  mat[1][0] = cpsi * sphi + cthe * cphi * spsi;
  mat[1][1] = -1 * spsi * sphi + cthe * cphi * cpsi;
  mat[1][2] = -1 * sthe * cphi;
  mat[1][3] = SG_ZERO;
  
  mat[2][0] = spsi * sthe;
  mat[2][1] = cpsi * sthe;
  mat[2][2] = cthe;
  mat[2][3] = SG_ZERO;
  
  mat[3][0] = SG_ZERO;
  mat[3][1] = SG_ZERO;
  mat[3][2] = SG_ZERO;
  mat[3][3] = SG_ONE;
}

inline void makeEulerZYZRotMat4(sgMat4 mat, double phi, double theta, double psi)
{
  float cpsi = (float)cos(psi);
  float cphi = (float)cos(phi);
  float cthe = (float)cos(theta);
  float spsi = (float)sin(psi);
  float sphi = (float)sin(phi);
  float sthe = (float)sin(theta);
  
  mat[0][0] = -1 * spsi * sphi + cthe * cphi * cpsi;
  mat[0][1] = -1 * cpsi * sphi - cthe * cphi * spsi;
  mat[0][2] = sthe * cphi;
  mat[0][3] = SG_ZERO;
  
  mat[1][0] = spsi * cphi + cthe * sphi * cpsi;
  mat[1][1] = cpsi * cphi - cthe * sphi * spsi;
  mat[1][2] = sthe * sphi;
  mat[1][3] = SG_ZERO;
  
  mat[2][0] = -1 * cpsi * sthe;
  mat[2][1] = spsi * sthe;
  mat[2][2] = cthe;
  mat[2][3] = SG_ZERO;
  
  mat[3][0] = SG_ZERO;
  mat[3][1] = SG_ZERO;
  mat[3][2] = SG_ZERO;
  mat[3][3] = SG_ONE;
}
#endif


/** \brief Create a rotation matrix
 *
 *  This function creates a rotation matrix from the original
 *  OpenGL glRotatef commands in CRRCAirplaneLaRCSim::draw().
 *
 *  \param m The matrix to be rotated
 *  \param phi Euler angle phi
 *  \param theta Euler angle theta
 *  \param psi Euler angle psi
 */
inline void makeOGLRotMat4(sgMat4 m, double phi, double theta, double psi)
{
  sgMat4 temp;
  sgVec3 rvec;
  
  //~ sgSetVec3(rvec, 0.0, 1.0, 0.0);
  //~ sgMakeRotMat4(temp, 90.0, rvec);
  //~ sgPreMultMat4(m, temp);
  
  sgSetVec3(rvec, 0.0, 1.0, 0.0);
  sgMakeRotMat4(temp, 180.0f - (float)psi * SG_RADIANS_TO_DEGREES, rvec);
  sgPreMultMat4(m, temp);
  
  sgSetVec3(rvec, -1.0, 0.0, 0.0);
  sgMakeRotMat4(temp, (float)theta * SG_RADIANS_TO_DEGREES, rvec);
  sgPreMultMat4(m, temp);
  
  sgSetVec3(rvec, 0.0, 0.0, 1.0);
  sgMakeRotMat4(temp, (float)phi * SG_RADIANS_TO_DEGREES, rvec);
  sgPreMultMat4(m, temp);
}


/** \brief Draw the airplane
 *
 *  This method actually does not draw anything. It only
 *  updates the aircraft's transformation node in the
 *  scenegraph with the aircraft's current position and
 *  orientation. The actual drawing is done by the global
 *  ssgCullAndDraw() call.
 *
 *  \param airplane pointer to the airplane's FDM object
 */
void CRRCAirplaneLaRCSimSSG::draw(FDMBase* airplane)
{
  CRRCMath::Vector3 pos = airplane->getPos();
  
  sgMat4 m;
  sgMakeIdentMat4(m);

  m[3][0] = pos.r[1];
  m[3][1] = -1 * pos.r[2];
  m[3][2] = -1 * pos.r[0];
  makeOGLRotMat4(m, airplane->getPhi(), airplane->getTheta(), airplane->getPsi());
  model_trans->setTransform(m);
  
}


/** \brief Calculate pitch and roll from a given normal vector.
 *
 *  Stolen from the tuxkart sources.
 *
 *  \param hpr  destination vector
 *  \param nrm  normal vector
 */
inline void pr_from_normal ( sgVec3 hpr, sgVec3 nrm )
{
  float sy = sin ( -hpr [ 0 ] * SG_DEGREES_TO_RADIANS ) ;
  float cy = cos ( -hpr [ 0 ] * SG_DEGREES_TO_RADIANS ) ;

  hpr[2] =  SG_RADIANS_TO_DEGREES * atan2 ( nrm[0] * cy - nrm[1] * sy, nrm[2] ) ;
  hpr[1] = -SG_RADIANS_TO_DEGREES * atan2 ( nrm[1] * cy + nrm[0] * sy, nrm[2] ) ;
}


/** \brief Draw the airplane shadow
 *
 *  This method actually does not draw anything. It only
 *  updates the aircraft shadow's transformation node in the
 *  scenegraph with the aircraft shadow's current position and
 *  orientation. The actual drawing is done by the global
 *  ssgCullAndDraw() call.
 *
 *  \todo calculate proper shadow rotation
 *  \todo adjust shadow location to match real terrain height
 *
 *  \param airplane pointer to the airplane's FDM object
 */
void CRRCAirplaneLaRCSimSSG::draw_shadow(FDMBase* airplane,
                                         float    shadow_matrix[4][4])
{
  CRRCMath::Vector3 pos = airplane->getPos();

  sgMat4 m;

  model_trans->getTransform(m);
  
  sgPostMultMat4(m, shadow_matrix);
//jwtodoshadow  m[3][1] += 0.1;
  
  shadow_trans->setTransform(m);
}
