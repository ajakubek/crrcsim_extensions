/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2006, 2007, 2008 Jan Reucker (original author)
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

#ifndef CRRC_LOADAIR_H
# define CRRC_LOADAIR_H

# include <string>
# include <vector>
# include <stdexcept>

# include <plib/ssg.h>

# include "global.h"
# include "include_gl.h"
# include "mod_fdm/fdm.h"
# include "crrc_sound.h"
# include "crrc_animation.h"
# include "mod_misc/SimpleXMLTransfer.h"



/** \brief A very basic airplane description class.
 *
 *  This is a first attempt to organize all airplane-related
 *  stuff into a class. At this time it's nothing more than
 *  a wrapper for the existing functions.
 * 
 *  Update: this class should contain everything but the simulation of
 *  the EOM (all the physics stuff). So this is about visualization.
 */
class CRRCAirplane
{
  public:
   
   /** \brief The constructor.
    *
    *  
    */
   CRRCAirplane();
   
   /** \brief The destructor.
    *
    *  Clean up.
    */
   virtual ~CRRCAirplane();
   
   /** \brief Draw the precompiled airplane shadow.
    *
    *  Applies some material properties and calls the
    *  shadow's display list.
    */
   virtual void draw_shadow(FDMBase* airplane,
                            float    shadow_matrix[4][4]) = 0;
   
   /** \brief Draw the airplane
    *
    *
    */
   virtual void draw(FDMBase* airplane) = 0;

   virtual int  getNumSounds()  {return (sound.size());};
   //~ virtual int  getSoundType(int soundnum)  {return sound[soundnum]->type;};
   //~ virtual double getSoundPitchFactor(int soundnum) { return(sound[soundnum]->dPitchFactor); };
  
   virtual CRRCAnimation* findAnimation(std::string anim_name);

   virtual std::vector<CRRCAnimation*> const& getAnimations() {return animations;}

  protected:
   std::vector<T_AirplaneSound*> sound;
   std::vector<CRRCAnimation*> animations;
};


/**
 * Reads geometric data directly from old-style .air-files.
 */
class CRRCAirplaneLaRCSim : public CRRCAirplane
{
  public:
   CRRCAirplaneLaRCSim();
   CRRCAirplaneLaRCSim(SimpleXMLTransfer* xml);
   virtual ~CRRCAirplaneLaRCSim();

  protected:
   
  /** \brief Initialize the airplane's sound.
    *
    *  Reads all sound related parameters from an xml description.
    *  \todo Make this method search all possible paths on Linux!
    */
   virtual void  initSound(SimpleXMLTransfer* xml);

   float max_thrust;
  
};


/** \brief Display airplane model using SSG
 *
 *  This airplane class also uses old-style .air-files,
 *  but does not use any of the geometry descriptions in
 *  the file. Instead it loads the model specified by
 *  the "model" parameter in the file.
 */
class CRRCAirplaneLaRCSimSSG : public CRRCAirplaneLaRCSim
{
  public:
    //CRRCAirplaneLaRCSimSSG(const char*        filename, ssgBranch* graph);
    CRRCAirplaneLaRCSimSSG(SimpleXMLTransfer* xml, ssgBranch* graph);
   
    virtual ~CRRCAirplaneLaRCSimSSG();
    
    virtual void draw(FDMBase* airplane);
    virtual void draw_shadow(FDMBase* airplane,
                             float    shadow_matrix[4][4]);

  
  private:
    ssgTransform  *initial_trans;
    ssgTransform  *model_trans;
    ssgEntity     *model;
    ssgEntity     *shadow;
    ssgTransform  *shadow_trans;
};


#endif // CRRC_LOADAIR_H
