/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Jan Reucker (original author)
 * Copyright (C) 2005, 2006, 2007, 2008 Jens Wilhelm Wulf
 * Copyright (C) 2005, 2008 Olivier Bordes
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
  

/** \file crrc_gui_main.cpp
 *
 *  The central file responsible for managing the graphical user interface.
 */

#include "../global.h"
#include "../SimStateHandler.h"
#include "crrc_gui_main.h"
#include "crrc_msgbox.h"
#include "crrc_planesel.h"
#include "crrc_video.h"
#include "crrc_launch.h"
#include "crrc_windthermal.h"
#include "crrc_joy.h"
#include "crrc_calibmap.h"
#include "../crrc_main.h"
#include "../config.h"
#include "../zoom.h"
#include "../crrc_sound.h"
#include "crrc_ctrlgen.h"
#include "crrc_audio.h"
#include "crrc_location.h"
#include "crrc_f3f.h"
#include "../crrc_graphics.h"


void GUI_IdleFunction(TSimInputs *in);

static void quitDialogCallback(int);

// Callback prototypes
static void file_exit_cb(puObject*);
static void file_save_cb(puObject*);

static void view_fullsc_cb(puObject*);
static void view_train_cb(puObject*);
static void view_testmode_cb(puObject*);
static void view_verbosity_cb(puObject*);
static void view_zoomin_cb(puObject*);
static void view_zoomout_cb(puObject*);
static void view_unzoom_cb(puObject*);

static void sim_restart_cb(puObject*);

static void opt_plane_cb(puObject*);
static void opt_launch_cb(puObject*);
static void opt_location_cb(puObject*);
static void opt_windthermal_cb(puObject*);
static void opt_video_cb(puObject*);
static void opt_audio_cb(puObject*);
static void opt_ctrl_general_cb(puObject*);

static void game_f3f_cb(puObject*);

static void help_web_cb(puObject*);
static void help_keys_cb(puObject*);
static void help_about_cb(puObject*);

// Menu entries and callback mapping
// Caution: submenu-entries must be declared in reverse order!
// File menu
static const char *file_submenu[]    = {"Exit", "Save Settings", NULL};
static puCallback file_submenu_cb[]  = {file_exit_cb, file_save_cb, NULL};

// View submenu
static const char *view_submenu[]   = { "Reset Zoom", "Zoom -", 
                                        "Zoom +", "Toggle Verbosity",
                                        "Toggle Test Mode",
                                        "Toggle Training Mode",
                                        "Toggle Fullscreen", NULL};
static puCallback view_submenu_cb[] = { view_unzoom_cb, view_zoomout_cb,
                                        view_zoomin_cb, view_verbosity_cb,
                                        view_testmode_cb,
                                        view_train_cb,
                                        view_fullsc_cb, NULL};

// Simulation menu
static const char *sim_submenu[]   = {"Restart", NULL};
static puCallback sim_submenu_cb[] = {sim_restart_cb, NULL};

// Options menu
static const char *opt_submenu[]   = {"Audio",
                                      "Controls",
                                      "Video", "Wind, Thermals", 
                                      "Launch", "Location",
                                      "Airplane", NULL};
static puCallback opt_submenu_cb[] = {opt_audio_cb,
                                      opt_ctrl_general_cb,
                                      opt_video_cb, 
                                      opt_windthermal_cb, opt_launch_cb, 
                                      opt_location_cb, opt_plane_cb, NULL};

// Game menu
static const char *game_submenu[]   = {"F3F", NULL};
static puCallback game_submenu_cb[] = {game_f3f_cb, NULL};

// Help menu
static const char *help_submenu[]   = {"About", "Keys", "Help", NULL};
static puCallback help_submenu_cb[] = {help_about_cb, help_keys_cb, help_web_cb, NULL};


/** \brief Create the GUI object.
 *
 *  Creates the GUI and sets its "visible" state.
 *  \param vis Set "visible" state. Defaults to true.
 */
