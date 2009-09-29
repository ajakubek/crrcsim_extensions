// -*- mode: c; mode: fold -*-
/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2008, 2009 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2008 - Jan Reucker
 * 
 * This file was copied from src/mod_fdm/fdm_larcsim/fdm_larcsim.cpp on 2008-09-05
 * and still contains some of its code.
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
//
#include "fdm_heli01.h"

#include <math.h>
#include <iostream>

#include "../../mod_misc/ls_constants.h"
#include "../ls_geodesy.h"
#include "../../mod_misc/SimpleXMLTransfer.h"
#include "../../mod_misc/lib_conversions.h"
#include "../xmlmodelfile.h"

/**
 * *****************************************************************************
 */

void CRRC_AirplaneSim_Heli01::initAirplaneState(double dRelVel,
                                                 double dTheta,
                                                 double dPsi,
                                                 double X,
                                                 double Y,
                                                 double Z,
                                                 double R_X,
                                                 double R_Y,
                                                 double R_Z) 
{
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

  v_V_local.r[0] = 0;      // local x-velocity (north)   [ft/s]
  v_V_local.r[1] = 0;      // local y-velocity (east)    [ft/s]
  v_V_local.r[2] = 0;      // local z-velocity (down)     [ft/s]
    
  v_V_local_rel_ground.r[1] = v_V_local.r[1];
  
  v_R_omega_body    = CRRCMath::Vector3(R_X, R_Y, R_Z); // body rate   [rad/s]  
  v_V_dot_local     = CRRCMath::Vector3(); // local acceleration   [ft/s^2]
  v_V_local_airmass = CRRCMath::Vector3(); // local velocity of steady airmass   [ft/s]

  dHeadingHoldInt = 0;
  
  power->reset(CRRCMath::Vector3());
  
  ls_step_init();
}


void CRRC_AirplaneSim_Heli01::update(TSimInputs* inputs,
                                     double      dt,
                                     int         multiloop) 
{
  env->CalculateWind(v_P_CG_Rwy.r[0],        v_P_CG_Rwy.r[1],        v_P_CG_Rwy.r[2],
                     v_V_local_airmass.r[0], v_V_local_airmass.r[1], v_V_local_airmass.r[2]);
  
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
            
    ls_step( dt );
    ls_aux();

    env->ControllerCallback(dt, this, inputs, &myInputs);
    
    aero(dt, &myInputs);
    
#if FDM_LOG_AERO_OUT != 0
    logVal(v_F_aero);
    logVal(v_M_aero);
#endif
    
    engine( dt, &myInputs );
    gear( &myInputs );

    ls_accel();
  }
}


CRRC_AirplaneSim_Heli01::CRRC_AirplaneSim_Heli01(const char* filename, FDMEnviroment* myEnv, SimpleXMLTransfer* cfg) : FDMBase("fdm_larcsim.dat", myEnv)
{
  // Previously there has been code to load an old-style .air-file. 
  // This has been removed as CRRCSim includes an automatic converter.
  SimpleXMLTransfer* fileinmemory = new SimpleXMLTransfer(filename);

  LoadFromXML(fileinmemory, cfg->getInt("airplane.verbosity", 5));
  
  delete fileinmemory;
}


CRRC_AirplaneSim_Heli01::CRRC_AirplaneSim_Heli01(SimpleXMLTransfer* xml, FDMEnviroment* myEnv, SimpleXMLTransfer* cfg) : FDMBase("fdm_larcsim.dat", myEnv)
{
  LoadFromXML(xml, cfg->getInt("airplane.verbosity", 5));
}

