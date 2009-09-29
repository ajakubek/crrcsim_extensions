/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2007, 2008 Jens Wilhelm Wulf (original author)
 * Copyright (C) 2005, 2006, 2008 Jan Reucker
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
  

// implementation of class CGUIVideoDialog

#include "crrc_video.h"

#include <stdio.h>
#include <stdlib.h>

#include "../crrc_main.h"
#include "../crrc_graphics.h"
#include "../mod_landscape/crrc_scenery.h"
#include "../zoom.h"
#include "../mod_misc/lib_conversions.h"
#include "../CTime.h"
#include "../global.h"
#include "crrc_msgbox.h"

static void CGUIVideoCallback(puObject *obj);

#define BUTTON_BOX_HEIGHT   (2*DLG_DEF_SPACE+DLG_DEF_BUTTON_HEIGHT)
#define COMBO_W  150
#define COMBO_H   20
#define SLIDER_W 200
#define SLIDER_H  25
#define NUM_W     70
#define LABEL_W  130

#define TOP_WIDGET_Y  (BUTTON_BOX_HEIGHT + 4*SLIDER_H + COMBO_H + 5*DLG_DEF_SPACE + DLG_CHECK_H)
#define AUTOZOOM_MAX  (0.4)
#define FPS_MIN       (20)
#define FPS_MAX       (100)

const int stdResCnt = 6;
const int stdResX[] = {640, 800, 1024, 1280, 1280, 1280};
const int stdResY[] = {480, 600,  768,  800,  854, 1024};

