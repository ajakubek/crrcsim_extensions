/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2008 Jens Wilhelm Wulf (original author)
 * Copyright (C) 2005, 2006, 2008 Jan Reucker
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
  

// implementation of class CGUIAudioDialog

#include "crrc_audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "../global.h"
#include "util.h"
#include "../crrc_main.h"
#include "../mod_misc/lib_conversions.h"
#include "../mod_misc/SimpleXMLTransfer.h"
#include "../crrc_sound.h"

static void CGUIAudioCallback(puObject *obj);

#define BUTTON_BOX_HEIGHT   (2*DLG_DEF_SPACE+DLG_DEF_BUTTON_HEIGHT)
#define SLIDER_W 330
#define SLIDER_H 25
#define COMBO_H  20
#define COMBO_W  330
#define LABEL_W  170
#define NUM_W    70

const char* CGUIAudioDialog::samplerates[] = {"22050 Hz", "44100 Hz", "48000 Hz", NULL};
const char* CGUIAudioDialog::enginesound[] = {"Off", "On", "Always on", NULL};

CGUIAudioDialog::CGUIAudioDialog() 
            : CRRCDialog()
{
  // Create Widgets
  comboSampleRate = new puaComboBox(LABEL_W + DLG_DEF_SPACE, BUTTON_BOX_HEIGHT + 3* DLG_DEF_SPACE + 2* SLIDER_H,
                                   LABEL_W + COMBO_W,       BUTTON_BOX_HEIGHT + 3* DLG_DEF_SPACE + 2* SLIDER_H + COMBO_H,
                                   (char**) samplerates, false);
  comboSampleRate->setLabelPlace(PUPLACE_CENTERED_LEFT);
  comboSampleRate->setLabel("Sample Rate");
  {
    int         nIdx = 0;
    std::string cr   = cfgfile->getString("sound.samplerate");
    std::string tmp;
    
    for (int n=0; n<3; n++)
    {
      tmp = samplerates[n];
      if (tmp.find(cr) == 0)
        nIdx = n;
    }
    comboSampleRate->setCurrentItem(nIdx);
  }
  comboSampleRate->reveal();
  
  comboEngine = new puaComboBox(LABEL_W + DLG_DEF_SPACE, BUTTON_BOX_HEIGHT + 2* DLG_DEF_SPACE + 1* SLIDER_H,
                               LABEL_W + COMBO_W,       BUTTON_BOX_HEIGHT + 2* DLG_DEF_SPACE + 1* SLIDER_H + COMBO_H,
                               (char**) enginesound, false);
  comboEngine->setLabelPlace(PUPLACE_CENTERED_LEFT);
  comboEngine->setLabel("Engine sound");
  comboEngine->setCurrentItem(cfgfile->getInt("sound.throttle.mode", 1));
  comboEngine->hide();
  
  slider_variovol = new puSlider(LABEL_W + DLG_DEF_SPACE,  BUTTON_BOX_HEIGHT + 1* DLG_DEF_SPACE + 0* SLIDER_H,
                                 SLIDER_W,
                                 FALSE, 
                                 SLIDER_H);
  slider_variovol->setLabelPlace(PUPLACE_CENTERED_LEFT);
  slider_variovol->setLabel("Variometer volume");
  slider_variovol->setSliderFraction(0.05);
  slider_variovol->setMinValue(0);
  slider_variovol->setMaxValue(16);
  slider_variovol->setStepSize(1);
  slider_variovol->setValue(cfgfile->getInt("sound.variometer.vol"));
  
  close();
  setSize(SLIDER_W + LABEL_W + 2*DLG_DEF_SPACE, 
          3*SLIDER_H + 4*DLG_DEF_SPACE + BUTTON_BOX_HEIGHT);
  setCallback(CGUIAudioCallback);
  
  // center the dialog on screen
  int wwidth, wheight;
  int current_width = getABox()->max[0] - getABox()->min[0];
  int current_height = getABox()->max[1] - getABox()->min[1];
  puGetWindowSize(&wwidth, &wheight);
  setPosition(wwidth/2 - current_width/2, wheight*2/3 - current_height);

  reveal();
}


/**
 * Destroy the dialog.
 */

CGUIAudioDialog::~CGUIAudioDialog()
{
}

/** \brief The dialog's callback.
 *
 */
void CGUIAudioCallback(puObject *obj)
{
  if (obj->getIntegerValue() == CRRC_DIALOG_OK)
  {
    CGUIAudioDialog *dlg = (CGUIAudioDialog*)obj;
    // Dialog left by clicking OK
    cfgfile->setAttributeOverwrite("sound.variometer.vol",
                                   dlg->slider_variovol->getStringValue());
    //~ cfgfile->setAttributeOverwrite("sound.throttle.mode",
                                   //~ dlg->comboEngine->getCurrentItem());
    {
      std::string SR = CGUIAudioDialog::samplerates[dlg->comboSampleRate->getCurrentItem()];
      
      int nSR  = atoi(SR.substr(0, SR.find(' ')).c_str());
      cfgfile->setAttributeOverwrite("sound.samplerate", nSR);
      
      int varvol = dlg->slider_variovol->getIntegerValue() << 3;
      printf("Setting vario volume to %d\n", varvol);
      Global::soundserver->setChannelVolume(vario_sound_channel, varvol);
    }    
  }
 
  puDeleteObject(obj);
}
