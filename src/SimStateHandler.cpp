/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2006, 2007, 2008 Jan Reucker (original author)
 * Copyright (C) 2006 Todd Templeton
 * Copyright (C) 2007, 2008 Jens Wilhelm Wulf
 * Copyright (C) 2008 Olivier Bordes
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
  

/** \file SimStateHandler.cpp
 *
 *  This file includes the implementation of the class
 *  SimStateHandler, which takes care of the current
 *  simulation state (running, paused, ...) and provides
 *  some handy time values.
 *
 *  \author J. Reucker
 */

#include "global.h"
#include "aircraft.h"
#include "crrc_main.h"
#include "crrc_soundserver.h"
#include "crrc_graphics.h"
#include "SimStateHandler.h"
#include "mod_mode/T_GameHandler.h"
#include "GUI/crrc_gui_main.h"
#include "mod_fdm/fdm.h"
#include "mod_windfield/windfield.h"

/// \todo current_time may be provided by the caller as a parameter
void idle(TSimInputs* inputs)
{
  static int initialization_time = 0;
  static int time_after_last_integration = 0;   //Time after the last integration of EOMs.
  float flDeltaT;
  int   multiloop;
  int   current_time;

  /**
   * One if the aircraft is outside of the windfield simulation,
   * zero otherwise.
   */
  int nAircraftOutsideWindfieldSim = 0;

  current_time = Global::Simulation->getTotalTime();
  if (Global::Simulation->getState() == STATE_RESUMING)
  {
    initialization_time=current_time;
    time_after_last_integration=0;
    Global::Simulation->setState(STATE_RUN);
  }

  // compute time since last execution of this code (considering pauses):
  flDeltaT = ((current_time-initialization_time) - time_after_last_integration)/1000.0;
  // The flight model should be calculated every dt seconds.
  // Now it has not been calculated for a longer time t, so
  // we calculate it multiloop = t / dt times.
  multiloop=(int)(flDeltaT/Global::dt);
  time_after_last_integration += (int)(multiloop*Global::dt*1000);
  update_thermals(flDeltaT);

  Global::aircraft->getFDMInterface()->update(inputs, Global::dt, multiloop);
  Global::Simulation->incSimSteps(multiloop);

  if (nAircraftOutsideWindfieldSim)
    Global::verboseString += " Outside windfield simulation!";

  double X_cg_rwy =    Global::aircraft->getPos().r[0];
  double Y_cg_rwy =    Global::aircraft->getPos().r[1];
  double H_cg_rwy = -1*Global::aircraft->getPos().r[2];
  
  Global::gameHandler->update(X_cg_rwy,Y_cg_rwy,H_cg_rwy);
  
  graphics_UpdateCamera(flDeltaT);
}



/**
 *  Creates the state machine.
 *
 */
SimStateHandler::SimStateHandler()
  : nState(STATE_RESUMING), IdleFunc(idle), OldIdleFunc(NULL),
    sim_steps(0), pause_time(0), accum_pause_time(0), reset_time(0)
{
}


/**
 *  Destroys the state machine.
 *
 */
SimStateHandler::~SimStateHandler()
{
}


/**
 *  Leaves the PAUSED state and continues the simulation.
 *
 */
void SimStateHandler::resume()
{
  nState = STATE_RESUMING;
  
  // add the time we spent in pause mode to the
  // accumulated pause time counter
  accum_pause_time += SDL_GetTicks() - pause_time;
  
  if (Global::soundserver != NULL)
  {
    Global::soundserver->pause(false);
  }
  LOG("Resuming.");
}


/**
 *  Pauses the simulation and stops the sound server.
 *
 */
void SimStateHandler::pause()
{
  if (nState != STATE_PAUSED)
  {
    // entering pause mode from a different mode
    nState = STATE_PAUSED;
    pause_time = SDL_GetTicks();
  }
  if (Global::soundserver != NULL)
  {
    Global::soundserver->pause(true);
  }
  LOG("Simulation paused.");
}


/**
 *  Resets the simulation, re-initializes the flight
 *  dynamics model and restarts the currently running
 *  game (if any).
 */
void SimStateHandler::reset()
{
  unsigned long int current;
  
  Global::inputs = TSimInputs();

  if (Global::testmode.test_mode)
    leave_test_mode();

  sim_steps = 0;
  
  current = SDL_GetTicks();
  reset_time = current;
  pause_time = current;
  accum_pause_time = 0;
  
  IdleFunc = idle;
  OldIdleFunc = NULL;
  
  initialize_flight_model();

  Global::gameHandler->reset();
  LOG("Simulation reset.");
}


/**
 *  Causes the simulation to terminate after the
 *  current frame.
 */
void SimStateHandler::quit()
{
  nState = STATE_EXIT;
}


/**
 *  Run the specified "idle" function.
 *
 *  \param in Collection of current control input values.
 */
void SimStateHandler::doIdle(TSimInputs* in)
{
  if ((IdleFunc != NULL)
      &&
      ((nState != STATE_PAUSED) || (Global::gui && Global::gui->isVisible())))
  {
    IdleFunc(in);
  }
}


/**
 *  Register a new idle function. The old idle function
 *  will be saved and can be restored by calling resetIdle().
 *
 *  \param new_idle Pointer to the new idle function
 */
void SimStateHandler::setNewIdle(TIdleFuncPtr new_idle)
{
  OldIdleFunc = IdleFunc;
  IdleFunc = new_idle;
}


/**
 *  Restore the previous idle function.
 *
 */
void SimStateHandler::resetIdle()
{
  if (OldIdleFunc != NULL)
  {
    IdleFunc = OldIdleFunc;
    OldIdleFunc = NULL;
  }
}


/**
 *  Returns the elapsed time since the last reset(),
 *  in milliseconds.
 *
 *  \return milliseconds since last reset
 */
unsigned long int SimStateHandler::getTotalTimeSinceReset()
{
  return (SDL_GetTicks() - reset_time);
}


/**
 *  Returns the elapsed time since the last reset(),
 *  but not counting the time spent in PAUSE mode.
 *
 *  \return unpaused time since last reset
 */
unsigned long int SimStateHandler::getGameTimeSinceReset()
{
  unsigned long int total_pause;
  unsigned long int current;
  
  current = SDL_GetTicks();
  total_pause = accum_pause_time;
  if (nState == STATE_PAUSED)
  {
    total_pause += current - pause_time;
  }
  return (current - (total_pause + reset_time));
}


/**
 *  Returns the simulation time since the last reset()
 *  (number of sim steps * dt, in ms)
 *
 *  \return simulation time since last reset
 */
unsigned long int SimStateHandler::getSimulationTimeSinceReset()
{
  return (unsigned long int)(sim_steps*Global::dt*1000);
}


