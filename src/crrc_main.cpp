/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2000, 2001, 2004 Jan Edward Kansky (original author)
 * Copyright (C) 2004-2009 Jens Wilhelm Wulf
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Jan Reucker
 * Copyright (C) 2004 Kees Lemmens
 * Copyright (C) 2005 Joel Lienard
 * Copyright (C) 2005 Lionel Cailler
 * Copyright (C) 2005, 2008 Olivier Bordes
 * Copyright (C) 2006 Todd Templeton
 * Copyright (C) 2007, 2008 Chris Bayley
 * Copyright (C) 2009 Joel Lienard
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
  
/*****************************************************************************
Title: CRRCsim, the Charles River Radio Control Club Flight Simulator Project
Authors:
Jan Kansky:  Programming
Mark Drela:  Aerodynamics

Purpose:  The idea for CRRCsim is to take away any last excuse you might
have for not using a flight simulator to keep your thumbs certified for RC
flying.   It's free, it works, enjoy.  This is an open source project, so if
you don't like the way something works, help us fix it!

Thanks:
  Bruce Jackson for the LaRCsim framework.
  Flight Gear project for the sky sphere.

Contacts:
If you'd like to help with CRRCSIM, then send me an email!
    email                : kansky@ll.mit.edu
*****************************************************************************/

#include "global.h"
#include "defines.h"
#include "crrc_loadair.h"
#include "crrc_main.h"
#include "crrc_system.h"
#include <crrc_config.h>
#include "crrc_sound.h"
#include "mod_landscape/crrc_scenery.h"
#include "SimStateHandler.h"
#include "crrc_graphics.h"
#include "mod_windfield/windfield.h"
#include "GUI/crrc_gui_main.h"
#include "GUI/crrc_joy.h"
#include "mod_misc/SimpleXMLTransfer.h"
#include "mod_misc/lib_conversions.h"
#include "config.h"
#include "mod_misc/filesystools.h"
#include "zoom.h"
#include "CTime.h"
#include "mod_mode/T_GameHandler.h"
#include "mod_mode/F3F/handlerF3F.h"
#include "mod_fdm/formats/airtoxml.h"
#include "mod_fdm/xmlmodelfile.h"
#include "mod_misc/crrc_rand.h"
#include "glconsole.h"
#include "crrc_fdm.h"
#include "mod_misc/ls_constants.h"
#include "mod_misc/scheduler.h"
#include "mod_main/eventhandler.h"
#include "aircraft.h"

#include "mod_inputdev/inputdev_serial2/inputdev_serial2.h"
#include "mod_inputdev/inputdev_mnav/inputdev_mnav.h"
#include "mod_inputdev/inputdev_audio/inputdev_audio.h"
#include "mod_inputdev/inputdev_parallel/inputdev_parallel.h"
#include "mod_inputdev/inputdev_software/inputdev_software.h"
#include "mod_inputdev/inputdev_serpic/inputdev_serpic.h"
#include "mod_inputdev/inputdev_serial/inputdev_serial.h"
#include "mod_inputdev/inputdev_zhenhua/inputdev_zhenhua.h"
#include "mod_inputdev/inputdev_socket/inputdev_socket.h"

#ifdef linux
#include "mod_inputdev/inputdev_rctran/inputdev_rctran.h"
#include "mod_inputdev/inputdev_rctran2/inputdev_rctran2.h"
#endif

#include <math.h>
#include <string>
#include <time.h>

//#include <plib/ssg.h>

//#define LOG_FRAMES

/*****************************************************************************/
//Configuration Data
//TInputDev  inputDev;

CTime   *crrc_time;

// Functions defined outside crrc_main.c and used inside crrc_main.c :
void key_down(SDL_keysym *keysym);

// Functions defined and used in crrc_main.cpp
void idle(TSimInputs* inputs);

// Pointer to scenery object and sky sphere
CVector *player_pos;

// Variometer sound
T_VariometerSound *vario_sound = NULL;
int vario_sound_channel = -1;

// Enviroment interface to FDM
FDMEnviroment* fdmenv = 0;


