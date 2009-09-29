/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2006, 2008 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2005, 2007, 2008 - Jan Reucker
 *   Copyright (C) 2005 - Lionel Cailler
 *   Copyright (C) 2006 - Todd Templeton
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
#ifndef CRRC_INPUTS_H
# define CRRC_INPUTS_H

#include <iostream>

/** \brief Flight model control input struct
 *
 * Although the sim currently does only use the four basic inputs,
 * I leave the additional ones...
 */
class TSimInputs
{
  public:
  
   /**
    * Only to map some input for steering a wheel.
    */
   enum eSteeringMap { smNOTHING, smAILERON, smRUDDER };
  
   enum { NUM_AUX_INPUTS=4 };
 
   float aileron;    ///< aileron input,          -0.5 ... 0.5
   float elevator;   ///< elevator input,         -0.5 ... 0.5
   float rudder;     ///< rudder input,           -0.5 ... 0.5
   float throttle;   ///< throttle input,          0.0 ... 1.0
   float flap;       ///< flap input,              0.0 ... 1.0
   float spoiler;    ///< spoiler input,           0.0 ... 1.0
   float retract;    ///< retractable gear input,  0.0 ... 1.0
   float pitch;      ///< rotor/propeller pitch,  -0.5 ... 0.5
   float aux[NUM_AUX_INPUTS]; ///< auxiliary inputs, -0.5 ... 0.5, used in display mode
  
  /**
   * 
   */
  void CopyFrom(TSimInputs* source)
  {
    this->aileron  = source->aileron;
    this->elevator = source->elevator;
    this->rudder   = source->rudder;
    this->throttle = source->throttle;
    this->flap     = source->flap;
    this->spoiler  = source->spoiler;
    this->retract  = source->retract;
    this->pitch    = source->pitch;
    
    for(int i = 0; i < NUM_AUX_INPUTS; i++)
      this->aux[i] = source->aux[i];
  };
  
   /**
    * Returns mapped input value.
    */
   float GetInput(eSteeringMap esm) const
   {
     switch (esm)
     {
       case smAILERON:
         return(aileron);
       case smRUDDER:
         return(rudder);
       default:
         return(0);
     }
   }
  
   /**
    * maybe this does help to get some kind of random number
    */
   int getRandNum() const
   {
     int tmp = 0;

     // Just mix values in some way.
     // In this case multiplying surely leads to the result being zero.
     // What to do?
     // Add?
     // XOR?
     tmp = *(int*)(void*)(&aileron);
     tmp ^= *(int*)(void*)(&rudder);
     tmp ^= *(int*)(void*)(&elevator);
     tmp ^= *(int*)(void*)(&throttle);
     tmp ^= *(int*)(void*)(&flap);
     tmp ^= *(int*)(void*)(&spoiler);
     tmp ^= *(int*)(void*)(&retract);
     tmp ^= *(int*)(void*)(&pitch);
     
     return(tmp);
   }

   TSimInputs()
   {
     int i;
     aileron  = 0;
     elevator = 0;
     rudder   = 0;
     throttle = 0;
     flap     = 0;
     spoiler  = 0;
     retract  = 0;
     pitch    = 0;
     for(i = 0; i < NUM_AUX_INPUTS; i++)
       aux[i] = 0;
   };
   
   void print()
   {
     std::cout << aileron << " " << elevator << " " << rudder << " " << throttle << "\n";
   };
   
}; 

#endif
