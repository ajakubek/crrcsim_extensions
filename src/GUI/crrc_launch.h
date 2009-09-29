/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2008, 2009 Jens Wilhelm Wulf (original author)
 * Copyright (C) 2006, 2008 Jan Reucker
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
  

/**
 *  \file crrc_launch.h
 *  Declaration of class CGUILaunchDialog
 */


#ifndef CRRC_LAUNCH_H
#define CRRC_LAUNCH_H

#include <plib/pu.h>
#include <plib/puAux.h>

#include "crrc_dialog.h"
#include "crrc_slider.h"
#include "../mod_misc/SimpleXMLTransfer.h"

class CGUILaunchDialog;

/** \brief The Launch options dialog.
 *
 */
class CGUILaunchDialog : public CRRCDialog
{
  public:
    CGUILaunchDialog();
    ~CGUILaunchDialog();
   
    puaComboBox* comboPresets;
    char**      presets;
   
    crrcSlider*   slider_velocity;
    crrcSlider*   slider_altitude;
    crrcSlider*   slider_angle;

    puButton*     check_dlg;
  
    puInput*      inputNewName;
  
    puText*       text_velocity_abs;
    char          acVelAbs[PUSTRING_MAX];
   
    SimpleXMLTransfer* presetGrp;
    int                nPresets;
  
    void    updateVelAbs();
  
    // todo: those values are not GUI-editable yet
    int    nPosRelativeToPlayer;
    double dRelPosFront;
    double dRelPosRight;
};

#endif // CRRC_LAUNCH_H
