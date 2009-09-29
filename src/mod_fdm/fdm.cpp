/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2006, 2008 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2006, 2007 - Jan Reucker
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
#include "fdm.h"

#include <iostream>

#include "../mod_math/CVector.h"

#include "../mod_fdm_config.h"
#if (MOD_FDM_USE_002 != 0)
# include "fdm_002/fdm_002.h"
#endif
#if (MOD_FDM_USE_LARCSIM != 0)
# include "fdm_larcsim/fdm_larcsim.h"
#endif
#if (MOD_FDM_USE_DISPLAYMODE != 0)
# include "fdm_displaymode/fdm_displaymode.h"
#endif
#if (MOD_FDM_USE_TESTMODE != 0)
# include "fdm_testmode/fdm_testmode.h"
#endif
#if (MOD_FDM_USE_HELI01 != 0)
# include "fdm_heli01/fdm_heli01.h"
#endif
#include "xmlmodelfile.h"

ModFDMInterface::ModFDMInterface()
 : launch_presets(NULL)
{
  fdm = (FDMBase*)0;
}

ModFDMInterface::~ModFDMInterface()
{
  if (launch_presets != NULL)
    delete launch_presets;
  if (fdm != (FDMBase*)0)
    delete fdm;
}

void ModFDMInterface::update(TSimInputs* inputs,
                             double      dt,
                             int         multiloop)
{
  fdm->update(inputs, dt, multiloop);
}

void ModFDMInterface::initAirplaneState(double dRelVel,
                                        double dTheta,
                                        double dPsi,
                                        double X,
                                        double Y,
                                        double Z,
                                        double R_X,
                                        double R_Y,
                                        double R_Z)
{
  fdm->initAirplaneState(dRelVel,
                         dTheta,
                         dPsi,
                         X,
                         Y,
                         Z,
                         R_X,
                         R_Y,
                         R_Z);
}

void ModFDMInterface::Clean()
{
  if (fdm != (FDMBase*)0)
  {
    delete fdm;
  }
  if (launch_presets != NULL)
  {
    delete launch_presets;
  }
  fdm = 0;
  launch_presets = 0;
}

void ModFDMInterface::loadAirplaneTestmode(double dNorth, double dEast, double dDown)
{
  Clean();
  
#if (MOD_FDM_USE_TESTMODE != 0)
  fdm = new CRRC_AirplaneSim_TestMode(CRRCMath::Vector3(dNorth, dEast, dDown));
#endif
}

void ModFDMInterface::loadAirplane(const char* filename, 
                                   FDMEnviroment* myEnv,
                                   SimpleXMLTransfer* cfg)
{
  std::string notloadstring = "";
  Clean();
  
  // todo: find out the type of model to be used by extension or by some value in 'cfg'?

  // Try all available FDMs until one of them doesn't throw an exception:

#if (MOD_FDM_USE_DISPLAYMODE != 0)
  if (fdm == 0 && CRRC_AirplaneSim_DisplayMode::UseMe(cfg))
  {
    try
    {
      fdm = new CRRC_AirplaneSim_DisplayMode(filename, cfg);
      std::cout << "Using CRRC_AirplaneSim_DisplayMode: " << filename << "\n";
    }
    catch (XMLException e)
    {
      notloadstring += "  Not CRRC_AirplaneSim_DisplayMode: " + std::string(e.what()) + std::string("\n");
      fdm = 0;
    }
  }  
#endif
  
#if (MOD_FDM_USE_HELI01 != 0)
  if (fdm == 0)
  {
    try
    {
      fdm = new CRRC_AirplaneSim_Heli01(filename, myEnv, cfg);
      std::cout << "Using CRRC_AirplaneSim_Heli01: " << filename << "\n";
    }
    catch (XMLException e)
    {
      notloadstring += "  Not CRRC_AirplaneSim_Heli01: " + std::string(e.what()) + std::string("\n");
      fdm = 0;
    }
  }  
#endif
  
#if (MOD_FDM_USE_LARCSIM != 0)
  if (fdm == 0)
  {
    try
    {
      fdm = new CRRC_AirplaneSim_Larcsim(filename, myEnv, cfg);
      std::cout << "Using CRRC_AirplaneSim_Larcsim: " << filename << "\n";
    }
    catch (XMLException e)
    {
      notloadstring += "  Not CRRC_AirplaneSim_Larcsim: " + std::string(e.what()) + std::string("\n");
      fdm = 0;
    }
  }
#endif

#if (MOD_FDM_USE_002 != 0)
  if (fdm == 0)
  {
    try
    {
      fdm = new CRRC_AirplaneSim_002(filename, myEnv);
      std::cout << "Using CRRC_AirplaneSim_002: " << filename << "\n";
    }
    catch (XMLException e)
    {
      notloadstring += "  Not CRRC_AirplaneSim_002: " + std::string(e.what()) + std::string("\n");
      fdm = 0;
    }
  }
#endif
  
  if (fdm == 0)
  {
    std::cerr << "The file could not be loaded by any FDM.\n"
      << "The following FDMs are enabled:\n"
      << "  displaymode: " << MOD_FDM_USE_DISPLAYMODE << "\n"
      << "  heli01:      " << MOD_FDM_USE_HELI01      << "\n"
      << "  larcsim:     " << MOD_FDM_USE_LARCSIM     << "\n"
      << "  002:         " << MOD_FDM_USE_002         << "\n"
      << "They said:\n"
      << notloadstring;
  }
}

