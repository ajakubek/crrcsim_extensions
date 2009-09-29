/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2006, 2008 Jan Reucker (original author)
 * Copyright (C) 2006, 2008 Jens Wilhelm Wulf
 * Copyright (C) 2008, 2009 Joel Lienard
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
  

// implementation of class CGUILocationDialog

#include "../global.h"
#include "../SimStateHandler.h"
#include "../mod_landscape/crrc_scenery.h"
#include "crrc_gui_main.h"
#include "crrc_location.h"
#include "../crrc_main.h"
#include "../mod_windfield/windfield.h"
#include "../crrc_graphics.h"
#include "../mod_misc/filesystools.h"
#include "util.h"

#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <string>

static void CGUILocationCallback(puObject *obj);

CGUILocationDialog::CGUILocationDialog() 
            : CRRCDialog(), cbox(NULL)
{
  DIR *dir=NULL;
  struct dirent *ent;
  const char* curLocName = Global::scenery->getName();
  int curLocIndex = -1;
  int lists_index = 0;
  std::vector<std::string> paths;
  std::vector<std::string> extlist;
  std::vector<std::string> fileslist;
  std::vector<std::string> locationslist;
  extlist.push_back("xml");
  
  T_Config::getLocationDirs(paths);
  
  for (unsigned int i = 0; i < paths.size(); i++)
  {
    if ((dir = opendir(paths[i].c_str())) == NULL)
    {
      #ifdef DEBUG_GUI
      std::cerr << "createFileList(): unable to open directory " << paths[i];
      std::cerr << std::endl; 
      #endif
    }
    else
    {
     while ((ent = readdir(dir)) != NULL)
      {
        std::string tmp;
        bool        fMatch = false;
        
        tmp = ent->d_name;
        
        for (unsigned int n=0; n<extlist.size() && fMatch == false; n++)
        {
          if (T_GUI_Util::checkExtension(tmp, extlist[n]))
            fMatch = true;
        }
                   
        if (fMatch)
        {
        std::string fullpath = paths[i] + "/" + tmp;
          //std::cout << fullpath << std::endl;
        SimpleXMLTransfer *loc = NULL;
        bool ok=false;
        std::string name;
        try
        {
          loc = new SimpleXMLTransfer(fullpath);
          name = loc->getChild("name")->getContentString();
          ok=true;
        }
        catch (XMLException e)
        {
          std::cerr << "Caught XML exception in CGUIlocationSelectDialog" << std::endl;
        }
        if(ok)
        {
          fileslist.push_back(fullpath);
          locationslist.push_back(name);
          if(name.compare(curLocName)==0) curLocIndex= lists_index;
          lists_index++;
        }
        delete loc;
        }
      }
      closedir(dir);
    }
  }
  filesList = T_GUI_Util::loadnames(fileslist, filesListSize);
  locationsList = T_GUI_Util::loadnames(locationslist, locationsListSize);

#define LIST_WIDGET_HEIGHT  (200)
#define LIST_WIDGET_WIDTH   (200)
#define BUTTON_BOX_HEIGHT   (2*DLG_DEF_SPACE+DLG_DEF_BUTTON_HEIGHT)

 
  cbox = new puaScrListBox (DLG_DEF_SPACE,
                              BUTTON_BOX_HEIGHT,
                              LIST_WIDGET_WIDTH,
                              LIST_WIDGET_HEIGHT, 
                              locationsList);

  cbox->setLabelPlace(PUPLACE_TOP_LEFT);
  cbox->setLabel("Select location:");
  close();
  setSize(LIST_WIDGET_WIDTH + 2*DLG_DEF_SPACE, 
          BUTTON_BOX_HEIGHT + LIST_WIDGET_HEIGHT + 4*DLG_DEF_SPACE  );
  setUserData(this);
  setCallback(CGUILocationCallback);
  cbox->setValue(curLocIndex);
 
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

CGUILocationDialog::~CGUILocationDialog()
{
  T_GUI_Util::freenames(locationsList, locationsListSize);
  T_GUI_Util::freenames(filesList, filesListSize);

}

std::string CGUILocationDialog::getLocation() const
{
  std::string loc = cbox->getStringValue();
  return loc;
}

int CGUILocationDialog::getLocationId() const
{
int id = cbox->getIntegerValue();
return id;
}


bool CGUILocationDialog::saveSelection() const
{
 const char* curLocName = Global::scenery->getName();
 int id = getLocationId();
 std::string filename = filesList[id];
 std::string newLocname = locationsList[id];
 if(newLocname.compare(curLocName))
 {
      cfg->setLocation(filename.c_str(), cfgfile);
      
      
	  Scenery* new_scenery = loadScenery(FileSysTools::getDataPath(filename).c_str());
    if(new_scenery)
			{
			clear_wind_field();
			delete Global::scenery;
			Global::scenery = new_scenery;
			}
      cfg->read(cfgfile);
      setWindowTitleString();
      *player_pos = Global::scenery->getPlayerPosition();
      Init_mod_windfield();
      Global::Simulation->reset();
  }
return true;
}

/** \brief The dialog's callback.
 *
 *  Load the new location.
 */
void CGUILocationCallback(puObject *obj)
{
  if (obj->getIntegerValue() == CRRC_DIALOG_OK)
  {
    CGUILocationDialog *dlg = (CGUILocationDialog*)obj->getUserData();
    dlg->saveSelection();
  }
  puDeleteObject(obj);
  Global::gui->hide();
}