CGUIVideoDialog::CGUIVideoDialog() 
            : CRRCDialog(), combo_res(NULL), slider_autozoom(NULL), slider_texoff(NULL)
{
  // save current values to detect changes in the callback
  use_fs = cfgfile->getInt("video.fullscreen.fUse", 0);
  full_x = cfgfile->getInt("video.resolution.fullscreen.x", 800);
  full_y = cfgfile->getInt("video.resolution.fullscreen.y", 600);
  window_x = cfgfile->getInt("video.resolution.window.x", 800);
  window_y = cfgfile->getInt("video.resolution.window.y", 600);

  // Generate list and set current value
  // Muss man den aktuellen Wert getrennt in die Liste tun?
  int nIdx = -1;
  {

    if (use_fs)
    {
      for (int n=0; n<stdResCnt; n++)
      {
        if (stdResX[n] == full_x && stdResY[n] == full_y)
          nIdx = n;
      }
    }
    else
    {
      for (int n=0; n<stdResCnt; n++)
      {
        if ((stdResX[n] == window_x) && (stdResY[n] == window_y))
          nIdx = n;
      }
    }
  }
  
  nResolutions = stdResCnt;
  if (nIdx == -1)
     nResolutions++;

  resolutions = (char**)malloc(sizeof(char**) * (nResolutions+1));

  // set list of resolutions 
  {
    std::string name;
    
    for (int n=0; n<nResolutions; n++)
    {
      if (n == stdResCnt)
      {
        if (use_fs)
          name = cfgfile->getString("video.resolution.fullscreen.x") + "x" + cfgfile->getString("video.resolution.fullscreen.y");
        else
          name = cfgfile->getString("video.resolution.window.x") + "x" + cfgfile->getString("video.resolution.window.y");
      }
      else
      {
        name = itoStr(stdResX[n], ' ', 1) + "x" + itoStr(stdResY[n], ' ', 1);
      }
      resolutions[n] = (char*)malloc(sizeof(char**) * (name.length()+1));
      strcpy(resolutions[n], name.c_str());
    }      
    resolutions[nResolutions] = (char*)0;
  }
            
  combo_res       = new puaComboBox(LABEL_W + DLG_DEF_SPACE, 
                                   TOP_WIDGET_Y - DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H,
                                   LABEL_W + COMBO_W,
                                   TOP_WIDGET_Y - DLG_DEF_SPACE - DLG_CHECK_H,
                                   resolutions, true);
  combo_res->setLabelPlace(PUPLACE_LOWER_LEFT);
  combo_res->setLabel("Resolution");
  if (nIdx == -1)
    combo_res->setCurrentItem(nResolutions - 1);
  else
    combo_res->setCurrentItem(nIdx);
  combo_res->setUserData(this);
  combo_res->reveal();
  combo_res->activate();
  
  fs_check = new puButton(LABEL_W + DLG_DEF_SPACE,
                          TOP_WIDGET_Y - DLG_CHECK_H,
                          LABEL_W + DLG_DEF_SPACE + DLG_CHECK_W,
                          TOP_WIDGET_Y);
  fs_check->setLabelPlace(PUPLACE_CENTERED_LEFT);
  fs_check->setLabel("Fullscreen");
  fs_check->setButtonType(PUBUTTON_VCHECK);
  if (use_fs)
    fs_check->setValue(1);
  else
    fs_check->setValue(0);
  fs_check->reveal();
  
  slider_autozoom = new crrcSlider( LABEL_W + DLG_DEF_SPACE,
                                    TOP_WIDGET_Y - 2*DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H - SLIDER_H,
                                    LABEL_W + DLG_DEF_SPACE + SLIDER_W,
                                    TOP_WIDGET_Y - 2*DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H,
                                    NUM_W);
  slider_autozoom->setLabelPlace(PUPLACE_CENTERED_LEFT);
  slider_autozoom->setLabel("autozoom");
  slider_autozoom->setMaxValue(AUTOZOOM_MAX);
  slider_autozoom->setStepSize(0.01);
  slider_autozoom->setValue(flAutozoom);
  
  // get a pointer to the video settings
  SimpleXMLTransfer *video = cfgfile->getChild("video", true);

  // frame rate limiter setting
  int iFPS = video->attributeAsInt("fps", DEFAULT_GAME_SPEED);
  slider_fps = new crrcSlider(LABEL_W + DLG_DEF_SPACE,
                              TOP_WIDGET_Y - 3*DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H - 2*SLIDER_H,
                              LABEL_W + DLG_DEF_SPACE + SLIDER_W,
                              TOP_WIDGET_Y - 3*DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H - SLIDER_H,
                              NUM_W);
  slider_fps->setLabelPlace(PUPLACE_CENTERED_LEFT);
  slider_fps->setLabel("FPS limit");
  slider_fps->setMinValue(FPS_MIN);
  slider_fps->setMaxValue(FPS_MAX);
  slider_fps->setStepSize(1);
  slider_fps->setValue(iFPS);

  // skybox texture offset setting
  dOldSkyTexOff = video->getDouble("skybox.texture_offset", DEFAULT_SKYBOX_TEXTURE_OFFSET);
  dOldSkyTexOff = dOldSkyTexOff * 10000.0;
  slider_texoff = new crrcSlider( LABEL_W + DLG_DEF_SPACE,
                                  TOP_WIDGET_Y - 4*DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H - 3*SLIDER_H,
                                  LABEL_W + DLG_DEF_SPACE + SLIDER_W,
                                  TOP_WIDGET_Y - 4*DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H - 2*SLIDER_H,
                                  NUM_W);
  slider_texoff->setLabelPlace(PUPLACE_CENTERED_LEFT);
  slider_texoff->setLabel("Skybox fix");
  slider_texoff->setMinValue(0);
  slider_texoff->setMaxValue(99);
  slider_texoff->setStepSize(1);
  slider_texoff->setValue((float)dOldSkyTexOff);

  // sloppy camera setting
  slider_sloppycam = new crrcSlider( LABEL_W + DLG_DEF_SPACE,
                                     TOP_WIDGET_Y - 5*DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H - 4*SLIDER_H,
                                     LABEL_W + DLG_DEF_SPACE + SLIDER_W,
                                     TOP_WIDGET_Y - 5*DLG_DEF_SPACE - DLG_CHECK_H - COMBO_H - 3*SLIDER_H,
                                     NUM_W);
  slider_sloppycam->setLabelPlace(PUPLACE_CENTERED_LEFT);
  slider_sloppycam->setLabel("Sloppy camera");
  slider_sloppycam->setMinValue(0);
  slider_sloppycam->setMaxValue(0.3);
  slider_sloppycam->setStepSize(0.05);
  slider_sloppycam->setValue(flSloppyCam);


  /**
   *  \todo widgets for color depth, shading, textures and mipmaps
   */
  
  
  close();
  setSize(SLIDER_W + LABEL_W + 2*DLG_DEF_SPACE, 
          TOP_WIDGET_Y + 2*DLG_DEF_SPACE);
  setCallback(CGUIVideoCallback);
  
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

CGUIVideoDialog::~CGUIVideoDialog()
{
  /** 
   *  \todo Destructor must clean up the (m)allocated menu entries!
   */
  
}


/** \brief The dialog's callback.
 *
 */
void CGUIVideoCallback(puObject *obj)
{
  CGUIVideoDialog* dlg = (CGUIVideoDialog*)obj;

  if (obj->getIntegerValue() == CRRC_DIALOG_OK)
  {
    // Dialog left by clicking OK
    {
      std::string resolution = dlg->combo_res->getStringValue();
      if (resolution.find('x') != std::string::npos)
      {
        // String might be valid
        std::string resx = resolution.substr(0, resolution.find('x'));
        std::string resy = resolution.substr(resolution.find('x')+1);
        int iResX = atoi(resx.c_str());
        int iResY = atoi(resy.c_str());
        
        int new_use_fs = dlg->fs_check->getIntegerValue();
        
        // now compare the new values to the old values to detect changes
        bool values_changed = false;
        
        if (new_use_fs != dlg->use_fs)
        {
          values_changed = true;
        }
        else
        {
          if (dlg->use_fs)
          {
            if ((iResX != dlg->full_x) || (iResY != dlg->full_y))
            {
              values_changed = true;
            }
          }
          else
          {
            if ((iResX != dlg->window_x) || (iResY != dlg->window_y))
            {
              values_changed = true;
            }
          }
        }

        /* change detected? then store all values */
        if (values_changed)
        {
          std::cout << "Screen resolution changed to " << resx << "x" << resy;
          std::cout << ((new_use_fs) ? " (fullscreen)" : " (windowed)") << std::endl;
          if (new_use_fs)
          {
            cfgfile->setAttributeOverwrite("video.resolution.fullscreen.x", resx);
            cfgfile->setAttributeOverwrite("video.resolution.fullscreen.y", resy);
          }
          else
          {
            cfgfile->setAttributeOverwrite("video.resolution.window.x", resx);
            cfgfile->setAttributeOverwrite("video.resolution.window.y", resy);
          }

          cfgfile->setAttributeOverwrite("video.fullscreen.fUse", new_use_fs);
          
          #ifdef linux
          // On Linux we can switch the mode on-the-fly
          video_setup(atoi(resx.c_str()), atoi(resy.c_str()), new_use_fs);
          #else
          // other platforms need a restart
          new CGUIMsgBox("Please save your configuration and restart CRRCSim!");
          #endif
        }
      }
    }
    flAutozoom = dlg->slider_autozoom->getFloatValue();
    
    Uint16 fps = dlg->slider_fps->getIntegerValue();
    crrc_time->setGameSpeed(fps);
    crrc_time->putBackIntoCfg(cfgfile);
    
    double dSkyTexOff = 0.0001 * dlg->slider_texoff->getFloatValue();
    if (dSkyTexOff != dlg->dOldSkyTexOff)
    {
      cfgfile->setAttributeOverwrite( "video.skybox.texture_offset",
                                      ftoStr(dSkyTexOff, 1, 4));
      Global::scenery->setupSky();
    }
    //
    flSloppyCam = dlg->slider_sloppycam->getFloatValue();
    cfgfile->setAttributeOverwrite("video.camera.sloppy",
                                    doubleToString(flSloppyCam));
  }
  
  puDeleteObject(obj);
}
