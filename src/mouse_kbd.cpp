/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2008 Jens Wilhelm Wulf (original author)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Jan Reucker
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
  

#include "mouse_kbd.h"
#include "defines.h"

#include "mod_misc/lib_conversions.h"
#include "mod_fdm/fdm_inputs.h"

#include <stdio.h>

void TInputDev::init(SimpleXMLTransfer* cfgfile)
{
  int size;
  SimpleXMLTransfer* item;
  SimpleXMLTransfer* group;
  SimpleXMLTransfer* item2;

  // set defaults
  for (int n=0; n<MAXJOYBUTTON + 1; n++)
    joystick_bind_b[n] = NOTHING;

  item = cfgfile->getChild("inputMethod.mouse.bindings.buttons", true);

  mouse_bind_l= getValButton(item->attribute("l",    "RESUME"), RESUME);
  mouse_bind_m= getValButton(item->attribute("m",    "RESET"),  RESET);
  mouse_bind_r= getValButton(item->attribute("r",    "PAUSE"),  PAUSE);
  mouse_bind_u= getValButton(item->attribute("up",   "INCTHROTTLE"),  INCTHROTTLE);
  mouse_bind_d= getValButton(item->attribute("down", "DECTHROTTLE"),  DECTHROTTLE);

  item = cfgfile->getChild("inputMethod.joystick", true);
  joystick_n = item->attributeAsInt("number", 0);

  group = item->getChild("bindings.buttons", true);
  size  = group->getChildCount();
  if (size > MAXJOYBUTTON + 1)
    size = MAXJOYBUTTON + 1;
  for (int n=0; n<size; n++)
  {
    item2 = group->getChildAt(n);
    joystick_bind_b[n] = getValButton(item2->attribute("bind", "NOTHING"), NOTHING);
  }

  // zoom control
  if (strU(cfgfile->getString("zoom.control", "KEYBOARD")).compare("MOUSE") == 0)
    zoom_control = MOUSE;
  else
    zoom_control = KEYBOARD;
}

void TInputDev::putBackIntoCfg(SimpleXMLTransfer* cfgfile)
{
  int size;
  SimpleXMLTransfer* item; // = cfgfile->getChild("inputMethod.mouse.bindings.axes");
  SimpleXMLTransfer* group;
  SimpleXMLTransfer* item2;

  item = cfgfile->getChild("inputMethod.mouse.bindings.buttons");

  item->setAttributeOverwrite("l",    ButtonStrings[mouse_bind_l]);
  item->setAttributeOverwrite("m",    ButtonStrings[mouse_bind_m]);
  item->setAttributeOverwrite("r",    ButtonStrings[mouse_bind_r]);
  item->setAttributeOverwrite("up",   ButtonStrings[mouse_bind_u]);
  item->setAttributeOverwrite("down", ButtonStrings[mouse_bind_d]);

  item = cfgfile->getChild("inputMethod.joystick");
  item->setAttributeOverwrite("number", joystick_n);

  group = item->getChild("bindings.buttons");

  // clean list
  size = group->getChildCount();
  for (int n=0; n<size; n++)
  {
    item2 = group->getChildAt(0);
    group->removeChildAt(0);
    delete item2;
  }
  // create new list
  for (int n=0; n<MAXJOYBUTTON + 1; n++)
  {
    item2 = new SimpleXMLTransfer();
    item2->setName("button");
    item2->addAttribute("bind", ButtonStrings[joystick_bind_b[n]]);
    group->addChild(item2);
  }

  // zoom control
  if (zoom_control == MOUSE)
    cfgfile->setAttributeOverwrite("zoom.control", "MOUSE");
  else
    cfgfile->setAttributeOverwrite("zoom.control", "KEYBOARD");
}

