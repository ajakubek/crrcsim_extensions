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
 *  \file crrc_launch.cpp
 *  Implementation of class CGUILaunchDialog
 */

#include "crrc_launch.h"

#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "../global.h"
#include "../aircraft.h"
#include "../mod_fdm/fdm.h"
#include "../config.h"
#include "../crrc_main.h"
#include "../mod_misc/lib_conversions.h"
#include "../mod_misc/SimpleXMLTransfer.h"

static void CGUILaunchCallback(puObject *obj);
static void CGUILaunchPresetCallback(puObject *obj);
static void CGUILaunchNewPresetCallback(puObject *obj);
static void CGUILaunchVelSliderCallback(puObject *obj);

#define BUTTON_BOX_HEIGHT   (2*DLG_DEF_SPACE+DLG_DEF_BUTTON_HEIGHT)
#define SHORT_SLIDER_W  (210)
#define SLIDER_W 330
#define SLIDER_H 25
#define COMBO_H  20
#define COMBO_W  330
#define LABEL_W  170
#define NUM_W    70


CGUILaunchDialog::CGUILaunchDialog() 
            : CRRCDialog()
{
  // Load presets
  // first copy those from the config file
  presetGrp = new SimpleXMLTransfer(cfgfile->getChild("launch"));

  // now add those from the current airplane
  SimpleXMLTransfer *alp = Global::aircraft->getFDMInterface()->getLaunchPresets();
  if (alp != NULL)
  {
    for (int i = 0; i < alp->getChildCount(); i++)
    {
      // don't add existing children, always make a copy!
      presetGrp->addChild(new SimpleXMLTransfer(alp->getChildAt(i)));
    }
  }

  // make a list for the combo box
  presets = T_GUI_Util::loadnames(presetGrp, nPresets);
  
  // Create Widgets
  comboPresets = new puaComboBox(LABEL_W + DLG_DEF_SPACE, 
                                BUTTON_BOX_HEIGHT + 5* DLG_DEF_SPACE + 4* SLIDER_H + DLG_CHECK_H,
                                LABEL_W + COMBO_W,
                                BUTTON_BOX_HEIGHT + 5* DLG_DEF_SPACE + 4* SLIDER_H + DLG_CHECK_H + COMBO_H,
                                presets, false);
  comboPresets->setLabelPlace(PUPLACE_CENTERED_LEFT);
  comboPresets->setLabel("Load Preset");
  comboPresets->setCurrentItem(0);
  comboPresets->setCallback(CGUILaunchPresetCallback);
  comboPresets->reveal();
  comboPresets->activate();
  comboPresets->setUserData(this);

  slider_altitude = new crrcSlider(LABEL_W + DLG_DEF_SPACE,
                                   BUTTON_BOX_HEIGHT + 4* DLG_DEF_SPACE + 3* SLIDER_H + DLG_CHECK_H,
                                   LABEL_W + DLG_DEF_SPACE + SLIDER_W,
                                   BUTTON_BOX_HEIGHT + 4* DLG_DEF_SPACE + 4* SLIDER_H + DLG_CHECK_H,
                                   NUM_W);
  slider_altitude->setLabelPlace(PUPLACE_CENTERED_LEFT);
  slider_altitude->setLabel("altitude [ft]");
  slider_altitude->setSliderFraction(0.05);
  slider_altitude->setMinValue(0);
  slider_altitude->setMaxValue(700);
  slider_altitude->setStepSize(1);
  slider_altitude->setValue((float)cfgfile->getDouble("launch.altitude"));
    
  slider_velocity = new crrcSlider(LABEL_W + DLG_DEF_SPACE,
                                   BUTTON_BOX_HEIGHT + 3* DLG_DEF_SPACE + 2* SLIDER_H + DLG_CHECK_H,
                                   LABEL_W + DLG_DEF_SPACE + SHORT_SLIDER_W,
                                   BUTTON_BOX_HEIGHT + 3* DLG_DEF_SPACE + 3* SLIDER_H + DLG_CHECK_H,
                                   NUM_W);
  slider_velocity->setLabelPlace(PUPLACE_CENTERED_LEFT);
  slider_velocity->setLabel("relative velocity");
  slider_velocity->setSliderFraction(0.05);
  slider_velocity->setMinValue(0);
  slider_velocity->setMaxValue(7);
  slider_velocity->setStepSize(0.1);
  slider_velocity->setUserData(this);
  slider_velocity->setValue((float)cfgfile->getDouble("launch.velocity_rel"));
  slider_velocity->setCallback(CGUILaunchVelSliderCallback);
  
  text_velocity_abs = new puText( LABEL_W + 2*DLG_DEF_SPACE + SHORT_SLIDER_W,
                                  BUTTON_BOX_HEIGHT + 3* DLG_DEF_SPACE + 2* SLIDER_H + DLG_CHECK_H);
  updateVelAbs();
  
  
  check_dlg = new puButton(LABEL_W + DLG_DEF_SPACE,
                           BUTTON_BOX_HEIGHT + 2* DLG_DEF_SPACE + 2* SLIDER_H,
                           LABEL_W + DLG_DEF_SPACE + DLG_CHECK_W,
                           BUTTON_BOX_HEIGHT + 2* DLG_DEF_SPACE + 2* SLIDER_H + DLG_CHECK_H);
  check_dlg->setButtonType(PUBUTTON_VCHECK);
  check_dlg->setLabelPlace(PUPLACE_CENTERED_LEFT);
  check_dlg->setLabel("Simulate SAL");
  check_dlg->setValue((int)cfgfile->getInt("launch.sal"));

  slider_angle = new crrcSlider(LABEL_W + DLG_DEF_SPACE,            BUTTON_BOX_HEIGHT + 1* DLG_DEF_SPACE + 1* SLIDER_H,
                                LABEL_W + DLG_DEF_SPACE + SLIDER_W, BUTTON_BOX_HEIGHT + 1* DLG_DEF_SPACE + 2* SLIDER_H,
                                NUM_W);
  slider_angle->setLabelPlace(PUPLACE_CENTERED_LEFT);
  slider_angle->setLabel("vertical angle [rad]");
  slider_angle->setSliderFraction(0.05);
  slider_angle->setMinValue(-0.78);
  slider_angle->setMaxValue(1.57);
  slider_angle->setStepSize(0.01);
  slider_angle->setValue((float)cfgfile->getDouble("launch.angle"));
  
  inputNewName = new puInput(          DLG_DEF_SPACE, BUTTON_BOX_HEIGHT + 0* DLG_DEF_SPACE + 0* SLIDER_H,
                             LABEL_W + DLG_DEF_SPACE, BUTTON_BOX_HEIGHT + 0* DLG_DEF_SPACE + 1* SLIDER_H);
  inputNewName->setValue("name of new preset");

  puOneShot* buttonTmp = new puOneShot(LABEL_W + DLG_DEF_SPACE,            BUTTON_BOX_HEIGHT + 0* DLG_DEF_SPACE + 0* SLIDER_H,
                                       LABEL_W + DLG_DEF_SPACE + SLIDER_W, BUTTON_BOX_HEIGHT + 0* DLG_DEF_SPACE + 1* SLIDER_H);
  buttonTmp->setLegend("Save as new preset");
  buttonTmp->setCallback(CGUILaunchNewPresetCallback);
  buttonTmp->setUserData(this);        
  
  close();
  setSize(SLIDER_W + LABEL_W + 2*DLG_DEF_SPACE, 
          4*SLIDER_H + DLG_CHECK_H + 6*DLG_DEF_SPACE + COMBO_H + BUTTON_BOX_HEIGHT);
  setCallback(CGUILaunchCallback);
  
  // center the dialog on screen
  int wwidth, wheight;
  int current_width = getABox()->max[0] - getABox()->min[0];
  int current_height = getABox()->max[1] - getABox()->min[1];
  puGetWindowSize(&wwidth, &wheight);
  setPosition(wwidth/2 - current_width/2, wheight/2 - current_height/2);

  reveal();
}


