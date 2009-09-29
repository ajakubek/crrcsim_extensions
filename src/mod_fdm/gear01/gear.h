/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2008 - Jan Reucker (refactoring, see note below)
 *
 * This file is partially based on work by
 *   Jan Kansky
 *   Jens Wilhelm Wulf
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
/** \file gear.h
 *
 *  Declaration of classes for hardpoint/wheel management
 */

#ifndef FDM_WHEELS_H
#define FDM_WHEELS_H

# include <vector>
# include "../ls_types.h"
# include "../fdm_inputs.h"
# include "../fdm_env.h"
# include "../../mod_math/vector3.h"
# include "../../mod_math/matrix33.h"
# include "../../mod_misc/SimpleXMLTransfer.h"
//# include "../../mod_misc/lib_conversions.h"

class CRRCAnimation;
class WheelSystem;

/**
* This class holds information about a single hard point/wheel
* on the airplane.
*/
class Wheel
{
  friend class WheelSystem;

  public:
    void update(TSimInputs*         inputs,
                FDMEnviroment*      env,
                CRRCMath::Matrix33  LocalToBody,
                CRRCMath::Vector3   const& v_P_CG_Rwy,
                CRRCMath::Vector3   const& v_R_omega_body,
                CRRCMath::Vector3   const& v_V_local_rel_ground,
                SCALAR psi);
    CRRCMath::Vector3 tempF, tempM;
 
  private:
    /**
     * body axes: x,y,z
     */
    CRRCMath::Vector3 v_P;

    std::string   anim_name;
    CRRCAnimation *animation;

    double spring_constant;
    double spring_damping;

    TSimInputs::eSteeringMap steering_mapping;   ///<  Indicates which RC channel controls the steering
    double steering_max_angle; ///<  Indicates maximum angle of steering wheel
    double percent_brake;
    double caster_angle_rad;   ///<  Alignment of the wheel
};


/**
 * This class represents a system of hardpoints (wheels).
 */
class WheelSystem
{
  public:
    WheelSystem();
    void init(SimpleXMLTransfer *ModelFile, SCALAR def_span);

    void update(TSimInputs* inputs,
                FDMEnviroment* env,
                CRRCMath::Matrix33 LocalToBody,
                CRRCMath::Vector3  const& v_P_CG_Rwy,
                CRRCMath::Vector3  const& v_R_omega_body,
                CRRCMath::Vector3  const& v_V_local_rel_ground,
                SCALAR psi);
    CRRCMath::Vector3 getForces() const {return v_Forces;};
    CRRCMath::Vector3 getMoments() const {return v_Moments;};

   /**
    * the longest distance from any of the aircrafts points to the CG
    */
    double getAircraftSize() const { return(dMaxSize); };

   /**
    * Wingspan of the aircraft in feet
    */
    double getWingspan() const { return(span_ft); };
   
   /**
    * returns Z coordinate of lowest point
    */
    double getZLow() const { return(dZLow); };

   /**
    * browse a list of animations and attach them to movable hardpoints
    */
    void registerAnimations(std::vector<CRRCAnimation*> const& animations);

  private:
    std::vector<Wheel>  wheels;
    CRRCMath::Vector3   v_Forces;
    CRRCMath::Vector3   v_Moments;

  /**
    * Longest distance of any of the airplanes points to the cg
    */
    double dMaxSize;

   /**
    * Wingspan in feet (calculated from hardpoints)
    */
   double span_ft;

   /**
    * Z coordinate of lowest point
    */
   double dZLow;
   
};





#endif  // FDM_WHEELS_H