CGUIMain::CGUIMain(bool vis) : visible(vis)
{
  puInit();
  puSetDefaultStyle(PUSTYLE_SMALL_BEVELLED);

  // Light grey, transparency
  puSetDefaultColourScheme(0.85, 0.85, 0.85, 0.85);
  // Light grey, no transparency
  //puSetDefaultColourScheme(0.85, 0.85, 0.85, 1.0);

  // create the menu bar
  main_menu_bar = new puMenuBar() ;
  main_menu_bar->add_submenu("File", (char**)file_submenu, file_submenu_cb);
  main_menu_bar->add_submenu("View", (char**)view_submenu, view_submenu_cb);
  main_menu_bar->add_submenu("Simulation", (char**)sim_submenu, sim_submenu_cb);
  main_menu_bar->add_submenu("Options", (char**)opt_submenu, opt_submenu_cb);
  main_menu_bar->add_submenu("Game", (char**)game_submenu, game_submenu_cb);
  main_menu_bar->add_submenu("Help", (char**)help_submenu, help_submenu_cb);
  main_menu_bar->close();
  
  verboseOutput = new puText(30, 30);
  verboseOutput->setColour(PUCOL_LABEL, 1, 0.1, 0.1);
  verboseOutput->reveal();
  
  if (visible)
  {
    main_menu_bar->reveal();
    SDL_ShowCursor(SDL_ENABLE);
  }
  else
  {
    main_menu_bar->hide();
    if (Global::TXInterface->inputMethod() == T_TX_Interface::eIM_mouse)
      SDL_ShowCursor(SDL_ENABLE);
    else
      SDL_ShowCursor(SDL_DISABLE);
  }

}


/** \brief Destroy the gui object.
 *
 *
 */
CGUIMain::~CGUIMain()
{
  puDeleteObject(main_menu_bar);
  main_menu_bar = NULL;
}


/** \brief Hide the GUI.
 *
 *  This method hides the GUI and all included widgets.
 */
void CGUIMain::hide()
{
  visible = false;
  Global::Simulation->resume();
  main_menu_bar->hide();
  if (Global::TXInterface->inputMethod() == T_TX_Interface::eIM_mouse)
    SDL_ShowCursor(SDL_ENABLE);
  else
    SDL_ShowCursor(SDL_DISABLE);
  Global::Simulation->resetIdle();
}


/** \brief Show the GUI.
 *
 *  This method sets the GUIs "visible" state to true and
 *  activates all included widgets.
 */
void CGUIMain::reveal()
{
  Global::Simulation->setNewIdle(GUI_IdleFunction);
  visible = true;
  Global::Simulation->pause();
  main_menu_bar->reveal();
  SDL_ShowCursor(SDL_ENABLE);
  LOG("Press <ESC> to hide menu and resume simulation.");
}


/** \brief Draw the GUI.
 *
 *  This method has to be called for each OpenGL frame as
 *  long as the GUI is visible. Note: to make the GUI
 *  invisible it's not sufficient to stop calling draw(),
 *  because any visible widget will remain clickable!
 *  Make sure to call hide() before, or simply call draw()
 *  as long as the GUI isVisible().
 */
void CGUIMain::draw()
{
  glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glAlphaFunc(GL_GREATER,0.1f);
  glEnable(GL_BLEND);
  puDisplay();
  glPopAttrib();
}


/** \brief The GUI's key press event handler.
 *
 *  This function should be called from the SDL event loop. It takes a keyboard event
 *  as an argument, translates it to PUI syntax and passes it to the
 *  PUI-internal keyboard function. If there's no active widget which could
 *  use the key event the function will return false, giving the caller
 *  the opportunity to use the event for other purposes.
 *  \param key A key symbol generated by an SDL keyboard event.
 *  \return true if PUI was able to handle the event
 */
