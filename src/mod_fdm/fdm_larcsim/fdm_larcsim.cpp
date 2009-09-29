// -*- mode: c; mode: fold -*-
/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2006, 2008, 2009 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2006, 2007, 2008 - Jan Reucker
 *   Copyright (C) 2006 - Todd Templeton
 *
 * This file is partially based on work by
 *   Jan Kansky
 *   Bruce Jackson
 * The respective methods have a header containing more details, see below.
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
#include "fdm_larcsim.h"

#include <math.h>
#include <iostream>

#include "../../mod_misc/ls_constants.h"
#include "../ls_geodesy.h"
#include "../../mod_misc/SimpleXMLTransfer.h"
#include "../../mod_misc/lib_conversions.h"
#include "../xmlmodelfile.h"

// 0, 1, 2
#define EOM_TEST 0

/**
 * *****************************************************************************
 */

void CRRC_AirplaneSim_Larcsim::initAirplaneState(double dRelVel,
                                                 double dTheta,
                                                 double dPsi,
                                                 double X,
                                                 double Y,
                                                 double Z,
                                                 double R_X,
                                                 double R_Y,
                                                 double R_Z) 
{
  float flVelocity = dRelVel * getTrimmedFlightVelocity();

  Phi       = 0.0;          // bank/roll angle  [rad]
  Theta     = dTheta;       // pitch attitude angle [rad]
  Psi       = dPsi;         // heading angle [rad]

  Alpha     = 0;            // angle of attack            [rad]
  Beta      = 0;            // sideslip angle             [rad]

  {
    // see ls_aux(): 'determine location in runway coordinates'
    double slr, dummy;
    ls_geod_to_geoc( 0, 0, &slr, &dummy);

    Latitude  = X/slr;
    Longitude = Y/slr;
    Altitude  = -1*Z;
  }

  {
    // horizontal velocity
    float flVHor = flVelocity * cos(dTheta);
    // horizontal velocity has to be upwind:
    v_V_local.r[0]  = cos(Psi)*flVHor;      // local x-velocity (north)   [ft/s]
    v_V_local.r[1]  = sin(Psi)*flVHor;      // local y-velocity (east)    [ft/s]
    
    v_V_local_rel_ground.r[1] = v_V_local.r[1];
  }
  v_V_local.r[2]         = flVelocity * sin(-dTheta);          // local z-velocity (down)     [ft/s]
  
  v_R_omega_body    = CRRCMath::Vector3(R_X, R_Y, R_Z); // body rate   [rad/s]  
  v_V_dot_local     = CRRCMath::Vector3(); // local acceleration   [ft/s^2]
  v_V_local_airmass = CRRCMath::Vector3(); // local velocity of steady airmass   [ft/s]
  
  // jwtodo: seems this can be used to simulate wind gusts
  v_V_gust_local  = CRRCMath::Vector3();
  
  CD_stall        = 0.05;   // drag coeff. during stalling    []

  m_V_atmo_rwy = CRRCMath::Matrix33();
  
  ls_step_init();
  
  power->reset(v_V_wind_body * FT_TO_M);  
}


void CRRC_AirplaneSim_Larcsim::update(TSimInputs* inputs,
                                      double      dt,
                                      int         multiloop) 
{
  double V_north_xp,V_east_xp,V_down_xp;   // temp vars for calculating wind grad.
  double V_north_yp,V_east_yp,V_down_yp;
  double V_north_zp,V_east_zp,V_down_zp;
  double V_north_xm,V_east_xm,V_down_xm;
  double V_north_ym,V_east_ym,V_down_ym;
  double V_north_zm,V_east_zm,V_down_zm;

  /**
   * Using a length of about roughly one half of the aircrafts
   * size to calculate wind gradients. 0.1 foot had been used before,
   * which leads to very high or zero gradients.
   */
  double delta_space = getAircraftSize()/2;
  
  int   nAircraftOutsideWindfieldSim = 
    env->CalculateWind(v_P_CG_Rwy.r[0]+delta_space, v_P_CG_Rwy.r[1],             v_P_CG_Rwy.r[2],
                       V_north_xp,                  V_east_xp,                   V_down_xp) |
    env->CalculateWind(v_P_CG_Rwy.r[0],             v_P_CG_Rwy.r[1]+delta_space, v_P_CG_Rwy.r[2],
                       V_north_yp,                  V_east_yp,                   V_down_yp) |
    env->CalculateWind(v_P_CG_Rwy.r[0],             v_P_CG_Rwy.r[1],             v_P_CG_Rwy.r[2]+delta_space,
                       V_north_zp,                  V_east_zp,                   V_down_zp) |
    env->CalculateWind(v_P_CG_Rwy.r[0]-delta_space, v_P_CG_Rwy.r[1],             v_P_CG_Rwy.r[2],
                       V_north_xm,                  V_east_xm,                   V_down_xm) |
    env->CalculateWind(v_P_CG_Rwy.r[0],             v_P_CG_Rwy.r[1]-delta_space, v_P_CG_Rwy.r[2],
                       V_north_ym,                  V_east_ym,                   V_down_ym) |
    env->CalculateWind(v_P_CG_Rwy.r[0],             v_P_CG_Rwy.r[1],             v_P_CG_Rwy.r[2]-delta_space,
                       V_north_zm,                  V_east_zm,                   V_down_zm) |
    env->CalculateWind(v_P_CG_Rwy.r[0],             v_P_CG_Rwy.r[1],             v_P_CG_Rwy.r[2],
                       v_V_local_airmass.r[0],      v_V_local_airmass.r[1],      v_V_local_airmass.r[2]);  

  if (nAircraftOutsideWindfieldSim)
  {
    // todo: some error message?
  }
  
  // Gradients are calculated from symmetric pairs to get symmetric behaviour.
  m_V_atmo_rwy.v[0][0] = (V_north_xp - V_north_xm)/(2*delta_space);
  m_V_atmo_rwy.v[0][1] = (V_north_yp - V_north_ym)/(2*delta_space);
  m_V_atmo_rwy.v[0][2] = (V_north_zp - V_north_zm)/(2*delta_space);
  m_V_atmo_rwy.v[1][0] = (V_east_xp  - V_east_xm) /(2*delta_space);
  m_V_atmo_rwy.v[1][1] = (V_east_yp  - V_east_ym) /(2*delta_space);
  m_V_atmo_rwy.v[1][2] = (V_east_zp  - V_east_zm) /(2*delta_space);
  m_V_atmo_rwy.v[2][0] = (V_down_xp  - V_down_xm) /(2*delta_space);
  m_V_atmo_rwy.v[2][1] = (V_down_yp  - V_down_ym) /(2*delta_space);
  m_V_atmo_rwy.v[2][2] = (V_down_zp  - V_down_zm) /(2*delta_space);

  for (int n=0; n<multiloop; n++)
  {
    logNewline();
    
#if FDM_LOG_POS != 0
    logVal(v_P_CG_Rwy.r[0]);
    logVal(v_P_CG_Rwy.r[1]);
    logVal(v_P_CG_Rwy.r[2]);
    logVal(getPhi());    
    logVal(getTheta());
    logVal(getPsi());    
#endif
#if FDM_LOG_WIND_IN != 0
    logVal(v_V_local_airmass);
    logVal(m_V_atmo_rwy);
#endif
            
    ls_step( dt );
    ls_aux();

    env->ControllerCallback(dt, this, inputs, &myInputs);
    
    aero( &myInputs );
    
#if FDM_LOG_AERO_OUT != 0
    logVal(v_F_aero);
    logVal(v_M_aero);
#endif
    
    engine( dt, &myInputs );
    gear( &myInputs );

    ls_accel();
  }
}


