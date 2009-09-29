/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project

 * Copyright (C) 2005, 2008 Jens Wilhelm Wulf (original author)
 * Copyright (C) 2005, 2008 Jan Reucker
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
  

// implementation of class CGUIInputScaleDialog
//
#include "crrc_scaleinput.h"

#include <stdio.h>
#include <stdlib.h>

#include "../crrc_main.h"
#include "../mod_misc/lib_conversions.h"

static void CGUIInputScaleCallback(puObject *obj);
static void InputScaleEnableButtonCallback(puObject *obj);
//~ static void CenterOffsetCallback(puObject* obj);

//#define BUTTON_BOX_HEIGHT   (2*DLG_DEF_SPACE+DLG_DEF_BUTTON_HEIGHT)
#define BUTTON_BOX_HEIGHT   (3*DLG_DEF_SPACE+2*DLG_DEF_BUTTON_HEIGHT)
#define SLIDER_W  (110)
#define SLIDER_H   (25)
#define LABEL_W    (80)
#define NUM_W      (60)
#define GAP        (10)

CGUIInputScaleDialog::CGUIInputScaleDialog(T_TX_Interface* itxi)
  : CRRCDialog()
{
  txi = itxi;

  // A group containing everything but the "enable" button
  all_widgets = new puGroup(0, BUTTON_BOX_HEIGHT);
  
  puText* info = new puText(DLG_DEF_SPACE, 
                            (NrOfAxes+1)* DLG_DEF_SPACE + (NrOfAxes+1) * SLIDER_H);
  info->setLabelPlace(PUPLACE_LOWER_RIGHT);
  info->setLabel("Hit tab or return after editing a value.\nAlso see documentation/options.txt");
  
  // Create widgets
  {
    for (int n=0; n<3; n++)
    {
      labels[n] = new puText(DLG_DEF_SPACE + LABEL_W + n*(GAP+SLIDER_W),
                             NrOfAxes* DLG_DEF_SPACE + NrOfAxes * SLIDER_H);
    }
    labels[0]->setLabel("trim");
    labels[1]->setLabel("nrate");
    labels[2]->setLabel("exp");
  }
  
  for (int n=0; n<NrOfAxes; n++)
  {
    slider_trim[n] = new crrcSlider(0*GAP + LABEL_W + DLG_DEF_SPACE + 0*SLIDER_W,
                                    n* DLG_DEF_SPACE +  n    * SLIDER_H,
                                    0*GAP + LABEL_W + DLG_DEF_SPACE + 1*SLIDER_W,
                                    n* DLG_DEF_SPACE + (n+1) * SLIDER_H,
                                    NUM_W);
    slider_trim[n]->setLabelPlace(PUPLACE_CENTERED_LEFT);
    slider_trim[n]->setLabel("");
    slider_trim[n]->setSliderFraction(0.2);
    slider_trim[n]->setMinValue(-0.5);
    slider_trim[n]->setMaxValue(0.5);
    slider_trim[n]->setStepSize(0.02);

    slider_nrate[n] = new crrcSlider( 1*GAP + LABEL_W + DLG_DEF_SPACE + 1*SLIDER_W,
                                      n* DLG_DEF_SPACE +  n    * SLIDER_H,
                                      1*GAP + LABEL_W + DLG_DEF_SPACE + 2*SLIDER_W,
                                      n* DLG_DEF_SPACE + (n+1) * SLIDER_H,
                                      NUM_W);
    slider_nrate[n]->setLabelPlace(PUPLACE_CENTERED_LEFT);
    slider_nrate[n]->setLabel("");
    slider_nrate[n]->setSliderFraction(0.2);
    slider_nrate[n]->setMinValue(0);
    slider_nrate[n]->setMaxValue(1);
    slider_nrate[n]->setStepSize(0.02);

    slider_exp[n] = new crrcSlider( 2*GAP + LABEL_W + DLG_DEF_SPACE + 2*SLIDER_W,
                                    n* DLG_DEF_SPACE +  n    * SLIDER_H,
                                    2*GAP + LABEL_W + DLG_DEF_SPACE + 3*SLIDER_W,
                                    n* DLG_DEF_SPACE + (n+1) * SLIDER_H,
                                    NUM_W);
    slider_exp[n]->setLabelPlace(PUPLACE_CENTERED_LEFT);
    slider_exp[n]->setLabel("");
    slider_exp[n]->setSliderFraction(0.2);
    slider_exp[n]->setMinValue(0);
    slider_exp[n]->setMaxValue(1);
    slider_exp[n]->setStepSize(0.02);

    switch (n)
    {
     case 3:
      slider_trim[n]  ->setLabel("aileron");
      slider_trim[n]  ->setValue(txi->mixer->trim_val[T_AxisMapper::AILERON]); 
      slider_nrate[n] ->setValue(txi->mixer->nrate_val[T_AxisMapper::AILERON]);       
      slider_exp[n]   ->setValue(txi->mixer->exp_val[T_AxisMapper::AILERON]); 
      break;     
     case 2:
      slider_trim[n]  ->setLabel("elevator");
      slider_trim[n]  ->setValue(txi->mixer->trim_val[T_AxisMapper::ELEVATOR]); 
      slider_nrate[n] ->setValue(txi->mixer->nrate_val[T_AxisMapper::ELEVATOR]);
      slider_exp[n]   ->setValue(txi->mixer->exp_val[T_AxisMapper::ELEVATOR]);
      break;      
     case 1:
      slider_trim[n]  ->setLabel("rudder");
      slider_trim[n]  ->setValue(txi->mixer->trim_val[T_AxisMapper::RUDDER]); 
      slider_nrate[n] ->setValue(txi->mixer->nrate_val[T_AxisMapper::RUDDER]);
      slider_exp[n]   ->setValue(txi->mixer->exp_val[T_AxisMapper::RUDDER]);
      break;      
     default:
      slider_trim[n]  ->setLabel("throttle");
      slider_trim[n]  ->setValue(txi->mixer->trim_val[T_AxisMapper::THROTTLE]); 
      slider_nrate[n] ->setValue(txi->mixer->nrate_val[T_AxisMapper::THROTTLE]);       
      slider_exp[n]   ->setValue(txi->mixer->exp_val[T_AxisMapper::THROTTLE]); 
      break;      
    }
  }
  
  all_widgets->close();

  if (txi->mixer->enabled)
  {
    all_widgets->reveal();
  }
  else
  {
    all_widgets->hide();
  }
  
  enable_button = new puButton( DLG_DEF_SPACE, 
                                (NrOfAxes+1)*SLIDER_H + (NrOfAxes+1)*DLG_DEF_SPACE + BUTTON_BOX_HEIGHT + SLIDER_H,
                                DLG_DEF_SPACE + DLG_CHECK_W,
                                (NrOfAxes+1)*SLIDER_H + (NrOfAxes+1)*DLG_DEF_SPACE + BUTTON_BOX_HEIGHT + SLIDER_H + DLG_CHECK_H);
  enable_button->setLabel("Enable");
  enable_button->reveal();
  enable_button->setLabelPlace(PUPLACE_CENTERED_RIGHT);
  enable_button->setUserData(this);
  enable_button->setButtonType(PUBUTTON_VCHECK);
  enable_button->setValue(0);
  enable_button->setCallback(InputScaleEnableButtonCallback);
  if (txi->mixer->enabled)
  {
    enable_button->setValue(1);
  }
  
  close();
  setSize(2*GAP + 3*SLIDER_W + 1*LABEL_W + 2*DLG_DEF_SPACE,
          (NrOfAxes+1)*SLIDER_H + (NrOfAxes+2)*DLG_DEF_SPACE + BUTTON_BOX_HEIGHT + SLIDER_H + DLG_CHECK_H);
  setCallback(CGUIInputScaleCallback);

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

CGUIInputScaleDialog::~CGUIInputScaleDialog()
{
}

/** \brief The dialog's callback.
 *
 */
void CGUIInputScaleCallback(puObject *obj)
{
  if (obj->getIntegerValue() == CRRC_DIALOG_OK)
  {
    // Dialog left by clicking OK
    CGUIInputScaleDialog* dlg   = (CGUIInputScaleDialog*)obj;

    dlg->txi->mixer->trim_val[T_AxisMapper::AILERON]  = dlg->slider_trim[3]  ->getFloatValue();
    dlg->txi->mixer->nrate_val[T_AxisMapper::AILERON] = dlg->slider_nrate[3] ->getFloatValue();
    dlg->txi->mixer->exp_val[T_AxisMapper::AILERON]   = dlg->slider_exp[3]   ->getFloatValue();
    
    dlg->txi->mixer->trim_val[T_AxisMapper::ELEVATOR]   = dlg->slider_trim[2]  ->getFloatValue();
    dlg->txi->mixer->nrate_val[T_AxisMapper::ELEVATOR]  = dlg->slider_nrate[2] ->getFloatValue();
    dlg->txi->mixer->exp_val[T_AxisMapper::ELEVATOR]    = dlg->slider_exp[2]   ->getFloatValue();
    
    dlg->txi->mixer->trim_val[T_AxisMapper::RUDDER]   = dlg->slider_trim[1]  ->getFloatValue();
    dlg->txi->mixer->nrate_val[T_AxisMapper::RUDDER]  = dlg->slider_nrate[1] ->getFloatValue();
    dlg->txi->mixer->exp_val[T_AxisMapper::RUDDER]    = dlg->slider_exp[1]   ->getFloatValue();
    
    dlg->txi->mixer->trim_val[T_AxisMapper::THROTTLE]   = dlg->slider_trim[0]  ->getFloatValue();
    dlg->txi->mixer->nrate_val[T_AxisMapper::THROTTLE]  = dlg->slider_nrate[0] ->getFloatValue();
    dlg->txi->mixer->exp_val[T_AxisMapper::THROTTLE]    = dlg->slider_exp[0]   ->getFloatValue();
  }

  puDeleteObject(obj);
}


/** 
 *  Callback for the "enable" button.
 */
void InputScaleEnableButtonCallback(puObject *obj)
{
  puButton *btn = static_cast<puButton*>(obj);
  CGUIInputScaleDialog* dlg = static_cast<CGUIInputScaleDialog*>(btn->getUserData());
  
  int val = btn->getIntegerValue();
  dlg->txi->mixer->enabled = val;
  
  if (val)
  {
    dlg->all_widgets->reveal();
  }
  else
  {
    dlg->all_widgets->hide();
  }
}