/*****************************************************************************/
void activate_test_mode()
{
  double dDist;

  TSimInputs inp  = TSimInputs();
  
  Global::testmode.test_mode  = TRUE;
  
  // Set distance to plane to make it visible in a good size, set 
  // fixed flFieldOfView.
  {            
    double dAircraftSize;
    float  flFieldOfView; // degrees
   
    Global::testmode.flAutozoom = flAutozoom;
    flAutozoom          = 0; // autozoom disabled
    
    dAircraftSize = Global::aircraft->getFDM()->getAircraftSize();
    // this is just to read flFieldOfView
    flFieldOfView = zoom_calc(dAircraftSize);
    
    // 0.42 * 2* tan(M_PI * flFieldOfView /2 / 180) * dDist  = 2*dAircraftSize
    dDist = dAircraftSize / ( 0.42 * tan(M_PI * flFieldOfView / 2 / 180) );
  }
  
  // enter testmode
  CVector ppos = Global::scenery->getPlayerPosition();
  CRRCMath::Vector3 planeposn(ppos.x - dDist, ppos.y, ppos.z);
  Global::aircraft->enterTestmode(planeposn); 
  initialize_flight_model();
  Global::aircraft->getFDMInterface()->update(&inp, 0, 0);
}

void leave_test_mode()
{
  Global::testmode.test_mode  = FALSE;
  Global::aircraft->leaveTestmode();
  flAutozoom = Global::testmode.flAutozoom;
}

/*****************************************************************************/

/** \brief Calculate Z rotation for simulated SAL
 *
 *  The angular velocity is calculated using an estimated
 *  rotation radius for the C-of-G of 0.8 m (stretched arm)
 *  plus half the wingspan. The value is negative, so a right-hand
 *  SAL is simulated.
 *
 *  \param vel relative launch velocity
 *
 *  \return body rotation around Z axis in rad/s
 */
double calculate_z_rotation(double vel)
{
  double radius = (0.8 / 0.3048) + (Global::aircraft->getFDM()->getWingspan() / 2.0);
  double velocity = vel * Global::aircraft->getFDM()->getTrimmedFlightVelocity(); // ft/s
  return (-1 * velocity / radius);
}


void initialize_flight_model()
{
  float Altitude;

  double velocity_rel = cfgfile->getDouble("launch.velocity_rel", 1);
  double dZRot = 0.0;
  
  if (cfgfile->getInt("launch.sal", 0) == 1)
  {
    dZRot = calculate_z_rotation(velocity_rel);
  }
  double wind_direction = (cfg->wind->getDirection()*M_PI/180);
  double posX, posY;
  
  int StartFromPlayer = cfgfile->getInt("launch.rel_to_player", 1);
  if (Global::scenery->getNumStartPosition() == 0) 
    StartFromPlayer = 1;
  
  if (StartFromPlayer == 1)
  {
    // default relative position is similar to what has been used on original 'Cape Cod' and 'Davis':
    double launchx = cfgfile->getDouble("launch.rel_front", MODELSTART_REL_FRONT);
    double launchy = cfgfile->getDouble("launch.rel_right", MODELSTART_REL_RIGHT);
    posX = -player_pos->z + launchx*cos(wind_direction) - launchy*sin(wind_direction);
    posY =  player_pos->x + launchx*sin(wind_direction) + launchy*cos(wind_direction);
  }
  else
  {
    CVector start_pos = Global::scenery->getStartPosition();
    posX = -start_pos.z;
    posY =  start_pos.x;
  }
  Altitude = cfgfile->getDouble("launch.altitude", 6)
             + Global::aircraft->getFDM()->getZLow() // height of CG above reference ellipsoid [ft]
             + Global::scenery->getHeight( posX, posY);

  Global::aircraft->getFDMInterface()->initAirplaneState(velocity_rel,
                                 cfgfile->getDouble("launch.angle", 0),
                                 wind_direction,
                                 posX,
                                 posY,
                                 -1*Altitude,
                                 0.0,
                                 0.0,
                                 dZRot);
  
  Global::Simulation->resume(); 
}

/*****************************************************************************/