int  TInputDev::getValAxis  (std::string asString, int nDefault)
{
  asString = strU(asString);

  if (asString.compare("AILERON") == 0)
    return(T_AxisMapper::AILERON);
  else if (asString.compare("ELEVATOR") == 0)
    return(T_AxisMapper::ELEVATOR);
  else if (asString.compare("RUDDER") == 0)
    return(T_AxisMapper::RUDDER);
  else if (asString.compare("THROTTLE") == 0)
    return(T_AxisMapper::THROTTLE);
  else
    return(nDefault);
}

int  TInputDev::getValButton(std::string asString, int nDefault)
{
  asString = strU(asString);

  if (asString.compare("RESUME") == 0)
    return(RESUME);
  else if (asString.compare("PAUSE") == 0)
    return(PAUSE);
  else if (asString.compare("RESET") == 0)
    return(RESET);
  else if (asString.compare("INCTHROTTLE") == 0)
    return(INCTHROTTLE);
  else if (asString.compare("DECTHROTTLE") == 0)
    return(DECTHROTTLE);
  else if (asString.compare("ZOOMIN") == 0)
    return(ZOOMIN);
  else if (asString.compare("ZOOMOUT") == 0)
    return(ZOOMOUT);
  else
    return(nDefault);
}

// description: see header file
std::string TInputDev::openJoystick()
{
#if TEST_WITHOUT_JOYSTICK == 0
  int numJoysticks;
  
  // check if the joystick subsystem is initialized,
  // and maybe initialize again
  if (!SDL_WasInit(SDL_INIT_JOYSTICK))
  {
    printf("TInputDev::openJoystick: Joystick subsystem not initialized, doing it now...\n");
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    if (!SDL_WasInit(SDL_INIT_JOYSTICK))
    {
      std::string SDLerr = SDL_GetError();
      printf("Can't initialize joystick subsystem!\n");
      printf("SDL says: %s\n", SDLerr.c_str());
      return ("Unable to initialize joystick subsystem. " + SDLerr);
    }
  }
  
  closeJoystick();
  
  numJoysticks = SDL_NumJoysticks();
  printf("TInputDev::openJoystick: SDL found %d joysticks\n", numJoysticks);
  if (numJoysticks > 0)
  {
    for (int i = 0; i < numJoysticks; i++)
    {
      printf("%d: %s\n", i, SDL_JoystickName(i));
    }
    printf("Trying to open joystick %d\n", joystick_n);
    joy = SDL_JoystickOpen(joystick_n);
    if (joy)
    {
      printf("Opened Joystick %d\n", joystick_n);
      printf("Name: %s\n", SDL_JoystickName(joystick_n));
      printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joy));
      printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));
      return("");
    }
    else
    {
      std::string SDLerr = SDL_GetError();
      printf("SDL says: %s\n", SDLerr.c_str());
      return("Couldn't open Joystick " + itoStr(joystick_n, ' ', 1) + ": " + SDLerr);
    }
  }
  else
  {
    return("No Joysticks found\n");
  }
#else
  printf("Opened fake joystick %d\n", joystick_n);
  return("");
#endif
}


// description: see header file
std::string TInputDev::openJoystick(int joy_n)
{
  joystick_n = joy_n;
  return openJoystick();
}


TInputDev::TInputDev()
{
  joy = (SDL_Joystick*)0;
}

void TInputDev::closeJoystick()
{
  if (joy != (SDL_Joystick*)0)
  {
    SDL_JoystickClose(joy);
    joy = (SDL_Joystick*)0;
  }
}

int TInputDev::getJoystickNumAxes()
{
  #if TEST_WITHOUT_JOYSTICK == 0
  if (joy != NULL)
  {
    return SDL_JoystickNumAxes(joy);
  }
  else
  {
    return 0;
  }
  #else
  return SIMULATED_JOYSTICK_AXES;
  #endif
}

int TInputDev::getJoystickNumButtons()
{
  #if TEST_WITHOUT_JOYSTICK == 0
  if (joy != NULL)
  {
    return SDL_JoystickNumButtons(joy);
  }
  else
  {
    return 0;
  }
  #else
  return SIMULATED_JOYSTICK_BUTTONS;
  #endif
}