bool CGUIMain::keyDownEventHandler(SDL_keysym& key)
{
  int tkey;
  bool ret;

  tkey = translateKey(key);
  ret = puKeyboard(tkey, PU_DOWN);
  
  // ESC key handling
  // note: translateKey() does not affect the ESC keysym,
  // so it is safe to test the SDL key value here
  if (!ret && (tkey == SDLK_ESCAPE))
  {
    if (isVisible())
    {
      CRRCDialog* top = CRRCDialog::getToplevel();
      if (top != NULL)
      {
        if (top->hasCancelButton())
        {
          //std::cout << "Invoking CANCEL for toplevel dialog" << std::endl;
          top->setValue(CRRC_DIALOG_CANCEL);
          top->invokeCallback();
        }
      }
      else
      {
        //std::cout << "No active dialog, hiding GUI" << std::endl;
        hide();
      }
    }
    else
    {
      reveal();
    }
    ret = true;
  }

  return ret;
}

/** \brief The GUI's key release event handler.
 *
 *  This function should be called from the SDL event loop. It takes a keyboard event
 *  as an argument, translates it to PUI syntax and passes it to the
 *  PUI-internal keyboard function. If there's no active widget which could
 *  use the key event the function will return false, giving the caller
 *  the opportunity to use the event for other purposes.
 *  \param key A key symbol generated by an SDL keyboard event.
 *  \return true if PUI was able to handle the event
 */
bool CGUIMain::keyUpEventHandler(SDL_keysym& key)
{
  int tkey = translateKey(key);
  return puKeyboard(tkey, PU_UP);
}

/** \brief The GUI's mouse button press event handler.
 *
 *  This function should be called from the SDL event loop.
 *  It translates a mouse event to PUI syntax and passes it to the
 *  PUI-internal mouse function. If there's no active widget which could
 *  use the event the function will return false, giving the caller
 *  the opportunity to use the event for other purposes.
 *  \param btn Code of the button as reported by SDL
 *  \param x Mouse x coordinate as reported by SDL
 *  \param y Mouse y coordinate as reported by SDL
 *  \return true if PUI was able to handle the event
 */
bool CGUIMain::mouseButtonDownHandler(int btn, int x, int y)
{
  return puMouse(translateMouse(btn), PU_DOWN, x, y);
}

/** \brief The GUI's mouse button release event handler.
 *
 *  This function should be called from the SDL event loop.
 *  It translates a mouse event to PUI syntax and passes it to the
 *  PUI-internal mouse function. If there's no active widget which could
 *  use the event the function will return false, giving the caller
 *  the opportunity to use the event for other purposes.
 *  \param btn Code of the button as reported by SDL
 *  \param x Mouse x coordinate as reported by SDL
 *  \param y Mouse y coordinate as reported by SDL
 *  \return true if PUI was able to handle the event
 */
bool CGUIMain::mouseButtonUpHandler(int btn, int x, int y)
{
  return puMouse(translateMouse(btn), PU_UP, x, y);
}


/** \brief The GUI's mouse motion handler.
 *
 *
 */
bool CGUIMain::mouseMotionHandler(int x, int y)
{
  return puMouse(x, y);
}


/** \brief Translate SDL key macros to PUI macros.
 *
 *  Make sure that SDL unicode support is turned on to
 *  make this work!
 */