void ModFDMInterface::loadAirplane(SimpleXMLTransfer* xml, 
                                   FDMEnviroment* myEnv,
                                   SimpleXMLTransfer* cfg)
{
  std::string notloadstring = "";
  Clean();
  
    
  // todo: find out the type of model to be used by some value in 'xml' or 'cfg'?

  // Try all available FDMs until one of them doesn't throw an exception:

#if (MOD_FDM_USE_DISPLAYMODE != 0)
  if (fdm == 0 && CRRC_AirplaneSim_DisplayMode::UseMe(cfg))
  {
    try
    {
      fdm = new CRRC_AirplaneSim_DisplayMode(xml, cfg);
      std::cout << "Using CRRC_AirplaneSim_DisplayMode\n";
    }
    catch (XMLException e)
    {
      notloadstring += "  Not CRRC_AirplaneSim_DisplayMode: " + std::string(e.what()) + std::string("\n");
      fdm = 0;
    }
  }
#endif
  
#if (MOD_FDM_USE_HELI01 != 0)
  if (fdm == 0)
  {
    try
    {
      fdm = new CRRC_AirplaneSim_Heli01(xml, myEnv, cfg);
      std::cout << "Using CRRC_AirplaneSim_Heli01\n";
    }
    catch (XMLException e)
    {
      notloadstring += "  Not CRRC_AirplaneSim_Heli01: " + std::string(e.what()) + std::string("\n");
      fdm = 0;
    }
  }
#endif  

#if (MOD_FDM_USE_LARCSIM != 0)
  if (fdm == 0)
  {
    try
    {
      fdm = new CRRC_AirplaneSim_Larcsim(xml, myEnv, cfg);
      std::cout << "Using CRRC_AirplaneSim_Larcsim\n";
    }
    catch (XMLException e)
    {
      notloadstring += "  Not CRRC_AirplaneSim_Larcsim: " + std::string(e.what()) + std::string("\n");
      fdm = 0;
    }
  }
#endif  

#if (MOD_FDM_USE_002 != 0)
  if (fdm == 0)
  {
    try
    {
      fdm = new CRRC_AirplaneSim_002(xml, myEnv);
      std::cout << "Using CRRC_AirplaneSim_002\n";
    }
    catch (XMLException e)
    {
      notloadstring += "  Not CRRC_AirplaneSim_002: " + std::string(e.what()) + std::string("\n");
      fdm = 0;
    }
  }
#endif

  if (fdm == 0)
  {
    std::cerr << "The file could not be loaded by any FDM.\n"
      << "The following FDMs are enabled:\n"
      << "  displaymode: " << MOD_FDM_USE_DISPLAYMODE << "\n"
      << "  heli01:      " << MOD_FDM_USE_HELI01      << "\n"
      << "  larcsim:     " << MOD_FDM_USE_LARCSIM     << "\n"
      << "  002:         " << MOD_FDM_USE_002         << "\n"
      << "They said:\n"
      << notloadstring;
  }
  
  // try to load launch presets from the airplane file
  launch_presets = XMLModelFile::getLaunchPresets(xml);
}

FDMBase::FDMBase(const char* logfilename, FDMEnviroment* myEnv)
{
  env = myEnv;
#if FDM_LOG != 0
  std::cout << "Opening fdm logfile\n";
  logfile.open(logfilename);
  nLogCnt = 0;
  nStep   = 0;
#endif
}

FDMBase::~FDMBase()
{
#if FDM_LOG != 0
  std::cout << "Closing fdm logfile\n";
  logfile.close();
#endif
}