CRRC_AirplaneSim_Larcsim::CRRC_AirplaneSim_Larcsim(const char* filename, FDMEnviroment* myEnv, SimpleXMLTransfer* cfg) : FDMBase("fdm_larcsim.dat", myEnv)
{
  // Previously there has been code to load an old-style .air-file. 
  // This has been removed as CRRCSim includes an automatic converter.
  SimpleXMLTransfer* fileinmemory = new SimpleXMLTransfer(filename);

  LoadFromXML(fileinmemory, cfg->getInt("airplane.verbosity", 5));
  
  delete fileinmemory;
}


CRRC_AirplaneSim_Larcsim::CRRC_AirplaneSim_Larcsim(SimpleXMLTransfer* xml, FDMEnviroment* myEnv, SimpleXMLTransfer* cfg) : FDMBase("fdm_larcsim.dat", myEnv)
{
  LoadFromXML(xml, cfg->getInt("airplane.verbosity", 5));
}

void CRRC_AirplaneSim_Larcsim::LoadFromXML(SimpleXMLTransfer* xml, int nVerbosity)
{
  SimpleXMLTransfer* i;
  SimpleXMLTransfer* cfg = XMLModelFile::getConfig(xml);
  
  {
    double to_ft;
    
    switch (xml->getInt("aero.units"))
    {
     case 0:
      to_ft = 1;
      break;
     case 1:
      to_ft = M_TO_FT;
      break;
     default:
      {
        throw std::runtime_error("Unknown units in aero");
      }
      break;
    }
    i = xml->getChild("aero.ref");
    C_ref = i->getDouble("chord") * to_ft;
    B_ref = i->getDouble("span") * to_ft;
    S_ref = i->getDouble("area") * to_ft * to_ft;
    U_ref = i->getDouble("speed") * to_ft;
  }
    
  i = xml->getChild("aero.misc");
  Alpha_0  = i->getDouble("Alpha_0");
  eta_loc  = i->getDouble("eta_loc");
  CG_arm   = i->getDouble("CG_arm");
  span_eff = i->getDouble("span_eff");
  
  i = xml->getChild("aero.m");
  Cm_0  = i->getDouble("Cm_0");
  Cm_a  = i->getDouble("Cm_a");
  Cm_q  = i->getDouble("Cm_q");
  Cm_de = i->getDouble("Cm_de");
  
  i = xml->getChild("aero.lift");
  CL_0    = i->getDouble("CL_0");
  CL_max  = i->getDouble("CL_max");
  CL_min  = i->getDouble("CL_min");
  CL_a    = i->getDouble("CL_a");
  CL_q    = i->getDouble("CL_q");
  CL_de   = i->getDouble("CL_de");
  CL_drop = i->getDouble("CL_drop");
  CL_CD0  = i->getDouble("CL_CD0");
  
  i = xml->getChild("aero.drag");
  CD_prof  = i->getDouble("CD_prof");
  Uexp_CD  = i->getDouble("Uexp_CD");
  CD_stall = i->getDouble("CD_stall");
  CD_CLsq  = i->getDouble("CD_CLsq");
  CD_AIsq  = i->getDouble("CD_AIsq");
  CD_ELsq  = i->getDouble("CD_ELsq");
  
  i = xml->getChild("aero.Y");
  CY_b  = i->getDouble("CY_b");
  CY_p  = i->getDouble("CY_p");
  CY_r  = i->getDouble("CY_r");
  CY_dr = i->getDouble("CY_dr");
  CY_da = i->getDouble("CY_da");
  
  i = xml->getChild("aero.l");
  Cl_b  = i->getDouble("Cl_b");
  Cl_p  = i->getDouble("Cl_p");
  Cl_r  = i->getDouble("Cl_r");
  Cl_dr = i->getDouble("Cl_dr");
  Cl_da = i->getDouble("Cl_da");
  
  i = xml->getChild("aero.n");
  Cn_b  = i->getDouble("Cn_b");
  Cn_p  = i->getDouble("Cn_p");
  Cn_r  = i->getDouble("Cn_r");
  Cn_dr = i->getDouble("Cn_dr");
  Cn_da = i->getDouble("Cn_da");
    
  flaps_drag = i->getDouble("aero.flaps.drag", 0);
  flaps_lift = i->getDouble("aero.flaps.lift", 0);
  flaps_off  = i->getDouble("aero.flaps.off",  0);
  
  spoiler_drag = i->getDouble("aero.spoiler.drag", 0);
  spoiler_lift = i->getDouble("aero.spoiler.lift", 0);
  
  {
    double to_slug;
    double to_slug_ft_ft;
    
    i = cfg->getChild("mass_inertia");
    switch (i->getInt("units"))
    {
     case 0:    
      to_slug       = 1;
      to_slug_ft_ft = 1;
      break;
     case 1:
      to_slug       = KG_TO_SLUG;
      to_slug_ft_ft = KG_M_M_TO_SLUG_FT_FT;
      break;
     default:
      {
        throw std::runtime_error("Unknown units in mass_inertia");
      }
      break;
    }
    Mass  = i->getDouble("Mass") * to_slug;
    I_xx  = i->getDouble("I_xx") * to_slug_ft_ft;
    I_yy  = i->getDouble("I_yy") * to_slug_ft_ft;
    I_zz  = i->getDouble("I_zz") * to_slug_ft_ft;
    I_xz  = i->getDouble("I_xz") * to_slug_ft_ft;
  }
  
  wheels.init(xml, B_ref);
  double span = wheels.getWingspan() * FT_TO_M;

  // calculate velocity in trimmed flight
  {
    // crrc_aero says: Cm = Cm_0 + Cm_a*(Alpha-Alpha_0)
    // So Cm is zero at
    float alpha = ((Cm_a * Alpha_0) - Cm_0 ) / Cm_a;
    // crrc_aero says: CL  = CL_0 + CL_a*(Alpha - Alpha_0);
    float cl = CL_0 + CL_a * (alpha - Alpha_0);
    
    // sanity check: currently this method does not really work for biplane2.air, as
    // a negative cl is computed. ???
    if (cl < 0.2)
    {
      cl = 0.2;
      printf("airplane: Cm_a=%f   Alpha_0=%f  Cm_0=%f\n", Cm_a, Alpha_0, Cm_0);
      printf("airplane: CL_a=%f               CL_0=%f\n", CL_a,          CL_0);
      printf("airplane: alpha=%f  C_L=%f\n", alpha, cl);
    }
    
    // v = sqrt(m * g * 2 / (F * CL * rho))
    // I'll use conversions to SI units, just to understand the numbers...
    const float foot = 0.3048;               // m
    const float slug = 14.5939041995;        // kg
    float m    = Mass * slug;                // kg
    float g    = 9.81;                       // m/s^2
    float F    = S_ref * (foot*foot);        // m^2
    float rho  = 1.225;                      // kg/m^3
    float v = sqrt(m * g * 2 / (F * cl * rho));
    trimmedFlightVelocity = v * M_TO_FT; // ft/s
  }
  
  // possibly there is no power description:
  if (cfg->indexOfChild("power") < 0)
  {
    power = new Power::Power();
  }
  else
  {
    power = new Power::Power(cfg, nVerbosity);
  }

  if (nVerbosity > 1)
  {
    std::cout << "--- Airplane description: ---------------------------------------\n";
    std::cout << "  Wingspan:                   " << span              << " m\n";
    std::cout << "  Mass:                       " << (Mass*SLUG_TO_KG) << " kg\n";
    std::cout << "  Velocity in trimmed flight: " << (trimmedFlightVelocity*FT_TO_M) << " m/s\n";
    std::cout << "-----------------------------------------------------------------\n";
  }
}