int CGUIMain::translateKey(const SDL_keysym& keysym)
{
  // Printable characters
  if (keysym.unicode > 0)
    return keysym.unicode;

  // Numpad key, translate no non-numpad equivalent
  if (keysym.sym >= SDLK_KP0 && keysym.sym <= SDLK_KP_EQUALS) 
  {
    switch (keysym.sym) 
    {
      case SDLK_KP0:
        return PU_KEY_INSERT;
      case SDLK_KP1:
        return PU_KEY_END;
      case SDLK_KP2:
        return PU_KEY_DOWN;
      case SDLK_KP3:
        return PU_KEY_PAGE_DOWN;
      case SDLK_KP4:
        return PU_KEY_LEFT;
      case SDLK_KP6:
        return PU_KEY_RIGHT;
      case SDLK_KP7:
        return PU_KEY_HOME;
      case SDLK_KP8:
        return PU_KEY_UP;
      case SDLK_KP9:
        return PU_KEY_PAGE_UP;
      default:
        return -1;
    }
  }

  // Everything else
  switch (keysym.sym) 
  {
    case SDLK_UP:
      return PU_KEY_UP;
    case SDLK_DOWN:
      return PU_KEY_DOWN;
    case SDLK_LEFT:
      return PU_KEY_LEFT;
    case SDLK_RIGHT:
      return PU_KEY_RIGHT;
  
    case SDLK_PAGEUP:
      return PU_KEY_PAGE_UP;
    case SDLK_PAGEDOWN:
      return PU_KEY_PAGE_DOWN;
    case SDLK_HOME:
      return PU_KEY_HOME;
    case SDLK_END:
      return PU_KEY_END;
    case SDLK_INSERT:
      return PU_KEY_INSERT;
    case SDLK_DELETE:
      return -1;
  
    case SDLK_F1:
      return PU_KEY_F1;
    case SDLK_F2:
      return PU_KEY_F2;
    case SDLK_F3:
      return PU_KEY_F3;
    case SDLK_F4:
      return PU_KEY_F4;
    case SDLK_F5:
      return PU_KEY_F5;
    case SDLK_F6:
      return PU_KEY_F6;
    case SDLK_F7:
      return PU_KEY_F7;
    case SDLK_F8:
      return PU_KEY_F8;
    case SDLK_F9:
      return PU_KEY_F9;
    case SDLK_F10:
      return PU_KEY_F10;
    case SDLK_F11:
      return PU_KEY_F11;
    case SDLK_F12:
      return PU_KEY_F12;
  
    default:
      return -1;
  }
}

// description: see header file
void CGUIMain::setVerboseText(const char* msg)
{ 
  verboseOutput->setLabel(msg); 
};

// description: see header file
void CGUIMain::errorMsg(const char* message)
{
  fprintf(stderr, "--- GUI error popup ---\n%s\n--- end popup ---------\n", message);
  new CGUIMsgBox(message);
}

void CGUIMain::doQuitDialog()
{
  reveal();
  if (options_changed())
  {
    CGUIMsgBox *msg = new CGUIMsgBox("Configuration has changed, save?", 
                                      CRRC_DIALOG_OK | CRRC_DIALOG_CANCEL,
                                      quitDialogCallback);
    msg->setOKButtonLegend("Yes");
    msg->setCancelButtonLegend("No");
  }
  else
  {
    Global::Simulation->quit();
  }
}

// The menu entry callbacks.

void optionNotImplementedYetBox()
{
  new CGUIMsgBox("Changing this from the GUI isn't implemented yet.\n"
                 "Please see crrcsim.xml and documentation/options.txt\n"
                 "for how to configure CRRCSim.");
}

static void file_exit_cb(puObject *obj)
{
  Global::gui->doQuitDialog();
}

static void file_save_cb(puObject *obj)
{
  options_saveToFile();
}

static void sim_restart_cb(puObject *obj)
{
  Global::Simulation->reset();
  Global::gui->hide();
}

static void opt_plane_cb(puObject *obj)
{
  new CGUIPlaneSelectDialog();
}

static void opt_location_cb(puObject *obj)
{
  new CGUILocationDialog();
}

static void opt_launch_cb(puObject *obj)
{
  new CGUILaunchDialog();
}

static void opt_windthermal_cb(puObject *obj)
{
  new CGUIWindThermalDialog();
}

static void opt_video_cb(puObject *obj)
{
  new CGUIVideoDialog();
}

static void opt_audio_cb(puObject *obj)
{
  new CGUIAudioDialog();
}

static void opt_ctrl_general_cb(puObject *obj)
{
  new CGUICtrlGeneralDialog();
}

static void game_f3f_cb(puObject *obj)
{
  new CGUIF3FDialog();
}