void Init_mod_windfield()
{
  initialize_wind_field(cfg->getCurLocCfgPtr(cfgfile));

  // Check if we're using the standard slope windfield
  // or the dynamic soaring windfield
  cfg->checkDynamicSoaring();
}

/*****************************************************************************/
void calc_fps()
{
  static int last_time = 0;
  int cur_time, time_diff;
  static int avg_diff = 10;
  int avg_fps;

  cur_time = SDL_GetTicks();
  time_diff = (cur_time - last_time);

  if (time_diff < 1)
    time_diff = 10;

  avg_diff += ((time_diff << 8) - avg_diff) >> 5;

  avg_fps = (int)rintf(1000.0 / (avg_diff >> 8));
  Global::nFPS = avg_fps;

  last_time = cur_time;
}

/**
 * Reverts to mouse input.
 */
std::string input_method_failed(std::string msg, bool boRevertToMouse = true)
{
  if (msg.length())
    msg += "\n";

  msg += "Failed to initialize selected input method.\n";
  if (boRevertToMouse)
  {
    msg += "Reverting to mouse input.";

    // close previous interface
    if (Global::TXInterface != (T_TX_Interface*)0)
    {
      delete Global::TXInterface;
      Global::TXInterface = (T_TX_Interface*)0;
    }

    // select and open mouse interface
    cfgfile->setAttributeOverwrite("inputMethod.method", InputMethodStrings[T_TX_Interface::eIM_mouse]);

    Global::TXInterface  = new T_TX_InterfaceSoftware(T_TX_Interface::eIM_mouse, 2);
    if (Global::TXInterface->init(cfgfile))
      msg += "\nEven mouse input failed:\n" + Global::TXInterface->getErrMsg();
  }

  return(msg);
}

/**
 * (Re)configures the input method according to the configuration
 * in <tt>cfgfile</tt>.
 * Returns a message, if something (bad) happened.
 */
std::string reconfigureInputMethod(bool boRevertToMouse)
{
  printf("std::string reconfigureInputMethod()\n");

  std::string method = strU(cfgfile->getString("inputMethod.method", InputMethodStrings[T_TX_Interface::eIM_mouse]));
  std::string msg;

  printf("New input method: %s\n", method.c_str());

  // stop old input method
  if (Global::TXInterface != (T_TX_Interface*)0)
  {
    Global::inputDev.closeJoystick();
    delete Global::TXInterface;
    Global::TXInterface = (T_TX_Interface*)0;
  }

  // create new one
  if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_keyboard])) == 0)
  {
    Global::TXInterface  = new T_TX_InterfaceSoftware(T_TX_Interface::eIM_keyboard, 4);
    Global::TXInterface->init(cfgfile);
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_mouse])) == 0)
  {
    Global::TXInterface  = new T_TX_InterfaceSoftware(T_TX_Interface::eIM_mouse, 2);
    Global::TXInterface->init(cfgfile);
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_joystick])) == 0)
  {
#if TEST_WITHOUT_JOYSTICK == 0
    msg = Global::inputDev.openJoystick();
    if (msg.length())
      return(input_method_failed(msg, boRevertToMouse));
#endif
    
    Global::TXInterface  = new T_TX_InterfaceSoftware(T_TX_Interface::eIM_joystick, Global::inputDev.getJoystickNumAxes());
    Global::TXInterface->init(cfgfile);
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_audio])) == 0)
  {
    #if PORTAUDIO > 0
    Global::TXInterface  = new T_TX_InterfaceAudio();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
    #else
      return(input_method_failed("This version of CRRCsim was built without support for the AUDIO interface.\n", boRevertToMouse));
    #endif
  }
#ifdef linux
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_rctran])) == 0)
  {
    Global::TXInterface  = new T_TX_InterfaceRCTRAN();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_rctran2])) == 0)
  {
    Global::TXInterface  = new T_TX_Interface_RCTran2();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
  }