/**
 * Destroy the dialog.
 */

CGUILaunchDialog::~CGUILaunchDialog()
{
  for (int n=0; n<nPresets; n++)
    free(presets[n]);
  free(presets);
}


/** \brief Update absolute velocity label
 *
 */
void CGUILaunchDialog::updateVelAbs()
{
  // get trimmed flight velocity
  double dTrimmedFlightVelocity = Global::aircraft->getFDM()->getTrimmedFlightVelocity();
  float fValue = (float)(slider_velocity->getFloatValue() * dTrimmedFlightVelocity);
  snprintf(acVelAbs, PUSTRING_MAX, "= %.1f ft/s", fValue);
  text_velocity_abs->setLabel(acVelAbs);
}


/** \brief Callback when moving the velocity slider
 *
 *
 */
void CGUILaunchVelSliderCallback(puObject *obj)
{
  CGUILaunchDialog* dlg   = (CGUILaunchDialog*)(obj->getUserData());
  dlg->updateVelAbs();
}


/** \brief callback to load a preset.
 *
 */
void CGUILaunchPresetCallback(puObject *obj)
{
  CGUILaunchDialog* dlg   = (CGUILaunchDialog*)obj->getUserData();
  int               nItem = dlg->comboPresets->getCurrentItem();
    
  if (nItem == 0)
  {
    // the "default" entry
    dlg->slider_altitude->setValue(0);
    dlg->slider_velocity->setValue(0);
    dlg->slider_angle->setValue(0);
    dlg->check_dlg->setValue(0);
    dlg->nPosRelativeToPlayer = 1;
    dlg->dRelPosFront = MODELSTART_REL_FRONT;
    dlg->dRelPosRight = MODELSTART_REL_RIGHT;    
  }
  else
  {
    SimpleXMLTransfer* preset = dlg->presetGrp->getChildAt(nItem-1);
    dlg->slider_altitude->setValue((float)preset->getDouble("altitude", 10.0));
    dlg->slider_velocity->setValue((float)preset->getDouble("velocity_rel", 1.0));
    dlg->slider_angle->setValue((float)preset->getDouble("angle", 0.0));
    dlg->check_dlg->setValue((int)preset->getInt("sal", 0));
    dlg->nPosRelativeToPlayer = preset->getInt("rel_to_player", 1);
    dlg->dRelPosFront = preset->getDouble("rel_front", MODELSTART_REL_FRONT);
    dlg->dRelPosRight = preset->getDouble("rel_right", MODELSTART_REL_RIGHT);
  }
  dlg->updateVelAbs();
}


