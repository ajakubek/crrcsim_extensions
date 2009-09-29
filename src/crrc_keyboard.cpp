/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2004 Kees Lemmens (original author)
 * Copyright (C) 2005, 2006, 2007, 2008 Jens Wilhelm Wulf
 * Copyright (C) 2005, 2006, 2007, 2008 Jan Reucker
 * Copyright (C) 2005 Joel Lienard
 * Copyright (C) 2006 Todd Templeton
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
  

#include "global.h"
#include "aircraft.h"
#include "SimStateHandler.h"
#include "crrc_main.h"
#include "defines.h"
#include "mod_windfield_config.h"
#include "zoom.h"
#include "GUI/crrc_gui_main.h"

#if DEBUG_THERMAL_SCRSHOT == 1
# include "mod_windfield/windfield.h"
# include "mod_fdm/fdm.h"
#endif

/*****************************************************************************/

void key_down(SDL_keysym *keysym)
{
  static int current_aux = -1;
  int received_begin_aux = 0;

  switch (keysym->unicode)
  {
    case 'q':
    case 'Q':
      if (Global::gui) 
        Global::gui->doQuitDialog();
      else
        Global::Simulation->quit();
      break;
  
    default:
      /* just to avoid compiler complains ;) */
      break;
  } 

  switch (keysym->sym)
  {
    case SDLK_r:
      Global::Simulation->reset();
      break;

    case SDLK_p:
      Global::Simulation->pause();
      LOG("Press <u> to resume.");
      break;

    case SDLK_u:
      Global::Simulation->resume();
      break;

    case SDLK_d:
      if (!(Global::gui && Global::gui->isVisible()))
      {
        if (Global::testmode.test_mode)
        {
          Global::Simulation->reset();
        }
        else
        {
          activate_test_mode();
        }
      }
      break;

    case KEY_ZOOM_OUT:
      if (Global::inputDev.zoom_control == KEYBOARD)
      {
        zoom_out();
      }
      break;

    case SDLK_t:
      if (Global::training_mode == FALSE)
      {
        Global::training_mode = TRUE;
        LOG("Training mode is ON, thermals are visible.");
      }
      else
      {
        Global::training_mode = FALSE;
        LOG("Training mode is OFF.");
      }
      break;

    case KEY_ZOOM_IN:
      if (Global::inputDev.zoom_control == KEYBOARD)
      {
        zoom_in();
      }
      break;

    case SDLK_KP5:
      Global::TXInterface->centerControls();
      break;

    case SDLK_LEFT:
    case SDLK_KP4:
      Global::TXInterface->move_rudder(0.05);
      break;

    case SDLK_RIGHT:
    case SDLK_KP6:
      Global::TXInterface->move_rudder(-0.05);
      break;

    case SDLK_KP7:
      Global::TXInterface->move_aileron(-0.05);
      break;

    case SDLK_KP9:
      Global::TXInterface->move_aileron(0.05);
      break;

    case SDLK_UP:
    case SDLK_KP8:
      Global::TXInterface->move_elevator(0.05);
      break;

    case SDLK_DOWN:
    case SDLK_KP2:
      Global::TXInterface->move_elevator(-0.05);
      break;

    case KEY_THROTTLE_MORE:
      Global::TXInterface->increase_throttle();
      break;

    case KEY_THROTTLE_LESS:
      Global::TXInterface->decrease_throttle();
      break;   
    case SDLK_F1:
      current_aux = 1;
      received_begin_aux = 1;
      break;
    case SDLK_F2:
      current_aux = 2;
      received_begin_aux = 1;
      break;
    case SDLK_F3:
      current_aux = 3;
      received_begin_aux = 1;
      break;
    case SDLK_F4:
      current_aux = 4;
      received_begin_aux = 1;
      break;
    case SDLK_1:
      if (current_aux >= 0)
        set_aux(current_aux, 1);
      break;
    case SDLK_2:
      if (current_aux >= 0)
        set_aux(current_aux, 2);
      break;
    case SDLK_3:
      if (current_aux >= 0)
        set_aux(current_aux, 3);
      break;
    /*case SDLK_q:
      if (gui)
        gui->doQuitDialog();
      break;*/
#if DEBUG_THERMAL_SCRSHOT == 1
    case SDLK_s:
      windfield_thermalScreenshot(Global::aircraft->getPos());
      break;
#endif
    default:
      /* just to avoid compiler complaints ;) */
      break;
  }
  
  if (!received_begin_aux)
    current_aux = -1;
}

