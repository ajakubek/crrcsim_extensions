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
/** \file gear.cpp
 *
 *  Definitions of class methods for hardpoint/wheel management
 */

#include "gear.h"
#include <stdexcept>
#include "../../mod_misc/ls_constants.h"
#include "../xmlmodelfile.h"
#include "../../crrc_animation.h"


/**
 * Schnittstellen zu CRRC_AirplaneSim_Larcsim:
 *
 * CRRCMath::Matrix33 LocalToBody           (Transformation matrix local to body)
 * CRRCMath::Vector3  v_P_CG_Rwy            (CG relative to runway, in rwy coordinates N/E/D)
 * CRRCMath::Vector3  v_R_omega_body        (Angular body rates)
 * CRRCMath::Vector3  v_V_local_rel_ground  (V rel w.r.t. earth surface)
 * SCALAR             euler_angles_v[2]     (Psi)
 */
void Wheel::update( TSimInputs* inputs,
                    FDMEnviroment* env,
                    CRRCMath::Matrix33 LocalToBody,
                    CRRCMath::Vector3  const& v_P_CG_Rwy,
                    CRRCMath::Vector3  const& v_R_omega_body,
                    CRRCMath::Vector3  const& v_V_local_rel_ground,
                    SCALAR psi)
{
  /*
   * Constants & coefficients for tyres on tarmac - ref [1]
   */

  /* skid function looks like:
   *
   *           mu  ^
   *               |
   *       max_mu  |       +
   *               |      /|
   *   sliding_mu  |     / +------
   *               |    /
   *               |   /
   *               +--+------------------------>
   *               |  |    |      sideward V
   *               0 bkout skid
   *                V     V
   */

  const SCALAR sliding_mu   = 0.55;
  const SCALAR rolling_mu   = 0.01;
  const SCALAR max_brake_mu = 0.6;
  const SCALAR max_mu       = 0.8;
  const SCALAR bkout_v      = 0.1;
  const SCALAR skid_v       = 1.0;

  /*
   * Local data variables
   */

  SCALAR reaction_normal_force;   /* wheel normal (to rwy) force */
  SCALAR cos_wheel_hdg_angle, sin_wheel_hdg_angle;  /* temp storage */
  SCALAR steering_angle_rad;
  SCALAR v_wheel_forward, v_wheel_sideward,  abs_v_wheel_sideward;
  SCALAR forward_mu, sideward_mu; /* friction coefficients */
  SCALAR beta_mu;                 /* breakout friction slope */
  SCALAR forward_wheel_force, sideward_wheel_force;

  CRRCMath::Vector3 temp3a, temp3b;
  CRRCMath::Vector3 v_V_wheel_local;
  CRRCMath::Vector3 v_F_wheel_local;
  
  /* wheel offset from cg,  X-Y-Z */
  CRRCMath::Vector3 v_P_wheel_cg_body;
  
  /* wheel offset from cg,  N-E-D */
  CRRCMath::Vector3 v_P_wheel_cg_local;
  
  /* wheel offset from rwy, N-E-U */
  CRRCMath::Vector3 v_P_wheel_rwy_local;
  

  beta_mu = max_mu/(skid_v-bkout_v);
  
  /*========================================*/
  /* Calculate wheel position w.r.t. runway */
  /*========================================*/

  /* First calculate wheel location w.r.t. cg in body (X-Y-Z) axes... */

  v_P_wheel_cg_body = v_P;

  if (animation != NULL)
  {
    animation->transformPoint(v_P_wheel_cg_body);
  }

  /* then converting to local (North-East-Down) axes... */

  v_P_wheel_cg_local = LocalToBody.multrans(v_P_wheel_cg_body);
  
  /* Add wheel offset to cg location in local axes */

  v_P_wheel_rwy_local = v_P_wheel_cg_local + v_P_CG_Rwy;

  /*============================*/
  /* Calculate wheel velocities */
  /*============================*/

  /* contribution due to angular rates */

  temp3a = v_R_omega_body * v_P_wheel_cg_body;

  /* transform into local axes */

  temp3b = LocalToBody.multrans(temp3a);

  /* plus contribution due to cg velocities */

  v_V_wheel_local = temp3b + v_V_local_rel_ground;

  /*===========================================*/
  /* Calculate forces & moments for this wheel */
  /*===========================================*/

  /* Steering angle */
  /* First, determine the control input for this wheel */
  steering_angle_rad = inputs->GetInput(steering_mapping);
  /* Then calculate the real angle. Full control input (+-0.5)
     shall result in full wheel deflection  */
  steering_angle_rad = 2 * steering_angle_rad * steering_max_angle;

  /* Calculate sideward and forward velocities of the wheel
   in the runway plane     */
  SCALAR tmp_angle = caster_angle_rad + steering_angle_rad + psi;
  cos_wheel_hdg_angle = cos(tmp_angle);
  sin_wheel_hdg_angle = sin(tmp_angle);

  v_wheel_forward  = v_V_wheel_local.r[0]*cos_wheel_hdg_angle
                   + v_V_wheel_local.r[1]*sin_wheel_hdg_angle;
  v_wheel_sideward = v_V_wheel_local.r[1]*cos_wheel_hdg_angle
                   - v_V_wheel_local.r[0]*sin_wheel_hdg_angle;    

  /* Calculate normal load force (simple spring constant) */

  reaction_normal_force = 0.;

  SCALAR z_earth = -1*env->GetSceneryHeight(v_P_wheel_rwy_local.r[0], v_P_wheel_rwy_local.r[1]);
  
  if( v_P_wheel_rwy_local.r[2] > z_earth)
  {
    // Forces are in lbf here, lengths in ft, velocities in ft/s. 
    // So:
    // 1 slug * 1 ft / s^2 = spring_constant * 1 ft - 1 ft/s  * spring_damping
    // spring_constant  = slug / s^2 = lbf / ft
    // spring_damping   = slug / s   = lbf * s / ft
    reaction_normal_force = spring_constant*(z_earth-v_P_wheel_rwy_local.r[2])
                           - v_V_wheel_local.r[2]*spring_damping;
  }

  /* Calculate friction coefficients */

  forward_mu = (max_brake_mu - rolling_mu) * percent_brake + rolling_mu;
  abs_v_wheel_sideward = sqrt(v_wheel_sideward*v_wheel_sideward);
  sideward_mu = sliding_mu;
  if (abs_v_wheel_sideward < skid_v)
    sideward_mu = (abs_v_wheel_sideward - bkout_v)*beta_mu;
  if (abs_v_wheel_sideward < bkout_v)
    sideward_mu = 0.;

  /* Calculate foreward and sideward reaction forces */

  forward_wheel_force  =   forward_mu*reaction_normal_force;
  sideward_wheel_force =  sideward_mu*reaction_normal_force;
  if(v_wheel_forward < 0.)
    forward_wheel_force = -forward_wheel_force;
  if(v_wheel_sideward < 0.)
    sideward_wheel_force = -sideward_wheel_force;

  /* Rotate into local (N-E-D) axes */

  v_F_wheel_local.r[0] = forward_wheel_force *cos_wheel_hdg_angle
                       - sideward_wheel_force*sin_wheel_hdg_angle;
  v_F_wheel_local.r[1] = forward_wheel_force *sin_wheel_hdg_angle
                       + sideward_wheel_force*cos_wheel_hdg_angle;
  v_F_wheel_local.r[2] = reaction_normal_force;

  /* Convert reaction force from local (N-E-D) axes to body (X-Y-Z) */

  tempF = LocalToBody * v_F_wheel_local;

  /* Calculate moments from force and offsets in body axes */

  tempM = v_P_wheel_cg_body * tempF;

}