/** \brief callback to save a preset.
 *
 */
void CGUILaunchNewPresetCallback(puObject *obj)
{
  CGUILaunchDialog* dlg   = (CGUILaunchDialog*)obj->getUserData();
  
  SimpleXMLTransfer* launch   = new SimpleXMLTransfer();
  
  launch->setName("preset");
  launch->setAttribute("name_en",       dlg->inputNewName->getStringValue());
  launch->setAttribute("altitude",      dlg->slider_altitude->getStringValue());
  launch->setAttribute("velocity_rel",  dlg->slider_velocity->getStringValue());
  launch->setAttribute("angle",         dlg->slider_angle->getStringValue());
  launch->setAttribute("sal",           dlg->check_dlg->getStringValue());
  launch->setAttribute("rel_to_player", dlg->nPosRelativeToPlayer);
  launch->setAttribute("rel_front",     doubleToString(dlg->dRelPosFront));
  launch->setAttribute("rel_right",     doubleToString(dlg->dRelPosRight));
  
  cfgfile->getChild("launch", true)->addChild(launch);
  
  // save file
  options_saveToFile();
  
  // exit dialog (otherwise we would have to regenerate presets)
  dlg->setValue(CRRC_DIALOG_OK);
  CGUILaunchCallback(dlg);
}

/** \brief The dialog's callback.
 *
 */
void CGUILaunchCallback(puObject *obj)
{
  CGUILaunchDialog *dlg = (CGUILaunchDialog*)obj;

  if (obj->getIntegerValue() == CRRC_DIALOG_OK)
  {
    // Dialog left by clicking OK
    cfgfile->setAttributeOverwrite("launch.altitude", 
                                   dlg->slider_altitude->getStringValue());

    cfgfile->setAttributeOverwrite("launch.velocity_rel", 
                                   dlg->slider_velocity->getStringValue());
    
    cfgfile->setAttributeOverwrite("launch.angle", 
                                   dlg->slider_angle->getStringValue());
    
    cfgfile->setAttributeOverwrite("launch.sal", dlg->check_dlg->getStringValue());

    
    cfgfile->setAttributeOverwrite("launch.rel_to_player", dlg->nPosRelativeToPlayer);
    cfgfile->setAttributeOverwrite("launch.rel_front",     doubleToString(dlg->dRelPosFront));
    cfgfile->setAttributeOverwrite("launch.rel_right",     doubleToString(dlg->dRelPosRight));
  }
  delete dlg->presetGrp;
  puDeleteObject(obj);
}
