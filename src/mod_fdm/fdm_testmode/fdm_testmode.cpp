// -*- mode: c; mode: fold -*-
/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2006, 2008 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2005, 2006 - Jan Reucker
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
#include "fdm_testmode.h"

#include <math.h>
#include <iostream>

#include "../../mod_misc/ls_constants.h"

/**
 * *****************************************************************************
 */


void CRRC_AirplaneSim_TestMode::initAirplaneState(double dRelVel,
                                                  double dTheta,
                                                  double dPsi,
                                                  double X,
                                                  double Y,
                                                  double Z,
                                                  double R_X,
                                                  double R_Y,
                                                  double R_Z)
{
}


void CRRC_AirplaneSim_TestMode::update(TSimInputs* inputs,
                                       double      dt,
                                       int         multiloop) 
{
  angle.r[0] =        inputs->aileron;
  angle.r[1] =       -inputs->elevator;
  angle.r[2] = dPsi0 -inputs->rudder;
  pos.r[1]   = basepos.r[1] +   8*(inputs->throttle)*sin(angle.r[2]);
  pos.r[0]   = basepos.r[0] +   8*(inputs->throttle)*cos(angle.r[2]);
  pos.r[2]   = basepos.r[2] -  10*(inputs->throttle)*sin(angle.r[1]);
}


CRRC_AirplaneSim_TestMode::CRRC_AirplaneSim_TestMode(CRRCMath::Vector3 ipos) : FDMBase("fdm_testmode.dat", 0)
{
  basepos  = ipos + CRRCMath::Vector3(0, 0, 0);
  dPsi0    = 0;
  pos      = CRRCMath::Vector3();
  angle    = CRRCMath::Vector3();
}


CRRC_AirplaneSim_TestMode::~CRRC_AirplaneSim_TestMode()
{
}

double CRRC_AirplaneSim_TestMode::getPhi()
{
  return(angle.r[0]);
}

double CRRC_AirplaneSim_TestMode::getTheta()
{
  return(angle.r[1]);
}

double CRRC_AirplaneSim_TestMode::getPsi()
{
  return(angle.r[2]);
}

CRRCMath::Vector3 CRRC_AirplaneSim_TestMode::getPos()
{
  return(pos);
}