CRRC_AirplaneSim_Larcsim::~CRRC_AirplaneSim_Larcsim()
{
  delete power;
}

double CRRC_AirplaneSim_Larcsim::getPhi()
{
  return(euler_angles_v[0]);
}

double CRRC_AirplaneSim_Larcsim::getTheta()
{
  return(euler_angles_v[1]);
}

double CRRC_AirplaneSim_Larcsim::getPsi()
{
  return(euler_angles_v[2]);
}

CRRCMath::Vector3 CRRC_AirplaneSim_Larcsim::getPos()
{
  return(v_P_CG_Rwy);
}

CRRCMath::Vector3 CRRC_AirplaneSim_Larcsim::getVel()
{
  return(v_V_local_rel_ground);
}

CRRCMath::Vector3 CRRC_AirplaneSim_Larcsim::getAccel()
{
  return(v_V_dot_local);
}

CRRCMath::Vector3 CRRC_AirplaneSim_Larcsim::getPQR()
{
  return(v_R_omega_body);
}

double CRRC_AirplaneSim_Larcsim::getLat()
{
  return(Latitude);
}

double CRRC_AirplaneSim_Larcsim::getLon()
{
  return(Longitude);
}

double CRRC_AirplaneSim_Larcsim::getAlt()
{
  return(Altitude);
}

/** \brief Calculate influence of gear and hardpoints
 *
 *  This method calculates the forces and moments on
 *  the airplane's body caused by the gear or other
 *  hardpoints touching the ground.
 *
 *  \param inputs   Current control inputs
 */
void CRRC_AirplaneSim_Larcsim::gear(TSimInputs* inputs)      
{
  wheels.update(inputs,
                env,
                LocalToBody,
                v_P_CG_Rwy,
                v_R_omega_body,
                v_V_local_rel_ground,
                Psi);

  v_F_gear = wheels.getForces();
  v_M_gear = wheels.getMoments();
}