#endif
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_parallel]))  == 0)
  {
    printf("Trying to init parallel\n");
    Global::TXInterface  = new T_TX_InterfaceParallel();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_serial2])) == 0)
  {
    Global::TXInterface  = new T_TX_InterfaceSerial2();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_mnav])) == 0)
  {
    Global::TXInterface  = new T_TX_InterfaceMNAV();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_serpic])) == 0)
  {
    Global::TXInterface  = new T_TX_InterfaceSerPIC();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_zhenhua])) == 0)
  {
    Global::TXInterface  = new T_TX_InterfaceZhenHua();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
  }
  else if (method.compare(strU(InputMethodStrings[T_TX_Interface::eIM_socket])) == 0)
  {
    Global::TXInterface  = new T_TX_InterfaceSocket();
    if (Global::TXInterface->init(cfgfile))
      return(input_method_failed(Global::TXInterface->getErrMsg(), boRevertToMouse));
  }
  else
    return(input_method_failed("", boRevertToMouse));

  return("");
}

/**
 * Reading of (mostly initial) values from the xml config into
 * global variables (at least global to this file).
 * Maybe those variables can be encapsulated somewhere else...
 */
void read_config_into_globals()
{
  zoom_reset();
  Global::training_mode = cfgfile->getInt("training_mode.fUse", 0);
  Global::dt = cfgfile->getDouble("simulation.flightModel.dt", 0.002777);
  graphics_read_config(cfgfile);
}

void write_globals_into_config()
{
  cfgfile->setAttributeOverwrite("training_mode.fUse", Global::training_mode);
  cfgfile->setAttributeOverwrite("simulation.flightModel.dt",
                                 doubleToString(Global::dt));
}

/*****************************************************************************/
void initializeRandomNumberGenerator()
{
  time_t sometime = time(0);
  srand((unsigned int) sometime);
  CRRC_Random::insertData(rand());
  std::cout << "RAND_MAX = " << RAND_MAX << "\n";
}

/*****************************************************************************/
/** 
 *  This function tries to perform some cleanup when
 *  exiting. If errmsg != NULL, a system message box
 *  will be opened with the message string.
 *
 *  \param exit_code should be CRRC_EXIT_SUCCESS or CRRC_EXIT_FAILURE
 *  \param errmsg    optional message to be displayed on exit
 */
void crrc_exit(int exit_code, const char *errmsg)
{
  // ToDo: clean up allocated objects
  SDL_Quit();
  
  if ((errmsg != NULL) && (*errmsg != '\0'))
  {
    if (exit_code == CRRC_EXIT_SUCCESS)
    {
      SystemMessage(errmsg, SM_INFO);
    }
    else
    {
      SystemMessage(errmsg, SM_ERROR);
    }
  }
  
  exit(exit_code);
}

/**
 * Description: see header file
 */
void loadAirplane()
{
  Global::aircraft->load(cfgfile, fdmenv);
}


void set_aux(int aux_num, int setting)
{
  if (aux_num <= 0 || aux_num > TSimInputs::NUM_AUX_INPUTS)
    return;
  switch(setting)
  {
    case 1:
      Global::inputs.aux[aux_num - 1] = -0.5;
      break;
    case 2:
      Global::inputs.aux[aux_num - 1] =  0.0;
      break;
    case 3:
      Global::inputs.aux[aux_num - 1] =  0.5;
      break;
    default:
      break;
  }
}



