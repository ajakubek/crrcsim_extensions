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
  

/** \file global.h
 *
 *  Global variables used in CRRCsim.
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include "mod_fdm/fdm_inputs.h"
#include "mod_inputdev/inputdev.h"
#include "mouse_kbd.h"

#include "glconsole.h"      // needed to make LOG() work without add. headers
// There's no need to pull in the full headers here.
// Just declare the classes and leave the responsibility
// to the files that really need the single variables.
class SimStateHandler;
class CGUIMain;
class CRRCAirplane;
class CRRCAudioServer;
class ModFDMInterface;
class Scenery;
class T_GameHandler;
class T_TX_Interface;
class TInputDev;
class Aircraft;

/**
 * Contains data related to test mode.
 * 
 * @author Jens Wilhelm Wulf
 */
struct TestModeData
{
  /**
   *  test TXInterface
   */
  int   test_mode;
  
  /**
   * The real flAutozoom is changed while in testmode
   */
  float flAutozoom;
  
};

class Global
{
  public:
    static SimStateHandler* Simulation;     ///< The simulation's main state machine.
    static int              training_mode;  ///< Draw thermals in the sky?
    static int              nVerbosity;     ///< How much info in the HUD?
    static Scenery*         scenery;        ///< The scenery.
    static CGUIMain*        gui;            ///< The GUI.
    static CRRCAudioServer* soundserver;    ///< The sound server.
    static T_GameHandler*   gameHandler;    ///< The active game mode.
    static TSimInputs       inputs;         ///< Control input values.
    static float            dt;             ///< time interval of integration of EOMs
    static std::string      verboseString;  ///< Informational line of text
    static TestModeData     testmode;       ///< Test mode data structure
    static GlConsole*       console;        ///< The console
    static int              nFPS;           ///< average video update rate (FPS)
    static T_TX_Interface*  TXInterface; 
    static TInputDev        inputDev;
    static Aircraft*        aircraft;       ///< A complete Aircraft (model & FDM).
};


/** This macro logs a line of text to the console */
#define LOG(_x)     {if (Global::console != NULL) Global::console->print(_x);}

#endif //GLOBAL_H