void CRRC_AirplaneSim_Larcsim::engine( SCALAR dt, TSimInputs* inputs) 
{
  v_F_engine = CRRCMath::Vector3();
  v_M_engine = CRRCMath::Vector3();
  
  inputs->pitch = 1;
  power->step(dt, inputs, v_V_wind_body.r[0]*FT_TO_M, &v_F_engine, &v_M_engine);

  // Convert SI to that other buggy system.
  v_F_engine *= N_TO_LBF;
  v_M_engine *= NM_TO_LBFFT;
}


/**
 * This method is based on crrc_aero.c in the initial version of CRRCSim. 
 * It did not contain a copyright or author note and was committed to CVS 
 * by Jan Kansky.
 * 
 * Calculate forces and moments for the current time step.
 */
void CRRC_AirplaneSim_Larcsim::aero(TSimInputs* inputs)    
{
  SCALAR elevator, aileron, rudder, flaps;

  SCALAR Phat, Qhat, Rhat;  // dimensionless rotation rates
  SCALAR CL_left, CL_cent, CL_right; // CL values across the span
  SCALAR dCL_left,dCL_cent,dCL_right; // stall-induced CL changes
  SCALAR dCD_left,dCD_cent,dCD_right; // stall-induced CD changes
  SCALAR dCl,dCn,dCl_stall,dCn_stall;  // stall-induced roll,yaw moments
  SCALAR dCm_stall;  // Stall induced pitching moment.
  SCALAR CL_wing, CD_all, CD_scaled, Cl_w;
  SCALAR Cl_r_mod,Cn_p_mod;
  SCALAR CL,CD,Cl,Cm,Cn;
  SCALAR QS;
  
  CRRCMath::Vector3 C_xyz;

  CRRCMath::Vector3 v_R_omega_atmo;
  
  CRRCMath::Matrix33 G;
  CRRCMath::Matrix33 m_V_body;

  SCALAR    Cos_alpha, Sin_alpha, Cos_beta;

  Cos_alpha = cos(Alpha);
  Sin_alpha = sin(Alpha);
  Cos_beta  = cos(Beta);

  elevator = inputs->elevator;
  aileron  = inputs->aileron;
  rudder   = inputs->rudder;
  flaps    = inputs->flap + flaps_off;

  /* compute gradients of Local velocities w.r.t. Body coordinates
      G_11  =  dU_local/dx_body
      G_12  =  dU_local/dy_body   etc.  */
    
  G = m_V_atmo_rwy * LocalToBody.trans();

  /* now compute gradients of Body velocities w.r.t. Body coordinates */
  /*  U_body_x  =  dU_body/dx_body   etc.  */

  m_V_body = LocalToBody * G;
  
  /* set rotation rates of airmass motion */
  v_R_omega_atmo.r[0] =  m_V_body.v[2][0];
  v_R_omega_atmo.r[1] = -m_V_body.v[2][1];
  v_R_omega_atmo.r[2] =  m_V_body.v[1][2];

  if (V_rel_wind != 0)
  {
    /* set net effective dimensionless rotation rates */
    // jww: the comment above is misleading. The unit of those values must be rad!
    Phat = (v_R_omega_body.r[0] - v_R_omega_atmo.r[0]) * B_ref / (2.0*V_rel_wind);
    Qhat = (v_R_omega_body.r[1] - v_R_omega_atmo.r[1]) * C_ref / (2.0*V_rel_wind);
    Rhat = (v_R_omega_body.r[2] - v_R_omega_atmo.r[2]) * B_ref / (2.0*V_rel_wind);
  }
  else
  {
    Phat=0;
    Qhat=0;
    Rhat=0;
  }

  /* compute local CL at three spanwise locations */
  {
    double lift_ = flaps_lift * flaps + spoiler_lift * inputs->spoiler;
    CL_left  = CL_0 + CL_a*(Alpha - Alpha_0 - Phat*eta_loc) + lift_;
    CL_cent  = CL_0 + CL_a*(Alpha - Alpha_0               ) + lift_;
    CL_right = CL_0 + CL_a*(Alpha - Alpha_0 + Phat*eta_loc) + lift_;
  }  

  /* set CL-limit changes */
  dCL_left  = 0.;
  dCL_cent  = 0.;
  dCL_right = 0.;

  stalling=0;
  {
    double cl_max_with_flaps = CL_max + flaps_lift * flaps;
    double cl_min_with_flaps = CL_min + flaps_lift * flaps;
    
    if (CL_left  > cl_max_with_flaps)
    {
      dCL_left  = cl_max_with_flaps-CL_left -CL_drop;
      stalling=1;
    }

    if (CL_cent  > cl_max_with_flaps)
    {
      dCL_cent  = cl_max_with_flaps-CL_cent -CL_drop;
      stalling=1;
    }
    
    if (CL_right > cl_max_with_flaps)
    {
      dCL_right = cl_max_with_flaps-CL_right -CL_drop;
      stalling=1;
    }
    
    if (CL_left  < cl_min_with_flaps)
    {
      dCL_left  = cl_min_with_flaps-CL_left -CL_drop;
      stalling=1;
    }
    
    if (CL_cent  < cl_min_with_flaps)
    {
      dCL_cent  = cl_min_with_flaps-CL_cent -CL_drop;
      stalling=1;
    }
    
    if (CL_right < cl_min_with_flaps)
    {
      dCL_right = cl_min_with_flaps-CL_right -CL_drop;
      stalling=1;
    }
  }
  
  /* set average wing CL */
  CL_wing = CL_0 + CL_a*(Alpha-Alpha_0)
    + 0.25*dCL_left + 0.5*dCL_cent + 0.25*dCL_right;

  /* correct profile CD for CL dependence and aileron dependence */
  CD_all = CD_prof
    + CD_CLsq*(CL_wing-CL_CD0)*(CL_wing-CL_CD0)
    + CD_AIsq*aileron*aileron
    + CD_ELsq*elevator*elevator
    + spoiler_drag * inputs->spoiler
    + flaps_drag   * flaps;
  
  /* scale profile CD with Reynolds number via simple power law */
  if (V_rel_wind > 0.1)
  {
    CD_scaled = CD_all*pow(((double)V_rel_wind/(double)U_ref),Uexp_CD);
  }
  else
  {
    CD_scaled=CD_all;
  }

  /* Scale lateral cross-coupling derivatives with wing CL */
  Cl_r_mod = Cl_r*CL_wing/CL_0;
  Cn_p_mod = Cn_p*CL_wing/CL_0;

  /* total average CD with induced and stall contributions */
  dCD_left  = CD_stall*dCL_left *dCL_left ;
  dCD_cent  = CD_stall*dCL_cent *dCL_cent ;
  dCD_right = CD_stall*dCL_right*dCL_right;

  /* total CL, with pitch rate and elevator contributions */
  CL = (CL_wing + CL_q*Qhat + CL_de*elevator)*Cos_alpha;

  /* assymetric stall will cause roll and yaw moments */
  dCl =  0.45*-1*(dCL_right-dCL_left)*0.5*eta_loc;
  dCn =  0.45*(dCD_right-dCD_left)*0.5*eta_loc;
  dCm_stall = (0.25*dCL_left + 0.5*dCL_cent + 0.25*dCL_right)*CG_arm;

  /* stall-caused moments in body axes */
  dCl_stall = dCl*Cos_alpha - dCn*Sin_alpha;
  dCn_stall = dCl*Sin_alpha + dCn*Cos_alpha;

  /* total CD, with induced and stall contributions */

  Cl_w = Cl_b*Beta  + Cl_p*Phat + Cl_r_mod*Rhat
    + dCl_stall  + Cl_da*aileron;
  CD = CD_scaled
    + (CL*CL + 32.0*Cl_w*Cl_w)*S_ref/(B_ref*B_ref*3.14159*span_eff)
      + 0.25*dCD_left + 0.5*dCD_cent + 0.25*dCD_right;

  /* total forces in body axes */
  C_xyz.r[0] = -CD*Cos_alpha + CL*Sin_alpha*Cos_beta*Cos_beta;
  C_xyz.r[2] = -CD*Sin_alpha - CL*Cos_alpha*Cos_beta*Cos_beta;
  C_xyz.r[1] = CY_b*Beta  + CY_p*Phat + CY_r*Rhat + CY_dr*rudder;

  /* total moments in body axes */
  Cl =        Cl_b*Beta  + Cl_p*Phat + Cl_r_mod*Rhat + Cl_dr*rudder
    + dCl_stall                               + Cl_da*aileron;
  Cn =        Cn_b*Beta  + Cn_p_mod*Phat + Cn_r*Rhat + Cn_dr*rudder
    + dCn_stall                               + Cn_da*aileron;
  Cm = Cm_0 + Cm_a*(Alpha-Alpha_0) +dCm_stall
    + Cm_q*Qhat                 + Cm_de*elevator;

  /* set dimensional forces and moments */
  QS = 0.5*Density*V_rel_wind*V_rel_wind * S_ref;

  v_F_aero = C_xyz * QS;

  v_M_aero.r[0] = Cl * QS * B_ref;
  v_M_aero.r[1] = Cm * QS * C_ref;
  v_M_aero.r[2] = Cn * QS * B_ref;
  
#if (EOM_TEST == 1)
  {
    double ele = 0.8*(-inputs->elevator);
    double ail = 0.8*(inputs->aileron);
    double rud = 0.8*(0.5*inputs->aileron);
   
    v_F_aero = CRRCMath::Vector3();
    v_M_aero = CRRCMath::Vector3(ail, ele, rud);
    stalling = 0;
  }
#endif  
  
}


