// -*- mode: c; mode: fold -*-
/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2008 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2008 - Jan Reucker
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
#ifndef FDM_HELI01_H
# define FDM_HELI01_H

# include <stdexcept>
# include <vector>
# include "../ls_types.h"
# include "../fdm.h"
# include "../../mod_math/vector3.h"
# include "../../mod_math/matrix33.h"
# include "../power/power.h"
# include "../../mod_misc/crrc_rand.h"
# include "../gear01/gear.h"

/**
 * simple physical model for a helicopter
 * 
 * @author Jens Wilhelm Wulf
 */
class CRRC_AirplaneSim_Heli01 : public FDMBase
{
   friend class ModFDMInterface;
   friend class CRRC_AirplaneSim_DisplayMode;
   
  public:

   virtual bool   isStalling() { return(false); };
   virtual CRRCMath::Vector3 getPos();   
   virtual double getPhi();
   virtual double getTheta();
   virtual double getPsi();

   /**
    * returns velocity w.r.t. earth surface
    */
   virtual CRRCMath::Vector3 getVel();
   virtual CRRCMath::Vector3 getAccel();

   virtual CRRCMath::Vector3 getPQR();

   virtual double getLat();
   virtual double getLon();
   virtual double getAlt();

   /**
    * Used for sound calculation. It returns the prop's number of revolutions
    * per second [1/s].
    */
   virtual double getPropFreq();

   /**
    * Returns relative battery capacity/fuel left (0..1).
    */
   virtual double getBatCapLeft() { return(power->getBatteryMin()); };

   /**
    * Returns velocity relative to airmass [ft/s].
    */
   virtual double getVRelAirmass() { return(V_rel_wind); };

   /**
    * the longest distance from any of the aircrafts points to the CG
    */
   virtual double getAircraftSize() { return(wheels.getAircraftSize()); };

   /**
    * computed velocity for trimmed flight in dead air [ft/s]
    */
   virtual double getTrimmedFlightVelocity() { return(0); };

   /**
    * Wingspan of the aircraft in feet
    */
   virtual double getWingspan() { return(wheels.getWingspan()); };
   
   /**
    * returns Z coordinate of lowest point
    */
   virtual double getZLow() { return(wheels.getZLow()); };

  private:

   void LoadFromXML(SimpleXMLTransfer* xml, int nVerbosity);
  
   void update(TSimInputs* inputs,
               double      dt,
               int         multiloop);

   virtual void initAirplaneState(double dRelVel,
                                  double dTheta,
                                  double dPsi,
                                  double X,
                                  double Y,
                                  double Z,
                                  double R_X,
                                  double R_Y,
                                  double R_Z);
   
   /**
    * read from file
    */
   CRRC_AirplaneSim_Heli01(const char* filename, FDMEnviroment* myEnv, SimpleXMLTransfer* cfg);

   /**
    * read from xml description
    */
   CRRC_AirplaneSim_Heli01(SimpleXMLTransfer* xml, FDMEnviroment* myEnv, SimpleXMLTransfer* cfg);

   virtual ~CRRC_AirplaneSim_Heli01();
   
  private:
      
   /// @name Mass and inertia
   //@{
   SCALAR Mass;  // inertia
   SCALAR I_xx;  // inertia
   SCALAR I_yy;  // inertia
   SCALAR I_zz;  // inertia
   SCALAR I_xz;  // inertia
   //@}
      
   /// @name Gear and ground interaction
   //@{

   /**
    * Vector containing all hard points/wheels
    */
   WheelSystem wheels;
   //@}

   
   
  private:
   
   void gear(TSimInputs* inputs);
   void aero(double dt, TSimInputs* inputs);
   void engine( SCALAR dt, TSimInputs* inputs);
   void ls_aux();
   void ls_step_init();
   void ls_step( SCALAR dt);
   void ls_accel();
   
  private:   

   /// @name written by constructor
   //@{
   
   /**
    * Propulsion system: batteries, shafts, engines, propellers.
    */
   Power::Power* power;
   
   /**
    * Highest value of a hard points y-coordinate -- it is assumed 
    * to be the rotor radius. If there's a parameter for rotor
    * radius one day, we'll use that.
    */
   double dRotorRadius;

   //@}

  /// @aero
  //@{
  double yaw_ctrl;
  double yaw_off;
  double yaw_damp;
  double yaw_damp_min_rel;
  double roll_ctrl;
  double roll_damp;
  double pitch_ctrl;
  double pitch_damp;
  double cp_ctrl;
  double cp_off;
  bool   fFixedPitch;
  double speed_damp;
  