/*****************************************************************************/
int main(int argc,char **argv)
{
  float flModelSoundVolume = 1.0f;
  float field_of_view;

  try
  {
    Global::TXInterface = (T_TX_Interface*)0;

    FileSysTools::SetAppname("crrcsim");
    Global::testmode.test_mode = FALSE;

    Global::Simulation = new SimStateHandler();
    
    Global::aircraft = new Aircraft();
    
    std::string startup_time;
    getSystemTimeString(startup_time);
    printf("CRRCsim %s started at %s\n", PACKAGE_VERSION, startup_time.c_str());
    printf("Running on %s\n", getOSVersionString());
    printf("Using plib version %d.%d.%d\n", PLIB_MAJOR_VERSION,
                                            PLIB_MINOR_VERSION,
                                            PLIB_TINY_VERSION);
    printf("Compiled with SDL version %d.%d.%d\n",  SDL_MAJOR_VERSION,
                                                    SDL_MINOR_VERSION,
                                                    SDL_PATCHLEVEL);
    printf("(Linked SDL version is %d.%d.%d)\n",  SDL_Linked_Version()->major,
                                                  SDL_Linked_Version()->minor,
                                                  SDL_Linked_Version()->patch);
#ifdef CRRC_DATA_PATH
    printf("Configured data path: %s\n", CRRC_DATA_PATH);
#endif

    {
      std::vector<std::string> search_path;
      
      FileSysTools::getSearchPathList(search_path, "");
      std::cout << "Data file search path:" << std::endl;
      for (std::vector<std::string>::size_type i = 0; i < search_path.size(); i++)
      {
        std::cout << "  " << search_path[i] << std::endl;
      }
    }

    initializeRandomNumberGenerator();

    {
      unsigned int SDLFlags = SDL_INIT_JOYSTICK;
      int nRetCodeCmdline;
      int i;

      try
      {
        // ***** Read configuration, parse commandline... ***********************
        for (i = 1; i < argc - 1; i++)
        {
          if (!strcmp(argv[i], "-g"))
            T_Config::putConfigFilePath(argv[i+1]);
        }
        
        cfg = new T_Config(cfgfile); // This will also set up cfgfile
        cfg->read(cfgfile);
        
        {
          int nNum = air_to_xml();
          if (nNum != 0)
          {
            // todo: files have been converted; issue message.
          }
        }
                    
        Global::inputDev.init(cfgfile);
        fdmenv = new CRRC_FDM_Env(cfgfile);  
        
        // command line options override settings read from the config file
        nRetCodeCmdline = crrc_checkopts(argc, argv, cfgfile, cfg);

        if (nRetCodeCmdline)
          crrc_exit(CRRC_EXIT_FAILURE);

        // must be after crrc_checkopts because crrc_checkopts can change
        //   video.enabled and sound.enabled based on command line options
        if (cfgfile->getInt("video.enabled", 1))
          SDLFlags |= SDL_INIT_VIDEO;
        if (cfgfile->getInt("sound.enabled", 1))
          SDLFlags |= SDL_INIT_AUDIO;
        SDL_Init(SDLFlags);
        SDL_EnableUNICODE(1); // We need this to pass keys to pui
        SDL_EnableKeyRepeat(50, 150);

        read_config_into_globals();

        std::string msg = reconfigureInputMethod();
        if (msg.length())
          printf("%s", msg.c_str());

        // ***** Video setup ****************************************************
        if (cfgfile->getInt("video.enabled", 1))
          video_setup(0, 0, 0);

        // ***** Setting window caption *****************************************
        if (cfgfile->getInt("video.enabled", 1))
          setWindowTitleString();

        // ***** Sound **********************************************************
        if (cfgfile->getInt("sound.enabled", 1))
        {
          try
          {
            Global::soundserver = new CRRCAudioServer(cfgfile);
            // init the variometer sound
            cfgfile->makeSureAttributeExists("sound.variometer.vol", "0");
            int vario_sound_volume = cfgfile->getInt("sound.variometer.vol") << 3;
            vario_sound = new T_VariometerSound(Global::soundserver->getAudioSpec());
            vario_sound_channel = Global::soundserver->playSample((T_SoundSample*)vario_sound, vario_sound_volume);
            // get engine sound volume
            cfgfile->makeSureAttributeExists("sound.model.vol", "1.0");
            flModelSoundVolume = (float)cfgfile->getDouble("sound.model.vol");
            soundUpdate3D(0.0, 0.0, 0.0, 0.0, flModelSoundVolume);

            Global::soundserver->pause();
          }
          catch (std::runtime_error& e)
          {
            fprintf(stderr, "%s\n", e.what());
            Global::soundserver = (CRRCAudioServer*)0;
          }
        }
        else
          Global::soundserver = (CRRCAudioServer*)0;

        // ***** Video **********************************************************

        initialize_scenegraph();
    
        std::string sceneryfile = cfg->getLocationName();
        Global::scenery = loadScenery(FileSysTools::getDataPath(sceneryfile).c_str());
        if (Global::scenery == NULL)
        {
          fprintf(stderr, "Unable to initialize scenery from file %s,\n",
                          sceneryfile.c_str());
          fprintf(stderr, "reverting to default scenery \"scenery/davis-orig.xml\"\n");
          
          // try the default scenery
          Global::scenery = loadScenery(FileSysTools::getDataPath("scenery/davis-orig.xml").c_str());
          if (Global::scenery == NULL)
          {
            std::string s;
            s = "Unable to initialize default scenery from file \"scenery/davis-orig.xml\"";
            crrc_exit(CRRC_EXIT_FAILURE, s.c_str());
          }
          else
          {
            // set configuration file back to default
            cfg->setLocation("scenery/davis-orig.xml", cfgfile);
          }
        }
        // re-read the wind configuration to activate any scenery defaults if
        // no wind config is present in the cfgfile for this location
        cfg->wind->read(cfgfile, cfg);

        if (cfgfile->getInt("video.enabled", 1))
          initialize_window(cfg);
        Init_mod_windfield();

        player_pos = new CVector(Global::scenery->getPlayerPosition());
				// Create invisible GUI
        if (cfgfile->getInt("video.enabled", 1))
          Global::gui = new CGUIMain(false);
        else
          Global::gui = NULL;

        // load airplane
        {
          bool airplane_failed = false;
          try
          {
            // load the airplane specified in the config file
            loadAirplane();
            initialize_flight_model();
          }
          catch (std::runtime_error& e)
          {
            fprintf(stderr, "%s\n", e.what());
            airplane_failed = true;
          }
          if (airplane_failed)
          {
            // Failed to load airplane file.
            // Using some fallback.
            cfgfile->setAttributeOverwrite("airplane.file", FileSysTools::getDataPath("models/allegro.xml"));
            try
            {
              loadAirplane();
              initialize_flight_model();
            }
            catch (std::runtime_error& e)
            {
              std::string s = "Unable to load airplane file:\n";
              s += e.what();
              fprintf(stderr, "%s\n", s.c_str());
              crrc_exit(CRRC_EXIT_FAILURE, s.c_str());
            }
          }
        }

        //initialise game mode
        if (cfgfile->getInt("game.f3f.enabled",0))
          Global::gameHandler= new HandlerF3F();
        else
          Global::gameHandler= new T_GameHandler();
      }
      catch (XMLException e)
      {
        std::string s = "XMLException: ";
        s += e.what();
        fprintf(stderr, "%s\n", s.c_str());
        crrc_exit(CRRC_EXIT_FAILURE, s.c_str());
      }
    }
      
    if (Global::soundserver != (CRRCAudioServer*)0)
      Global::soundserver->pause(false);
    Global::Simulation->reset();
    
    //~ nVerbosity = 3;
    crrc_time = new CTime(cfgfile);
    
    Global::console = new GlConsole(400, 125, 15, 15);
    if (Global::console == NULL)
    {
      crrc_exit(CRRC_EXIT_FAILURE, "Unable to initialize console.");
    }
    Global::console->setAutoHide(4000, 1000);
    LOG("CRRCsim successfully started!");
    LOG("Press <ESC> to show the setup menu.");
    
#ifdef LOG_FRAMES
    FILE* fp = fopen("frames.dat", "w");
    if (fp == NULL)
    {
      crrc_exit(CRRC_EXIT_FAILURE, "Failed.");
    }
#endif
    
    Scheduler scheduler;
    EventHandler eventHandler(&scheduler);
    
    while (Global::Simulation->getState() != STATE_EXIT)
    {
      crrc_time->update();
      // handle_events();
      scheduler.Run();

      Global::TXInterface->getInputData(&Global::inputs);

      Global::Simulation->doIdle(&Global::inputs);
      
      // random data
      {
        CRRC_Random::insertData(SDL_GetTicks());
        CRRC_Random::insertData(Global::inputs.getRandNum());                   
      }

      // get aircraft position from FDM
      CRRCMath::Vector3 vFdmPos = Global::aircraft->getPos();
      double X_cg_rwy =    vFdmPos.r[0];
      double Y_cg_rwy =    vFdmPos.r[1];
      double H_cg_rwy = -1*vFdmPos.r[2];
      float  distance_to_model = player_pos->distance(X_cg_rwy, H_cg_rwy, Y_cg_rwy);
      
      field_of_view = zoom_calc(distance_to_model);
      if (Global::gui)
        adjust_zoom(field_of_view);
      calc_fps();
      
      #if 0
      Global::verboseString += " X: " + ftoStr(vFdmPos.r[0], 2, 2, true, false);
      Global::verboseString += " Y: " + ftoStr(vFdmPos.r[1], 2, 2, true, false);
      Global::verboseString += " Z: " + ftoStr(vFdmPos.r[2], 2, 2, true, false);
      Global::verboseString += " Phi: " + ftoStr(Global::aircraft->getFDM()->getPhi() * SG_RADIANS_TO_DEGREES, 2, 2, true, false);
      Global::verboseString += " Theta: " + ftoStr(Global::aircraft->getFDM()->getTheta() * SG_RADIANS_TO_DEGREES, 2, 2, true, false);
      Global::verboseString += " Psi: " + ftoStr(Global::aircraft->getFDM()->getPsi() * SG_RADIANS_TO_DEGREES, 2, 2, true, false);
      if (Global::gui)
        Global::gui->setVerboseText(Global::verboseString.c_str());
      #else
      switch (Global::nVerbosity)
      {
       case 3:
        Global::verboseString += "FPS: " + itoStr(Global::nFPS, ' ', 1);
        //fallthrough
       case 2:
        Global::verboseString += " FoV: " + ftoStr(field_of_view, 2, 1, false, false);
        //fallthrough
       case 1:
        Global::verboseString += 
            "\nAil: " + ftoStr(Global::inputs.aileron,  2, 2, true, false)
          + " Ele: "  + ftoStr(Global::inputs.elevator, 2, 2, true, false)
          + " Rud: "  + ftoStr(Global::inputs.rudder,   2, 2, true, false)
          + " Thr: "  + ftoStr(Global::inputs.throttle, 2, 2, true, false)
          + "\nFlp: " + ftoStr(Global::inputs.flap,     2, 2, true, false)
          + " Spo: "  + ftoStr(Global::inputs.spoiler,  2, 2, true, false)
          + " Ret: "  + ftoStr(Global::inputs.retract,  2, 2, true, false)
          + " Pit: "  + ftoStr(Global::inputs.pitch,    2, 2, true, false);
        if (Global::gui)
          Global::gui->setVerboseText(Global::verboseString.c_str());
        else
        {
          static int verbose_print_c = 0;
          if (++verbose_print_c >= 30)
          {
            Global::verboseString += "\n";
            std::cout << Global::verboseString;
            verbose_print_c = 0;
          }
        }
        break;

       default:
        if (Global::gui)
          Global::gui->setVerboseText("");
        break;
      }
      #endif
      
      if (Global::gui)
        display();
      Global::verboseString = "";

#ifdef LOG_FRAMES
      fprintf(fp, "%lu\n", (unsigned long)SDL_GetTicks());
#endif
      
      // sound calculations
      if (Global::soundserver != (CRRCAudioServer*)0)
        soundUpdate3D(distance_to_model,
                      Global::aircraft->getFDM()->getPropFreq(),
                      -1*vFdmPos.r[2],
                      Global::aircraft->getFDM()->getVRelAirmass()/Global::aircraft->getFDM()->getTrimmedFlightVelocity(),
                      flModelSoundVolume);
    }
#ifdef LOG_FRAMES
    fclose(fp);
#endif

  }
  catch (std::exception& e)
  {
    std::string s = "Caught exception: ";
    s += e.what();
    crrc_exit(CRRC_EXIT_FAILURE, s.c_str());
  }

  delete fdmenv;
  if (vario_sound != (T_VariometerSound*)0)
  {
    Global::soundserver->stopChannel(vario_sound_channel);
    delete vario_sound;
  }
  delete Global::aircraft;
  delete Global::scenery;
  graphics_cleanup();
  if (Global::soundserver != (CRRCAudioServer*)0)
    delete Global::soundserver;
  delete Global::Simulation;

  crrc_exit(CRRC_EXIT_SUCCESS, NULL);

  // crrc_exit() will never return, keep the compiler happy anyway:
  return 0;
}