void CRRC_AirplaneSim_Larcsim::ls_step_init() 
{
  TSimInputs    ZeroInput = TSimInputs();

  /* Set past values to zero */
  v_V_dot_past = CRRCMath::Vector3();
  latitude_dot_past = longitude_dot_past = radius_dot_past  = 0;
  v_R_omega_dot_body_past = CRRCMath::Vector3();
  e_dot_0_past = e_dot_1_past = e_dot_2_past = e_dot_3_past = 0;

  /* Initialize geocentric position from geodetic latitude and altitude */

  ls_geod_to_geoc( Latitude, Altitude, &Sea_level_radius, &Lat_geocentric);
  Lon_geocentric = Longitude;
  Radius_to_vehicle = Altitude + Sea_level_radius;

  /* Initialize quaternions and transformation matrix from Euler angles */

  e_0 = cos(Psi*0.5)*cos(Theta*0.5)*cos(Phi*0.5)
    + sin(Psi*0.5)*sin(Theta*0.5)*sin(Phi*0.5);
  e_1 = cos(Psi*0.5)*cos(Theta*0.5)*sin(Phi*0.5)
    - sin(Psi*0.5)*sin(Theta*0.5)*cos(Phi*0.5);
  e_2 = cos(Psi*0.5)*sin(Theta*0.5)*cos(Phi*0.5)
    + sin(Psi*0.5)*cos(Theta*0.5)*sin(Phi*0.5);
  e_3 = -cos(Psi*0.5)*sin(Theta*0.5)*sin(Phi*0.5)
    + sin(Psi*0.5)*cos(Theta*0.5)*cos(Phi*0.5);
  LocalToBody.v[0][0] = e_0*e_0 + e_1*e_1 - e_2*e_2 - e_3*e_3;
  LocalToBody.v[0][1] = 2*(e_1*e_2 + e_0*e_3);
  LocalToBody.v[0][2] = 2*(e_1*e_3 - e_0*e_2);
  LocalToBody.v[1][0] = 2*(e_1*e_2 - e_0*e_3);
  LocalToBody.v[1][1] = e_0*e_0 - e_1*e_1 + e_2*e_2 - e_3*e_3;
  LocalToBody.v[1][2] = 2*(e_2*e_3 + e_0*e_1);
  LocalToBody.v[2][0] = 2*(e_1*e_3 + e_0*e_2);
  LocalToBody.v[2][1] = 2*(e_2*e_3 - e_0*e_1);
  LocalToBody.v[2][2] = e_0*e_0 - e_1*e_1 - e_2*e_2 + e_3*e_3;
  
  /*    Initialize vehicle model                        */
  ls_aux();

  aero(&ZeroInput);
  engine(0, &ZeroInput);
  gear(&ZeroInput);

  /*    Calculate initial accelerations */
  ls_accel();

  /* Initialize auxiliary variables */
  ls_aux();
}