void CRRC_AirplaneSim_Heli01::LoadFromXML(SimpleXMLTransfer* xml, int nVerbosity)
{
  if (xml->getString("type").compare("heli01") != 0 ||
      xml->getInt("version") != 3)
  {
    throw XMLException("file is not for heli01");
  }    
    
  SimpleXMLTransfer* i;
  SimpleXMLTransfer* cfg = XMLModelFile::getConfig(xml);  
  
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
    
  {
    fCoaxial         =(cfg->getInt("aero.coaxial",      0) != 0);
    fFixedPitch      =(cfg->getInt("power.fixed_pitch", 0) != 0);
    speed_damp       = cfg->getDouble("aero.speed.damp");    
        
    yaw_ctrl         = cfg->getDouble("aero.yaw.ctrl");
    yaw_damp         = cfg->getDouble("aero.yaw.damp");    
    yaw_damp_min_rel = cfg->getDouble("aero.yaw.damp_min_rel");
    yaw_dist         = cfg->getDouble("aero.yaw.dist", 0);
    yaw_off          = cfg->getDouble("aero.yaw.off",  0);
    cp_to_yaw        = cfg->getDouble("aero.yaw.cp_to_yaw",   0);
    dHeadingHold     = cfg->getDouble("aero.yaw.HeadingHold", 0);
    
    roll_ctrl        = cfg->getDouble("aero.roll.ctrl");
    roll_damp        = cfg->getDouble("aero.roll.damp");    
    roll_dist        = cfg->getDouble("aero.roll.dist", 0);
    dForwardToRoll   = cfg->getDouble("aero.roll.ForwardToRoll", 1E-4);
    
    pitch_ctrl       = -1 * cfg->getDouble("aero.pitch.ctrl", roll_ctrl);
    pitch_damp       =      cfg->getDouble("aero.pitch.damp", roll_damp);
    pitch_dist       =      cfg->getDouble("aero.pitch.dist", roll_dist);

    cp_ctrl          = cfg->getDouble("aero.cp.ctrl", 1);
    cp_off           = cfg->getDouble("aero.cp.off",  0);

    yaw_moment_mul   = cfg->getDouble("aero.yaw.moment_mul", 1);
    
    if (fCoaxial)
    {
      yaw_moment_mul = 0;
      dForwardToRoll = 0;
      yaw_off        = 0;
    }
     
    yaw_vane      = cfg->getDouble("aero.yaw.vane",   0);
    pitch_vane    = cfg->getDouble("aero.pitch.vane", 0);
    
    // convert to internal representation:
    yaw_damp   *= I_zz / KG_M_M_TO_SLUG_FT_FT;
    yaw_dist   *= I_zz / KG_M_M_TO_SLUG_FT_FT;
    yaw_vane   *= I_zz / KG_M_M_TO_SLUG_FT_FT;
    roll_damp  *= I_xx / KG_M_M_TO_SLUG_FT_FT;
    roll_dist  *= I_xx / KG_M_M_TO_SLUG_FT_FT;
    pitch_damp *= I_yy / KG_M_M_TO_SLUG_FT_FT;
    pitch_dist *= I_yy / KG_M_M_TO_SLUG_FT_FT;
    pitch_vane *= I_yy / KG_M_M_TO_SLUG_FT_FT;

    roll_ctrl  *= roll_damp/0.5;
    pitch_ctrl *= pitch_damp/0.5;
    
    if (fabs(dHeadingHold) > 1.0E-8)
    {
      dHeadingHold *= I_zz / KG_M_M_TO_SLUG_FT_FT;
      yaw_ctrl *= 2;
    }
    else
    {
      yaw_ctrl *= yaw_damp_min_rel * yaw_damp/0.5;
    }
    
    // The ground effect parameters should be quite independent of the helicopter
    // parameters...shouldn't they? However, they can be adjusted.
    dGEClimbMul = xml->getDouble("GroundEffect.climb.mul", 0.5);
    dGEClimbExp = xml->getDouble("GroundEffect.climb.exp", 2.0);
    dGEDistMul  = xml->getDouble("GroundEffect.dist.mul",  1.5);
    dGEDistExp  = xml->getDouble("GroundEffect.dist.exp",  2.0);

    {
      double tau  = xml->getDouble("Disturbance.tau_filter", 0.2);
      dist_t_init = xml->getDouble("Disturbance.time",       0.2);
      
      filt_rnd_yaw.init(0,   tau);
      filt_rnd_roll.init(0,  tau);
      filt_rnd_pitch.init(0, tau);
      dist_t = 0;
    }
  }  

  wheels.init(xml, 0);
  dRotorRadius = wheels.getWingspan() * FT_TO_M;
      
  // possibly there is no power description:
  if (cfg->indexOfChild("power") < 0)
  {
    power = new Power::Power();
  }
  else
  {
    power = new Power::Power(cfg, nVerbosity);
  }
}


CRRC_AirplaneSim_Heli01::~CRRC_AirplaneSim_Heli01()
{
  delete power;
}