/**
 * Calculate the sum of forces and moments resulting from
 * the hardpoints interacting with the environment.
 *
 * \param inputs          Pointer to controller input class
 * \param env             Pointer to FDM's environment interface
 * \param LocalToBody     Transformation matrix from local to body coordinates
 * \param v_P_CG_Rwy      Position of CG in runway coordinates
 * \param v_R_omega_body  Angular rates in body coordinates
 * \param v_V_local_rel_ground  Velocity relative to ground in local coordinates
 */
void WheelSystem::update( TSimInputs* inputs,
                          FDMEnviroment* env,
                          CRRCMath::Matrix33 LocalToBody,
                          CRRCMath::Vector3  const& v_P_CG_Rwy,
                          CRRCMath::Vector3  const& v_R_omega_body,
                          CRRCMath::Vector3  const& v_V_local_rel_ground,
                          SCALAR psi)
{
  int i;                        /* per wheel loop counter */
  int num_wheels = wheels.size();

  /*
   * Execution starts here
   */

  v_Forces  = CRRCMath::Vector3();  /* Initialize sum of forces... */
  v_Moments = CRRCMath::Vector3();  /* ...and moments  */
      
  for (i=0;i<num_wheels;i++)     /* Loop for each wheel */
  {
    wheels[i].update( inputs,
                      env,
                      LocalToBody,
                      v_P_CG_Rwy,
                      v_R_omega_body,
                      v_V_local_rel_ground,
                      psi);

    /* Sum forces and moments across all wheels */
    v_Forces  += wheels[i].tempF;
    v_Moments += wheels[i].tempM;
  }
}

/**
 * Create a WheelSystem with no hardpoints
 */