/**
 * This code is based on ls_step.c in the original version of CRRCSim, which said:
 * 
 *                      Written 920802 by Bruce Jackson.  Based upon equations
 *                      given in reference [1] and a Matrix-X/System Build block
 *                      diagram model of equations of motion coded by David Raney
 *                      at NASA-Langley in June of 1992.    
 *  
 *              [ 1]    McFarland, Richard E.: "A Standard Kinematic Model
 *                      for Flight Simulation at NASA-Ames", NASA CR-2497,
 *                      January 1975
 *  
 *              [ 2]    ANSI/AIAA R-004-1992 "Recommended Practice: Atmos-
 *                      pheric and Space Flight Vehicle Coordinate Systems",
 *                      February 1992
 */
void CRRC_AirplaneSim_Larcsim::ls_step( SCALAR dt )   
{
  SCALAR        dth;
  SCALAR        epsilon, inv_eps;
  SCALAR        e_dot_0, e_dot_1, e_dot_2, e_dot_3;
  SCALAR        cos_Lat_geocentric, inv_Radius_to_vehicle;
    
  CRRCMath::Vector3    v_R_omega_local;    /* Angular L rates      */
  CRRCMath::Vector3    v_R_local_in_body;  
  CRRCMath::Vector3    v_R_omega_total;    /* Diff btw B & L       */

  VECTOR_3    geocentric_rates_v;       /* Geocentric linear velocities */
#define Geocentric_rates_v      geocentric_rates_v
#define Latitude_dot            geocentric_rates_v[0]
#define Longitude_dot           geocentric_rates_v[1]
#define Radius_dot              geocentric_rates_v[2]

/* Update time */

  dth = 0.5*dt;

/*  L I N E A R   V E L O C I T I E S   */

/* Integrate linear accelerations to get velocities */
/*    Using predictive Adams-Bashford algorithm     */

  v_V_local += (v_V_dot_local*3 - v_V_dot_past)*dth;

/* record past states */

  v_V_dot_past = v_V_dot_local;

/* Calculate trajectory rate (geocentric coordinates) */

  inv_Radius_to_vehicle = 1.0/Radius_to_vehicle;
  cos_Lat_geocentric = cos(Lat_geocentric);

  if ( cos_Lat_geocentric != 0)
  {
    Longitude_dot = v_V_local.r[1]/(Radius_to_vehicle*cos_Lat_geocentric);
  }
  else
  {
    // This is just to stop some compilers from complaining about a
    // non-initialized Longitude_dot. It's not mathematically correct
    // (Longitude_dot will move towards +inf if the cosine gets 0),
    // but it also should be irrelevant and at least it's better than
    // relying on something that isn't initialized.
    Longitude_dot = 0;
    fprintf(stderr, "Error: Longitude_dot --> +inf!\n");
  }

  Latitude_dot = v_V_local.r[0]*inv_Radius_to_vehicle;
  Radius_dot   = -v_V_local.r[2];

/*  A N G U L A R   V E L O C I T I E S   A N D   P O S I T I O N S  */

/* Integrate rotational accelerations to get velocities */

  v_R_omega_body = v_R_omega_body + (v_R_omega_dot_body*3 - v_R_omega_dot_body_past)*dth;
  
  // sanity check: v_R_omega_body.length() * dt < pi/2
  {
    double vRo_max = 0.5*M_PI / dt;
    double vRo_len = v_R_omega_body.length();
    
    if (vRo_len > vRo_max)
      v_R_omega_body *= vRo_max/vRo_len;
  }
  
/* Save past states */
  
  v_R_omega_dot_body_past = v_R_omega_dot_body;

/* Calculate local axis frame rates due to travel over curved earth */

  v_R_omega_local.r[0] =  v_V_local.r[1]*inv_Radius_to_vehicle;
  v_R_omega_local.r[1] = -v_V_local.r[0]*inv_Radius_to_vehicle;
  v_R_omega_local.r[2] = -v_V_local.r[1]*tan(Lat_geocentric)*inv_Radius_to_vehicle;

  /* Transform local axis frame rates to body axis rates */
  v_R_local_in_body = LocalToBody * v_R_omega_local;

  /* Calculate total angular rates in body axis */
  v_R_omega_total = v_R_omega_body - v_R_local_in_body;

/* Transform to quaternion rates (see Appendix E in [2]) */

  e_dot_0 = 0.5*( -v_R_omega_total.r[0]*e_1 - v_R_omega_total.r[1]*e_2 - v_R_omega_total.r[2]*e_3 );
  e_dot_1 = 0.5*(  v_R_omega_total.r[0]*e_0 - v_R_omega_total.r[1]*e_3 + v_R_omega_total.r[2]*e_2 );
  e_dot_2 = 0.5*(  v_R_omega_total.r[0]*e_3 + v_R_omega_total.r[1]*e_0 - v_R_omega_total.r[2]*e_1 );
  e_dot_3 = 0.5*( -v_R_omega_total.r[0]*e_2 + v_R_omega_total.r[1]*e_1 + v_R_omega_total.r[2]*e_0 );

/* Integrate using trapezoidal as before */

  e_0 = e_0 + dth*(e_dot_0 + e_dot_0_past);
  e_1 = e_1 + dth*(e_dot_1 + e_dot_1_past);
  e_2 = e_2 + dth*(e_dot_2 + e_dot_2_past);
  e_3 = e_3 + dth*(e_dot_3 + e_dot_3_past);

/* calculate orthagonality correction  - scale quaternion to unity length */

  epsilon = sqrt(e_0*e_0 + e_1*e_1 + e_2*e_2 + e_3*e_3);
  inv_eps = 1/epsilon;

  e_0 = inv_eps*e_0;
  e_1 = inv_eps*e_1;
  e_2 = inv_eps*e_2;
  e_3 = inv_eps*e_3;

/* Save past values */

  e_dot_0_past = e_dot_0;
  e_dot_1_past = e_dot_1;
  e_dot_2_past = e_dot_2;
  e_dot_3_past = e_dot_3;

/* Update local to body transformation matrix */

  LocalToBody.v[0][0] = e_0*e_0 + e_1*e_1 - e_2*e_2 - e_3*e_3;
  LocalToBody.v[0][1] = 2*(e_1*e_2 + e_0*e_3);
  LocalToBody.v[0][2] = 2*(e_1*e_3 - e_0*e_2);
  LocalToBody.v[1][0] = 2*(e_1*e_2 - e_0*e_3);
  LocalToBody.v[1][1] = e_0*e_0 - e_1*e_1 + e_2*e_2 - e_3*e_3;
  LocalToBody.v[1][2] = 2*(e_2*e_3 + e_0*e_1);
  LocalToBody.v[2][0] = 2*(e_1*e_3 + e_0*e_2);
  LocalToBody.v[2][1] = 2*(e_2*e_3 - e_0*e_1);
  LocalToBody.v[2][2] = e_0*e_0 - e_1*e_1 - e_2*e_2 + e_3*e_3;
  
/* Calculate Euler angles */

  Theta = asin( -1*LocalToBody.v[0][2] );

  if( LocalToBody.v[0][0] == 0 )
    Psi = 0;
  else
    Psi = atan2( LocalToBody.v[0][1], LocalToBody.v[0][0] );

  if( LocalToBody.v[2][2] == 0 )
    Phi = 0;
  else
    Phi = atan2( LocalToBody.v[1][2], LocalToBody.v[2][2] );

/* Resolve Psi to 0 - 359.9999 */

  if (Psi < 0 ) Psi = Psi + 2*M_PI;

/*  L I N E A R   P O S I T I O N S   */

/* Trapezoidal acceleration for position */

  Lat_geocentric       = Lat_geocentric    + dth*(Latitude_dot  + latitude_dot_past );
  Lon_geocentric       = Lon_geocentric    + dth*(Longitude_dot + longitude_dot_past);
  Radius_to_vehicle    = Radius_to_vehicle + dth*(Radius_dot    + radius_dot_past );

/* Save past values */

  latitude_dot_past  = Latitude_dot;
  longitude_dot_past = Longitude_dot;
  radius_dot_past    = Radius_dot;

/* end of ls_step */
}