double CRRC_AirplaneSim_Heli01::getPhi()
{
  return(euler_angles_v[0]);
}

double CRRC_AirplaneSim_Heli01::getTheta()
{
  return(euler_angles_v[1]);
}

double CRRC_AirplaneSim_Heli01::getPsi()
{
  return(euler_angles_v[2]);
}

CRRCMath::Vector3 CRRC_AirplaneSim_Heli01::getPos()
{
  return(v_P_CG_Rwy);
}

CRRCMath::Vector3 CRRC_AirplaneSim_Heli01::getVel()
{
  return(v_V_local_rel_ground);
}

CRRCMath::Vector3 CRRC_AirplaneSim_Heli01::getAccel()
{
  return(v_V_dot_local);
}

CRRCMath::Vector3 CRRC_AirplaneSim_Heli01::getPQR()
{
  return(v_R_omega_body);
}

double CRRC_AirplaneSim_Heli01::getLat()
{
  return(Latitude);
}

double CRRC_AirplaneSim_Heli01::getLon()
{
  return(Longitude);
}

double CRRC_AirplaneSim_Heli01::getAlt()
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
void CRRC_AirplaneSim_Heli01::gear(TSimInputs* inputs)      
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

void CRRC_AirplaneSim_Heli01::engine( SCALAR dt, TSimInputs* inputs) 
{
  v_F_engine = CRRCMath::Vector3();
  v_M_engine = CRRCMath::Vector3();
  
  if (fFixedPitch)
  {
    double thr = inputs->throttle;
    if (thr < 0.5)
      inputs->throttle = cp_off * thr/0.5;
    else
      inputs->throttle = cp_off + (1.0-cp_off) * (thr-0.5)/0.5;
    
    inputs->pitch = 1.0;    
  }
  else
  {
    inputs->pitch    = cp_ctrl * (inputs->throttle - 0.5) + cp_off;
    inputs->throttle = 1.0;
  }
  
  power->step(dt, inputs, -v_V_wind_body.r[2]*FT_TO_M, &v_F_engine, &v_M_engine);
  
  v_M_engine = CRRCMath::Vector3(0, 0, -v_M_engine.r[0] * yaw_moment_mul);

  v_F_engine = CRRCMath::Vector3(0, 0, -v_F_engine.r[0]);
  
  // --- Ground effect -------------------
  // Sources I found on the net state that ground effect vanishes at an altitude 
  // more than 1.0 to 1.5 times rotor diameter:
  //   http://www.grider.de/pdf/Aerodynamik%20Der%20Hubschrauber.pdf
  //   http://www.polizei-hubschrauber.de/grundlagen/flugphysik/index.html
  // So a function is needed which
  //   - is equal to one at altitude >= 1.5*2*dRotorRadius
  //   - becomes greater at lower altitudes
  //   - is not only linear
  //   - needs a small number of parameters
  //   - is somehow independend of dRotorRadius
  // Maybe something like y = a*x^b is a good starting point.
  // To make it suit other needs, lets make it
  double dGEMul;
  double dDistGE = 1.0 - (Altitude/(3*dRotorRadius));
  // So we have dDistGE=1 at ground level and dDistGE=0 at an altitude
  // of 1.5*2*dRotorRadius.
  if (dDistGE < 0)
    dGEMul = 1;
  else
    dGEMul = 1 + dGEClimbMul * pow(dDistGE, dGEClimbExp);
  // Transform from body to local frame
  v_F_engine = LocalToBody.multrans(v_F_engine);
  // apply
  if (v_F_engine.r[2] < 0)
    v_F_engine.r[2] *= dGEMul;
  // Transform from local to body frame
  v_F_engine = LocalToBody * v_F_engine;

  // Convert SI to that other buggy system.
  v_F_engine *= N_TO_LBF;
  v_M_engine *= NM_TO_LBFFT;  
}