static void help_web_cb(puObject *obj)
{
  new CGUIMsgBox("See http://crrcsim.sourceforge.net/ for more information.\n\n"
                 "With your copy of CRRCSim you also received documentation\n"
                 "in a subdirectory named \"documentation\". Take a look at \"index.html\".");
}

static void help_keys_cb(puObject *obj)
{
  new CGUIMsgBox( "Key mapping:\n\n"
                  "ESC    show/hide menu\n"
                  "q      quit\n"
                  "r      restarts after crash\n"
                  "p      pause\n"
                  "u      unpause\n"
                  "t      toggles training mode which displays the location of the thermals\n"
                  "d      toggle control input debugging mode\n"
                  "pg-up  increase throttle (if you aren't using JOYSTICK or better)\n"
                  "pg-dwn decrease throttle (if you aren't using JOYSTICK or better)\n"
                  "+      zoom in (assuming zoom.control is KEYBOARD)\n"
                  "-      zoom out (assuming zoom.control is KEYBOARD)\n\n"
                  "left/right arrow  left/right rudder\n"
                  "up/down arrow     up/down elevator"
                );
}

static void help_about_cb(puObject *obj)
{
  new CGUIMsgBox(PACKAGE_STRING);
}

static void view_fullsc_cb(puObject *obj)
{
  int nFullscreen;
  int nX, nY;
  if (cfgfile->getInt("video.fullscreen.fUse"))
  {
    nFullscreen = 0;
    nX          = cfgfile->getInt("video.resolution.window.x", 800);
    nY          = cfgfile->getInt("video.resolution.window.y", 600);
  }
  else
  {
    nFullscreen = 1;
    nX          = cfgfile->getInt("video.resolution.fullscreen.x", 800);
    nY          = cfgfile->getInt("video.resolution.fullscreen.y", 600);
  }
  #ifdef linux
  // On Linux we can switch the mode on-the-fly.
  // This also puts the fullscreen flag back into the config file.
  video_setup(nX, nY, nFullscreen);
  #else
  // Other platforms need to put the value back into the
  // config file and restart manually.
  cfgfile->setAttributeOverwrite("video.fullscreen.fUse", nFullscreen);
  new CGUIMsgBox("Please save your configuration and restart CRRCSim!");
  #endif
}

static void view_train_cb(puObject *obj)
{
  if (Global::training_mode)
    Global::training_mode = 0;
  else
    Global::training_mode = 1;
}

static void view_verbosity_cb(puObject *obj)
{
  if (Global::nVerbosity == 3)
    Global::nVerbosity = 0;
  else
    Global::nVerbosity++;
}

static void view_zoomin_cb(puObject *obj)
{
  zoom_in();
}

static void view_zoomout_cb(puObject *obj)
{
  zoom_out();
}

static void view_unzoom_cb(puObject *obj)
{
  zoom_reset();
}

static void quitDialogCallback(int choice)
{
  if (choice == CRRC_DIALOG_OK)
  {
    options_saveToFile();
  }
  Global::Simulation->quit();
}

static void view_testmode_cb(puObject *obj)
{
  Global::testmode.test_mode = (Global::testmode.test_mode ^ 0x0001) & 0x0001;
  if (Global::testmode.test_mode)
    activate_test_mode();
  else
    leave_test_mode();
}


/** \brief The GUI's "idle" function.
 *
 *  This function will be called from the main loop while
 *  the GUI is active. Its main purpose is to propagate
 *  the interface's input values to the CGUIMain object.
 *  A dialog that needs to evaluate the input signals may
 *  then access this local copy of the values by calling
 *  CGUIMain::getInputValues()
 */
void GUI_IdleFunction(TSimInputs *in)
{
  CRRCDialog *dlg = CRRCDialog::getToplevel();
  
  Global::gui->setInputValues(in);
  if (dlg != NULL)
  {
    dlg->update();
  }
}

void CGUIMain::setInputValues(TSimInputs *in)
{
  memcpy(&input, in, sizeof(TSimInputs));
}

TSimInputs* CGUIMain::getInputValues()
{
  return &input;
}