  double yaw_dist;
  double roll_dist;
  double pitch_dist;
  RandGauss rnd_yaw;
  RandGauss rnd_roll;
  RandGauss rnd_pitch;
  CRRCMath::PT1 filt_rnd_yaw;
  CRRCMath::PT1 filt_rnd_roll;
  CRRCMath::PT1 filt_rnd_pitch;
  double in_rnd_yaw;
  double in_rnd_roll;
  double in_rnd_pitch;
  double dist_t;
  double dist_t_init;
  
  double dHeadingHold;
  double dHeadingHoldInt;
  
  bool fCoaxial;

  double dForwardToRoll;
  
  double yaw_moment_mul;
  double yaw_vane;  
  double pitch_vane;
  double cp_to_yaw;  
  
   /**
    * Ground effect parameters, see code
    */
  double dGEClimbMul;
  double dGEClimbExp;
  double dGEDistMul;
  double dGEDistExp;
  
  
  //@}  
  
   /// @name written by init, update
   //@{
   
   /**
    * velocity of airmass (steady winds) 
    * north, east, down
    */
   CRRCMath::Vector3 v_V_local_airmass;
            
   //@}
   
   /// @name written by aero
   //@{
   /**
    * Force x/y/z
    */
   CRRCMath::Vector3 v_F_aero;
   
   /**
    * Moment
    * l/m/n <-> roll/pitch/yaw
    */
   CRRCMath::Vector3 v_M_aero;
   
   //@}

   /// @name written by gear
   //@{
   
   CRRCMath::Vector3 v_F_gear;
   CRRCMath::Vector3 v_M_gear;
   
   //@}

   /// @name written by engine
   //@{
   CRRCMath::Vector3  v_F_engine;
   CRRCMath::Vector3  v_M_engine;
   //@}

   /// @name written by step
   //@{
   SCALAR	latitude_dot_past, longitude_dot_past, radius_dot_past;

   /**
    * P, Q, R
    */
   CRRCMath::Vector3 v_R_omega_dot_body_past;
   
   /**
    * north, east, down
    */
   CRRCMath::Vector3 v_V_dot_past;
   
   SCALAR	e_0, e_1, e_2, e_3;
   SCALAR	e_dot_0_past, e_dot_1_past, e_dot_2_past, e_dot_3_past;
   int	step_inited;
   
   
   /**
    * Transformation matrix local to body
    */
   CRRCMath::Matrix33 LocalToBody;
   
   /**
    * Angular body rates
    */
   CRRCMath::Vector3 v_R_omega_body;
   
   CRRCMath::Vector3 v_V_local;
   
   VECTOR_3    euler_angles_v;
#define Euler_angles_v		euler_angles_v
#define	Phi			euler_angles_v[0]
#define	Theta			euler_angles_v[1]
#define	Psi			euler_angles_v[2]

    VECTOR_3    geocentric_position_v;
#define Geocentric_position_v	geocentric_position_v
#define Lat_geocentric 		geocentric_position_v[0]
#define	Lon_geocentric 		geocentric_position_v[1]
#define	Radius_to_vehicle	geocentric_position_v[2]
   //@}   
   
   /// @name written by accel
   //@{
   
   CRRCMath::Vector3 v_R_omega_dot_body;
      
   CRRCMath::Vector3 v_V_dot_local;
      
   //@}
   
   /// @name written by aux
   //@{
   
   /**
    * Wind-relative velocities in body axis	
    */
   CRRCMath::Vector3 v_V_wind_body;
      
   SCALAR Sea_level_radius;
   
    VECTOR_3    geodetic_position_v;
#define Geodetic_position_v	geodetic_position_v
#define Latitude		geodetic_position_v[0]
#define	Longitude		geodetic_position_v[1]
#define Altitude       		geodetic_position_v[2]

   /**
    * V rel w.r.t. earth surface
    */
   CRRCMath::Vector3 v_V_local_rel_ground;

   SCALAR    V_rel_wind;
   SCALAR    Alpha, Beta;                        /* in radians	*/
   SCALAR    Gravity;		/* Local acceleration due to G	*/
   SCALAR    Density;
	
   /**
    * CG relative to runway, in rwy coordinates
    * north/east/down
    */
   CRRCMath::Vector3 v_P_CG_Rwy;
   
   //@}
         
};

#endif