// Calculate forces and moments for the current time step.
void CRRC_AirplaneSim_Heli01::aero(double dt, TSimInputs* inputs)
{
  // Ground effect, see 'ground effect' in engine().
  double dGEMul;
  double dDistGE = 1.0 - (Altitude/(3*dRotorRadius));
  if (dDistGE < 0)
    dGEMul = 1;
  else
    dGEMul = 1 + dGEDistMul * pow(dDistGE, dGEDistExp);
  
  CRRCMath::Vector3 v_V_wind_body_SI = v_V_wind_body * FT_TO_M;
  
  v_F_aero = v_V_wind_body_SI * (-speed_damp);
  
  dist_t -= dt;
  if (dist_t < 0)
  {
    dist_t += dist_t_init;
    in_rnd_yaw   = rnd_yaw.Get();
    in_rnd_roll  = rnd_roll.Get();
    in_rnd_pitch = rnd_pitch.Get();
  }
  
  filt_rnd_yaw.step(dt, in_rnd_yaw);
  filt_rnd_roll.step(dt, in_rnd_roll);
  filt_rnd_pitch.step(dt, in_rnd_pitch);
  
  double yd = yaw_damp * (1 - 2*fabs(inputs->rudder)*(1 - yaw_damp_min_rel));
  
  double yc;
  if (fabs(dHeadingHold) > 1.0E-8)
  {
    dHeadingHoldInt += dHeadingHold*(-yaw_ctrl * inputs->rudder - v_R_omega_body.r[2]);
    yc = dHeadingHoldInt;
  }
  else
    yc = yaw_off - yaw_ctrl * inputs->rudder + cp_to_yaw * (inputs->throttle - 0.5);
  
  v_M_aero = CRRCMath::Vector3(  roll_ctrl  * inputs->aileron
                               - roll_damp  * v_R_omega_body.r[0] 
                               + roll_dist  * filt_rnd_roll.val * dGEMul,
                                 pitch_ctrl * inputs->elevator
                               - pitch_damp * v_R_omega_body.r[1]
                               + pitch_dist * filt_rnd_pitch.val * dGEMul,
                               yc
                               - yd         * v_R_omega_body.r[2]
                               + yaw_dist   * filt_rnd_yaw.val * dGEMul);

  // When not hovering and not being a coaxial rotor, relative wind velocity adds 
  // differently to the blades veloctiy, and creates a moment normal to moving 
  // direction.
  // Project relative wind velocity onto rotor disc: we already have that in
  // v_V_wind_body. So just apply this:
  v_M_aero += CRRCMath::Vector3(v_V_wind_body_SI.r[0] * dForwardToRoll,
                                v_V_wind_body_SI.r[1] * dForwardToRoll,
                                0);
  
  // vane effect
  v_M_aero += CRRCMath::Vector3(0, 
                                -pitch_vane * v_V_wind_body_SI.r[2],
                                yaw_vane    * v_V_wind_body_SI.r[1]);
    
  // Convert SI to that other buggy system.
  v_F_aero *= N_TO_LBF;
  v_M_aero *= NM_TO_LBFFT;    
}


void CRRC_AirplaneSim_Heli01::ls_step_init() 
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

  aero(0, &ZeroInput);
  engine(0, &ZeroInput);
  gear(&ZeroInput);

  /*    Calculate initial accelerations */
  ls_accel();

  /* Initialize auxiliary variables */
  ls_aux();
}


void CRRC_AirplaneSim_Heli01::ls_step( SCALAR dt )   
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


void CRRC_AirplaneSim_Heli01::ls_aux()       
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


void CRRC_AirplaneSim_Heli01::ls_accel()     
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
  
  /* Transform from body to local frame */
  v_F_local = LocalToBody.multrans(v_F);
  
  /* Calculate linear accelerations */
  tan_Lat_geocentric = tan(Lat_geocentric);
  inv_Mass    = 1/Mass;
  inv_Radius  = 1/Radius_to_vehicle;
  
  v_V_dot_local.r[0] = inv_Mass*v_F_local.r[0] + inv_Radius*(v_V_local.r[0]*v_V_local.r[2] - v_V_local.r[1]*v_V_local.r[1] *tan_Lat_geocentric);
  v_V_dot_local.r[1] = inv_Mass*v_F_local.r[1] + inv_Radius*(v_V_local.r[1]*v_V_local.r[2]  + v_V_local.r[0]*v_V_local.r[1]*tan_Lat_geocentric);
  v_V_dot_local.r[2] = inv_Mass*v_F_local.r[2] + Gravity - inv_Radius*(v_V_local.r[0]*v_V_local.r[0] + v_V_local.r[1]*v_V_local.r[1]);

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


double CRRC_AirplaneSim_Heli01::getPropFreq() 
{
  return(power->getPropFreq());
}