WheelSystem::WheelSystem()
: v_Forces(0, 0, 0), v_Moments(0,0,0), dMaxSize(0), span_ft(0), dZLow(0) 
{
}

/**
 * Initialize a WheelSystem from an XML file.
 *
 * \param ModelFile   pointer to file class
 * \param def_span    default span if no hardpoints are found to calculate it
 */
void WheelSystem::init(SimpleXMLTransfer *ModelFile, SCALAR  def_span)
{
  Wheel              wheel;
  SimpleXMLTransfer  *e, *i;
  unsigned int       uSize;
  double             x, y, z;
  double             dist;
  double             to_ft;
  double             to_lbf_per_ft;
  double             to_lbf_s_per_ft;
  CRRCMath::Vector3  pCG;
   
  /**
   * Tracks wingspan [m]
   */
  double span = 0;
  span_ft = 0.0;
  
  
  //
  pCG = CRRCMath::Vector3(0, 0, 0);
  if (ModelFile->indexOfChild("CG") >= 0)
  {
    i = ModelFile->getChild("CG");
    pCG.r[0] = i->attributeAsDouble("x", 0);
    pCG.r[1] = i->attributeAsDouble("y", 0);
    pCG.r[2] = i->attributeAsDouble("z", 0);
    
    if (i->attributeAsInt("units") == 1)
      pCG *= M_TO_FT;
  }
  
  // let's assume that there is nothing below the CG:
  dZLow    = 0;
  
  // let's assume that there is nothing distant from the CG:
  dMaxSize = 0;
  
  wheels.clear();
  
  i = ModelFile->getChild("wheels");
  switch (i->getInt("units"))
  {
   case 0:    
    to_ft           = 1;
    to_lbf_per_ft   = 1;
    to_lbf_s_per_ft = 1;
    break;
   case 1:
    to_ft           = M_TO_FT;
    to_lbf_per_ft   = FT_TO_M / LBF_TO_N;
    to_lbf_s_per_ft = FT_TO_M / LBF_TO_N;
    break;
   default:
    {
      throw std::runtime_error("Unknown units in wheels");
    }
    break;
  }        
  uSize = i->getChildCount();
  for (unsigned int n=0; n<uSize; n++)
  {
    e = i->getChildAt(n);
    
    x = e->getDouble("pos.x") * to_ft - pCG.r[0];
    y = e->getDouble("pos.y") * to_ft - pCG.r[1];
    z = e->getDouble("pos.z") * to_ft - pCG.r[2];
    wheel.v_P = CRRCMath::Vector3(x, y, z);
    wheel.anim_name = e->getString("pos.animation", "");
    wheel.animation = NULL;

    wheel.spring_constant    = e->getDouble("spring.constant") * to_lbf_per_ft;
    wheel.spring_damping     = e->getDouble("spring.damping")  * to_lbf_s_per_ft;
    wheel.percent_brake      = e->getDouble("percent_brake");
    wheel.caster_angle_rad   = e->getDouble("caster_angle_rad");

    if (e->indexOfChild("steering") >= 0)
    {
      std::string s = e->getString("steering.mapping", "NOTHING");
      wheel.steering_max_angle = e->getDouble("steering.max_angle", 1.0);
      wheel.steering_mapping   = XMLModelFile::GetSteering(s);
    }
    else
    {
      wheel.steering_mapping   = TSimInputs::smNOTHING;
      wheel.steering_max_angle = 0;
    }
    wheels.push_back(wheel);
    
    // track wingspan
    if (span < y)
      span = y;
    // lowest point?
    if (dZLow < z)
      dZLow = z;
    // far away (Z distance is assumed to be low)?
    dist = x*x + y*y;
    if (dist > dMaxSize)
      dMaxSize = dist;
  }
  dMaxSize = sqrt(dMaxSize);
  span_ft  = 2 * span;

  // just in case: if there were no hardpoints, use the reference span
  if (span_ft == 0.0)
  {
    span_ft = def_span;
  }
}


void WheelSystem::registerAnimations(std::vector<CRRCAnimation*> const& animations)
{
  std::vector<CRRCAnimation*>::const_iterator anim_it;
  std::vector<Wheel>::iterator wheel_it;

  for (anim_it = animations.begin(); anim_it != animations.end(); anim_it++)
  {
    for (wheel_it = wheels.begin(); wheel_it != wheels.end(); wheel_it++)
    {
      if ((*wheel_it).anim_name == (*anim_it)->getSymbolicName())
      {
        std::cout << "Coupling hardpoint to animation " << (*anim_it)->getSymbolicName() << std::endl;
        (*wheel_it).animation = *anim_it;
        break;
      }
    }
  }
}