/**
 * This method is largely based on ls_aux.c (initial version of CRRCSim), which is 
 * taken from LaRCSIM and was created 9208026 as part of C-castle simulation project
 * by Bruce Jackson.
 */
void CRRC_AirplaneSim_Larcsim::ls_aux()       
{
  /* velocity of veh. relative to airmass */
  CRRCMath::Vector3 v_V_local_rel_airmass;
  
  /* update geodetic position */

  ls_geoc_to_geod( Lat_geocentric, Radius_to_vehicle,
                   &Latitude, &Altitude, &Sea_level_radius );
  Longitude = Lon_geocentric;

  /* Form relative velocity vector */

  v_V_local_rel_ground.r[0] = v_V_local.r[0];
  v_V_local_rel_ground.r[1] = v_V_local.r[1];
  v_V_local_rel_ground.r[2] = v_V_local.r[2];  

  v_V_local_rel_airmass = v_V_local_rel_ground - v_V_local_airmass;
  
  v_V_wind_body = LocalToBody * v_V_local_rel_airmass;
    
  // jwtodo: body and local are added? This must be wrong. But it has been like this in CRRCSim...
  v_V_wind_body += v_V_gust_local;
  
  V_rel_wind = v_V_wind_body.length();

  /* Calculate flight path and other flight condition values */

  if (v_V_wind_body.r[0] == 0)
    Alpha = 0;
  else
    Alpha = atan2( v_V_wind_body.r[2], v_V_wind_body.r[0] );

  if (V_rel_wind == 0)
    Beta = 0;
  else
    Beta = asin( v_V_wind_body.r[1]/ V_rel_wind );

    /* Calculate local gravity  */

  // original code called
  //      ls_gravity( Radius_to_vehicle, Lat_geocentric, &Gravity );
  Gravity = env->GetG(Altitude);
    /* call function for (smoothed) density ratio, sonic velocity, and
           ambient pressure */

  Density = env->GetRho(Altitude);
  
/* Determine location in runway coordinates */

  v_P_CG_Rwy.r[0] = Sea_level_radius * Latitude;
  v_P_CG_Rwy.r[1] = Sea_level_radius * Longitude;
  v_P_CG_Rwy.r[2] = Sea_level_radius - Radius_to_vehicle;
  
/* end of ls_aux */

}


/**
 * This code is based on ls_accel.c in the original version of CRRCSim, which said:
 * 
 *    Written 920731 by Bruce Jackson.  Based upon equations
 *    given in reference [1] and a Matrix-X/System Build block
 *    diagram model of equations of motion coded by David Raney
 *    at NASA-Langley in June of 1992.
 */
void CRRC_AirplaneSim_Larcsim::ls_accel()     
{
  SCALAR        inv_Mass, inv_Radius;
  SCALAR        ixz2, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10;
  SCALAR        tan_Lat_geocentric;

  CRRCMath::Vector3 v_F;
  CRRCMath::Vector3 v_M_cg;
  CRRCMath::Vector3 v_F_local;
  
  /* Sum forces and moments at reference point (center of gravity) */

  v_F    = v_F_aero + v_F_engine + v_F_gear;
  v_M_cg = v_M_aero + v_M_engine + v_M_gear;
  
#if (EOM_TEST == 2)
  
  switch ((nLogCnt>>5) % 6)
  {
   case 0:
    v_F    = CRRCMath::Vector3(1, 0, 0);
    v_M_cg = CRRCMath::Vector3(0, 0, 0);
    break;
   case 1:
    v_F    = CRRCMath::Vector3(0, 0, 0);
    v_M_cg = CRRCMath::Vector3(0, 2, 0);
    break;
   case 2:
    v_F    = CRRCMath::Vector3(0, 0, 3);
    v_M_cg = CRRCMath::Vector3(0, 0, 0);
    break;
   case 3:
    v_F    = CRRCMath::Vector3(0, 0, 0);
    v_M_cg = CRRCMath::Vector3(1, 0, 0);
    break;
   case 4:
    v_F    = CRRCMath::Vector3(0, 2, 0);
    v_M_cg = CRRCMath::Vector3(0, 0, 0);
    break;
   case 5:
    v_F    = CRRCMath::Vector3(0, 0, 0);
    v_M_cg = CRRCMath::Vector3(0, 0, 3);
    break;
  }
  
  {
    double d = sin(M_PI*(nLogCnt&0x1F)/32.0);
    
    v_F    *= d*d;
    v_M_cg *= d*d;
  }
  
  
  if (  ((nLogCnt>>5)/6) % 2)
  {
    v_F    *= -1;
    v_M_cg *= -1;
  }
  
#endif  
  
  /* Transform from body to local frame */

  v_F_local = LocalToBody.multrans(v_F);
  
  /* Calculate linear accelerations */

  tan_Lat_geocentric = tan(Lat_geocentric);
  inv_Mass    = 1/Mass;
  inv_Radius  = 1/Radius_to_vehicle;
  
  v_V_dot_local.r[0] = inv_Mass*v_F_local.r[0] + inv_Radius*(v_V_local.r[0]*v_V_local.r[2] - v_V_local.r[1]*v_V_local.r[1] *tan_Lat_geocentric);
  v_V_dot_local.r[1] = inv_Mass*v_F_local.r[1] + inv_Radius*(v_V_local.r[1]*v_V_local.r[2]  + v_V_local.r[0]*v_V_local.r[1]*tan_Lat_geocentric);
#if EOM_TEST != 0
  v_V_dot_local.r[2] = inv_Mass*v_F_local.r[2]           - inv_Radius*(v_V_local.r[0]*v_V_local.r[0] + v_V_local.r[1]*v_V_local.r[1]);
#else
  v_V_dot_local.r[2] = inv_Mass*v_F_local.r[2] + Gravity - inv_Radius*(v_V_local.r[0]*v_V_local.r[0] + v_V_local.r[1]*v_V_local.r[1]);
#endif
  /* Invert the symmetric inertia matrix */

  ixz2 = I_xz*I_xz;
  c0  = 1/(I_xx*I_zz - ixz2);
  c1  = c0*((I_yy-I_zz)*I_zz - ixz2);
  c4  = c0*I_xz;
  /* c2  = c0*I_xz*(I_xx - I_yy + I_zz); */
  c2  = c4*(I_xx - I_yy + I_zz);
  c3  = c0*I_zz;
  c7  = 1/I_yy;
  c5  = c7*(I_zz - I_xx);
  c6  = c7*I_xz;
  c8  = c0*((I_xx - I_yy)*I_xx + ixz2);
  /* c9  = c0*I_xz*(I_yy - I_zz - I_xx); */
  c9  = c4*(I_yy - I_zz - I_xx);
  c10 = c0*I_xx;

  /* Calculate the rotational body axis accelerations */

  v_R_omega_dot_body.r[0] = (c1*v_R_omega_body.r[2] + c2*v_R_omega_body.r[0])*v_R_omega_body.r[1] + c3*v_M_cg.r[0] +  c4*v_M_cg.r[2];
  v_R_omega_dot_body.r[1] =  c5*v_R_omega_body.r[2]*v_R_omega_body.r[0] + c6*(v_R_omega_body.r[2]*v_R_omega_body.r[2] - v_R_omega_body.r[0]*v_R_omega_body.r[0]) + c7*v_M_cg.r[1];
  v_R_omega_dot_body.r[2] = (c8*v_R_omega_body.r[0] + c9*v_R_omega_body.r[2])*v_R_omega_body.r[1] + c4*v_M_cg.r[0] + c10*v_M_cg.r[2];
}


double CRRC_AirplaneSim_Larcsim::getPropFreq() 
{
  return(power->getPropFreq());
}


void CRRC_AirplaneSim_Larcsim::registerAnimations(std::vector<CRRCAnimation*> const& anims)
{
  wheels.registerAnimations(anims);
}



