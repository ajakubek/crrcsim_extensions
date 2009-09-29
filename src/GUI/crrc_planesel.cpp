/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2006, 2007, 2008 Jan Reucker (original author)
 * Copyright (C) 2005-2006, 2008-2009 Jens Wilhelm Wulf
 * Copyright (C) 2007 Tom Willis
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
  

// implementation of class CGUIPlaneSelectDialog

#include "../global.h"
#include "../aircraft.h"
#include "crrc_gui_main.h"
#include "crrc_planesel.h"
#include "../crrc_main.h"
#include "../crrc_loadair.h"
#include "../mod_misc/filesystools.h"
#include "../mod_misc/lib_conversions.h"
#include "../mod_fdm/fdm.h"
#include "../crrc_graphics.h"
#include "../mod_fdm/formats/airtoxml.h"
#include "../mod_fdm/xmlmodelfile.h"
#include "util.h"

#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <string>

static void CGUIPlaneSelCallback(puObject *obj);
static void CGUIPlaneSelCategoryCallback(puObject *obj);
static void CGUIPlaneSelPlaneListCallback(puObject *obj);
static void CGUIPlaneSelGraphicsCallback(puObject *obj);

#define LIST_WIDGET_HEIGHT  (200)
#define LIST_WIDGET_WIDTH   (200)
#define CONF_SEL_WIDTH      (200)
#define PREVIEW_HEIGHT      (200)
#define PREVIEW_WIDTH       (200)
#define DESCRIPTION_HEIGHT  (75)
#define BUTTON_BOX_HEIGHT   (2*DLG_DEF_SPACE+DLG_DEF_BUTTON_HEIGHT)

//#define DEBUG_GUI


CGUIPlaneSelectDialog::CGUIPlaneSelectDialog() 
            : CRRCDialog(),
              cat(NULL), catList(NULL), catListSize(0),
              planes(NULL), planeList(NULL), planeListSize(0),
              gbox(NULL), optsGraphics(NULL), nOptsGraphics(0), optsGrpGraphics(NULL),
              cbox(NULL), optsConfig(NULL), nOptsConfig(0), optsGrpConfig(NULL),
              location_label(NULL)
{
  // height of a text label
  int msg_height  = puGetDefaultLegendFont().getStringHeight("jX")
                  + puGetDefaultLegendFont().getStringDescender()
                  + PUSTR_TGAP + PUSTR_BGAP;
  
  // top of the list box
  int top_of_listbox = BUTTON_BOX_HEIGHT + msg_height + DESCRIPTION_HEIGHT + 3*DLG_DEF_SPACE + LIST_WIDGET_HEIGHT;

  // the file location text label
  location_label = new puText(DLG_DEF_SPACE, 
                              BUTTON_BOX_HEIGHT + DESCRIPTION_HEIGHT + 2*DLG_DEF_SPACE);
  location_label->setLabel("File: /path/to/my/airplane.xml");

  // the description box
  description = new puaLargeInput( DLG_DEF_SPACE,
                                  BUTTON_BOX_HEIGHT + DLG_DEF_SPACE,
                                  4*DLG_DEF_SPACE + LIST_WIDGET_WIDTH + CONF_SEL_WIDTH + PREVIEW_WIDTH,
                                  DESCRIPTION_HEIGHT,
                                  1,    // num of arrow pairs
                                  16,   // slider width
                                  0);   // wrap text
  description->disableInput();
  description->setText("This is a short description of the selected model.");

  // Use launch default check box
  check_usedefault = new puButton(3*DLG_DEF_SPACE + LIST_WIDGET_WIDTH,
                                  top_of_listbox - 3*DLG_DEF_BUTTON_HEIGHT - DLG_CHECK_H - 4*DLG_DEF_SPACE,
                                  3*DLG_DEF_SPACE + LIST_WIDGET_WIDTH + DLG_CHECK_W,
                                  top_of_listbox - 3*DLG_DEF_BUTTON_HEIGHT - 4*DLG_DEF_SPACE);
  check_usedefault->setButtonType(PUBUTTON_VCHECK);
  check_usedefault->setLabelPlace(PUPLACE_CENTERED_RIGHT);
  check_usedefault->setLabel("Load launch default");
  check_usedefault->setValue((int)cfgfile->getInt("airplane.use_default_launch", 1));
  check_usedefault->greyOut();

  // Graphics selection box
  gbox = new puaComboBox(3*DLG_DEF_SPACE + LIST_WIDGET_WIDTH,
                        top_of_listbox - DLG_DEF_BUTTON_HEIGHT,
                        3*DLG_DEF_SPACE + LIST_WIDGET_WIDTH + CONF_SEL_WIDTH,
                        top_of_listbox,
                        optsGraphics,
                        false);
  gbox->setLabelPlace(PUPLACE_TOP_LEFT);
  gbox->setLabel("Select graphics:");
  gbox->setCurrentItem(0);
  gbox->setUserData(this);
  gbox->setCallback(CGUIPlaneSelGraphicsCallback);
  
  // Configuration selection box
  cbox = new puaComboBox( 3*DLG_DEF_SPACE + LIST_WIDGET_WIDTH,
                         top_of_listbox - 3*DLG_DEF_BUTTON_HEIGHT - DLG_DEF_SPACE,
                         3*DLG_DEF_SPACE + LIST_WIDGET_WIDTH + CONF_SEL_WIDTH,
                         top_of_listbox - 2*DLG_DEF_BUTTON_HEIGHT - DLG_DEF_SPACE,
                         optsConfig,
                         false);
  cbox->setLabelPlace(PUPLACE_TOP_LEFT);
  cbox->setLabel("Select config:");
  cbox->setCurrentItem(0);

  // Airplane selection list
  planes = new puaScrListBox (DLG_DEF_SPACE,
                              BUTTON_BOX_HEIGHT + msg_height + DESCRIPTION_HEIGHT + 3*DLG_DEF_SPACE,
                              LIST_WIDGET_WIDTH,
                              LIST_WIDGET_HEIGHT, 
                              NULL);
  planes->setLabelPlace(PUPLACE_TOP_LEFT);
  planes->setLabel("Select airplane:");
  planes->setUserData(this);
  planes->setCallback(CGUIPlaneSelPlaneListCallback);

  // preview widget
  preview = new puaGLPreview ( 5*DLG_DEF_SPACE + LIST_WIDGET_WIDTH + CONF_SEL_WIDTH,
                               BUTTON_BOX_HEIGHT + msg_height + DESCRIPTION_HEIGHT + 3*DLG_DEF_SPACE,
                               5*DLG_DEF_SPACE + LIST_WIDGET_WIDTH + CONF_SEL_WIDTH + PREVIEW_WIDTH,
                               BUTTON_BOX_HEIGHT + msg_height + DESCRIPTION_HEIGHT + 3*DLG_DEF_SPACE + PREVIEW_HEIGHT);
  preview->setLabelPlace(PUPLACE_TOP_LEFT);
  preview->setLabel("Preview:");

  // Airplane category selection combo box
  updateCategories();
  cat = new puaComboBox( DLG_DEF_SPACE,
                        BUTTON_BOX_HEIGHT + 6*DLG_DEF_SPACE 
                          + LIST_WIDGET_HEIGHT + msg_height 
                          + DESCRIPTION_HEIGHT,
                        LIST_WIDGET_WIDTH + 10,
                        BUTTON_BOX_HEIGHT + 6*DLG_DEF_SPACE 
                          + LIST_WIDGET_HEIGHT + msg_height 
                          + DESCRIPTION_HEIGHT + DLG_DEF_BUTTON_HEIGHT,
                        catList,
                        false);
  cat->setLabelPlace(PUPLACE_TOP_LEFT);
  cat->setLabel("Select category:");
  cat->setCallback(CGUIPlaneSelCategoryCallback);
  cat->setUserData(this);
  cat->setCurrentItem(0);

  // find and highlight the current airplane
  SimpleXMLTransfer* ap = cfgfile->getChild("airplane");
  
  int iCurrentGraphics = ap->attributeAsInt("graphics", 0);
  int iCurrentConfig   = ap->attributeAsInt("config", 0);
  std::string sCurrentPlane = ap->getString("file", "");

  if (sCurrentPlane != "")
  {
    //std::cout << "Current plane: " << sCurrentPlane << std::endl;

    int index = -1;
    
    for (std::vector<std::string>::size_type i = 0; i < plane_paths.size(); i++)
    {
      if (plane_paths[i] == sCurrentPlane)
      {
        index = i;
        break;
      }
    }
    if (index >= 0)
    {
      planes->setValue(index);
      index -= 2;
      if (index < 0)
      {
        index = 0;
      }
      planes->setTopItem(index);        /// slider has to be adjusted, too !!!!!!
      updateFileInfo();
      updatePreview();
      gbox->setCurrentItem(iCurrentGraphics);
      cbox->setCurrentItem(iCurrentConfig);
    }
  }

  // finalize the dialog
  close();
  setSize(LIST_WIDGET_WIDTH + CONF_SEL_WIDTH + PREVIEW_WIDTH + 6*DLG_DEF_SPACE, 
          BUTTON_BOX_HEIGHT + LIST_WIDGET_HEIGHT + 2*DLG_DEF_BUTTON_HEIGHT + 6*DLG_DEF_SPACE + msg_height + DESCRIPTION_HEIGHT);
  setCallback(CGUIPlaneSelCallback);
  
  // center the dialog on screen
  int wwidth, wheight;
  int current_width = getABox()->max[0] - getABox()->min[0];
  int current_height = getABox()->max[1] - getABox()->min[1];
  puGetWindowSize(&wwidth, &wheight);
  setPosition(wwidth/2 - current_width/2, (wheight - current_height)/2);

  reveal();
}


/**
 * Destroy the dialog.
 */
CGUIPlaneSelectDialog::~CGUIPlaneSelectDialog()
{
  T_GUI_Util::freenames(catList, catListSize);
  T_GUI_Util::freenames(planeList, planeListSize);
  cleanUpConfigAndGraphics();
}


/** 
 *  Scans all model directories for .air and .xml files
 *  and adds their complete paths to the mfiles vector.
 */
void CGUIPlaneSelectDialog::createMFileList(std::vector<std::string>& filelist)
{
  std::vector<std::string> paths;
  DIR *dir;
  struct dirent *ent;
  std::vector<std::string> extlist;

  filelist.clear();

  // allowed extensions:
  extlist.push_back("xml");
  extlist.push_back("air");
  
  T_Config::getModelDirs(paths);

  for (unsigned int i = 0; i < paths.size(); i++)
  {
    if ((dir = opendir(paths[i].c_str())) == NULL)
    {
      #ifdef DEBUG_GUI
      std::cerr << "createMFileList(): unable to open directory " << paths[i];
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
          filelist.push_back(fullpath);
        }
      }
      closedir(dir);
    }
  }
}


/**
 *  Update the list for the category combo box
 *
 */
void CGUIPlaneSelectDialog::updateCategories()
{
  std::vector<std::string> cats;
  std::vector<std::string> mfiles;

  createMFileList(mfiles);

  if (catList != NULL)
  {
    T_GUI_Util::freenames(catList, catListSize);
    catList = NULL;
    catListSize = 0;
  }

  cats.push_back("All models");
  
  for (unsigned int i = 0; i < mfiles.size(); i++)
  {
    SimpleXMLTransfer *model = NULL;
    try
    {
      model = new SimpleXMLTransfer(mfiles[i]);
      SimpleXMLTransfer *xmlcateg = model->getChild("categories");
      int numcats = xmlcateg->getChildCount();
      
      for (int k = 0; k < numcats; k++)
      {
        std::string currcat = xmlcateg->getChildAt(k)->getContentString();
        bool found = false;
        
        std::vector<std::string>::iterator it = cats.begin();
        it++;   // Allways start behind "All models"

        while ((it != cats.end()) && (!found))
        {
          int c = currcat.compare(*it);
          
          if (c == 0)
          {
            // found a duplicate
            found = true;
          }
          else if (c < 0)
          {
            // next item is lexicographically greater, insert
            // current item right here
            cats.insert(it, currcat);
            found = true;
          }
          it++;
        }
        
        if (!found)
        {
          // new category, lexicographically greater than all
          // existing categories
          cats.push_back(currcat);
        }
      }
    }
    catch (XMLException e)
    {
      // don't worry if child does not exist or a file isn't XML
    }
    delete model;
  }

  catList = T_GUI_Util::loadnames(cats, catListSize);
  
}


/**
 * Get all planes of a given category.
 * \param planes list of planes with complete path
 * \param category the selected category
 */
void CGUIPlaneSelectDialog::getPlanesByCategory(std::vector<std::string>& planes,
                                                std::string category)
{
  std::vector<std::string> mfiles;
  
  planes.clear();
  createMFileList(mfiles);
  
  if (category == "All models")
  {
    planes = mfiles;
  }
  else
  {
    for (unsigned int i = 0; i < mfiles.size(); i++)
    {
      SimpleXMLTransfer *model = NULL;
      try
      {
        model = new SimpleXMLTransfer(mfiles[i]);
        SimpleXMLTransfer *xmlcateg = model->getChild("categories");
        int numcats = xmlcateg->getChildCount();
        
        for (int k = 0; k < numcats; k++)
        {
          std::string currcat = xmlcateg->getChildAt(k)->getContentString();
          if (currcat == category)
          {
            planes.push_back(mfiles[i]);
            break;
          }
        }
      }
      catch (XMLException e)
      {
        // don't worry if child does not exist or a file isn't XML
      }
      delete model;
    }
  }
}


/**
 *  Update the list of planes shown in the dialog,
 *  depending on the selected category
 */
void CGUIPlaneSelectDialog::updatePlaneList()
{
  std::vector<std::string> pnames;
  std::vector<std::string> paths_temp;
  
  plane_paths.clear();
  
  if (planeList != NULL)
  {
    T_GUI_Util::freenames(planeList, planeListSize);
    planeList = NULL;
    planeListSize = 0;
  }

  // get a list of all planes in the category selected by the combo box
  getPlanesByCategory(paths_temp, cat->getStringValue());
  
  // walk through temporary array and copy to pnames and plane_paths,
  // sorted by model names
  for (unsigned int i = 0; i < paths_temp.size(); i++)
  {
    std::string name = getModelName(paths_temp[i]);
    
    std::vector<std::string>::iterator name_it = pnames.begin();
    std::vector<std::string>::iterator path_it = plane_paths.begin();
    bool found = false;
    
    while ((name_it != pnames.end()) && !found)
    {
      int c = name.compare(*name_it);
      
      if (c <= 0)
      {
        // duplicate name or next element is lexicographically greater,
        // insert current item here
        pnames.insert(name_it, name);
        plane_paths.insert(path_it, paths_temp[i]);
        found = true;
      }
      
      name_it++;
      path_it++;
    }
    
    if (!found)
    {
      // current item goes to the end of the list
      pnames.push_back(name);
      plane_paths.push_back(paths_temp[i]);
    }
  }

  // create a list of all names that can be used by the PUI widget
  planeList = T_GUI_Util::loadnames(pnames, planeListSize);
  
  // update the PUI listbox 
  planes->newList(planeList);
  planes->setValue((int)0);
  planes->invokeCallback();
}


/**
 *  Get the name of an airplane model. This should be specified
 *  in the XML file. If not, the name will be derived from the
 *  file name.
 *
 *  \param path model file with full path
 *  \return name of the model
 */
std::string CGUIPlaneSelectDialog::getModelName(std::string path)
{
  std::string name = "";
  SimpleXMLTransfer *model = NULL;
  
  // first we try to read the name from the XML file
  try
  {
    model = new SimpleXMLTransfer(path);
    name = model->getChild("name.en")->getContentString();
  }
  catch (XMLException e)
  {
    #ifdef DEBUG_GUI
    std::cerr << "Caught XML exception in CGUIPlaneSelectDialog::getModelName:" << std::endl;
    std::cerr << "  " << e.what() << std::endl;
    std::cerr << "  file " << path << std::endl;
    #endif
  }
  delete model;

  // name still empty? then we had no luck with the XML file...
  if (name == "")
  {
    std::string::size_type dot_index;
    std::string::size_type slash_index;
    
    dot_index = path.find_last_of('.');
    if (dot_index == std::string::npos)
    {
      // path did not contain a dot, take everything up to the end
      dot_index = path.length() - 1;
    }

    slash_index = path.find_last_of('/');
    if (slash_index == std::string::npos)
    {
      // path did not contain a slash, take everything from the beginning
      slash_index = 0;
    }
    
    if (slash_index >= dot_index)
    {
      // something is terribly wrong, there is a slash after the last dot!
      // just take the whole path and let the caller decide what to do...
      slash_index = 0;
      dot_index = path.length() -1;
    }
    
    name = path.substr(slash_index + 1, (dot_index - slash_index) - 1);
  }
  
  return name;
}


/**
 *  Free the configuration and graphics selection combo boxes
 *  and their related lists.
 */
void CGUIPlaneSelectDialog::cleanUpConfigAndGraphics()
{
  if (optsGraphics != NULL)
  {
    T_GUI_Util::freenames(optsGraphics, nOptsGraphics);
    optsGraphics = NULL;
    nOptsGraphics = 0;
    gbox->newList(optsGraphics);
  }
  
  if (optsConfig != NULL)
  {
    T_GUI_Util::freenames(optsConfig, nOptsConfig);
    optsConfig = NULL;
    nOptsGraphics = 0;
    cbox->newList(optsGraphics);
  }
}


/**
 *  Update the dialog if a new file is selected.
 *
 */
void CGUIPlaneSelectDialog::updateFileInfo()
{
  // Get the currently selected item and check if the value makes sense
  int entry = planes->getIntegerValue();
  if ((entry >= 0) && (entry < planes->getNumItems()))
  {
    // Update the location label
    location_label_string = "File: ";
    location_label_string += plane_paths[entry];
    location_label->setLabel(location_label_string.c_str());
    
    // Update the description box and launch default checkbox
    description_string = "";
    try
    {
      SimpleXMLTransfer *model = new SimpleXMLTransfer(plane_paths[entry]);
      description_string = model->getChild("description.en")->getContentString();
      // The test for a "launch" child shouldn't throw an exception,
      // so we let it create the child if the test fails and then
      // test for its children to see if it was a real "launch" tag
      SimpleXMLTransfer *launch = model->getChild("launch", true);
      if (launch->getChildCount() > 0)
      {
        check_usedefault->setValue((int)cfgfile->getInt("airplane.use_default_launch", 1));
        check_usedefault->activate();
      }
      else
      {
        check_usedefault->setValue(1);  // always use "default" if there are no options
        check_usedefault->greyOut();
      }
      delete model;
    }
    catch (XMLException e)
    {
      #ifdef DEBUG_GUI
      std::cerr << "Caught XML exception in CGUIPlaneSelectDialog::updateFileInfo:" << std::endl;
      std::cerr << "  " << e.what() << std::endl;
      std::cerr << "  file " << plane_paths[entry] << std::endl;
      #endif
    }
    description_string = T_GUI_Util::trimWhitespace(description_string);
    description_string = T_GUI_Util::breakLines(description_string, 64);
    description->setText(description_string.c_str());
    description->setTopLineInWindow(0);

    // clean up the configuration option boxes
    cleanUpConfigAndGraphics();
    
    // Check if there are any configuration options
    try
    {
      SimpleXMLTransfer* xml = new SimpleXMLTransfer(plane_paths[entry]);

      if (XMLModelFile::ListOptions(xml))
      {
        optsGrpGraphics = xml->getChild("options.graphics");
        optsGrpConfig   = xml->getChild("options.config");
        optsGraphics    = T_GUI_Util::loadnames(optsGrpGraphics, nOptsGraphics, false);
        optsConfig      = T_GUI_Util::loadnames(optsGrpConfig, nOptsConfig, false);
      }
      else
      {
        optsGraphics = (char **) malloc(2 * sizeof(char *));
        optsGraphics[0] = strdup("default");
        optsGraphics[1] = NULL;
        nOptsGraphics = 1;

        optsConfig = (char **) malloc(2 * sizeof(char *));
        optsConfig[0] = strdup("default");
        optsConfig[1] = NULL;
        nOptsConfig = 1;

        // no options
        optsGrpGraphics = NULL;
        optsGrpConfig = NULL;
      }
      gbox->newList(optsGraphics);
      cbox->newList(optsConfig);
      
      delete xml;
    }
    catch (XMLException e)
    {
      #ifdef DEBUG_GUI
      std::string msg = "Error opening airplane specification file: ";
      msg += plane_paths[entry];
      msg += ": ";
      msg += e.what();
      #endif
    }
  }
}


/**
 *  Update the contents of the GL preview when an "interesting" change to the
 *  ui takes palce.  This is typically called from a pui callback.
 */
void CGUIPlaneSelectDialog::updatePreview()
{
  SimpleXMLTransfer *xml = NULL;
  int graphics = 0;
  int entry;
  std::string modelFile;
  std::string fname;
  
  entry = planes->getIntegerValue();
  if ((entry >= 0) && (entry < planes->getNumItems()))
  {
    fname = plane_paths[entry];
  }

  if (FileSysTools::fileExists(fname))
  {
    graphics = gbox->getCurrentItem();

    try 
    {
      xml = new SimpleXMLTransfer(fname);
      for (int i = 0; i < xml->getChildCount(); i++) 
      {
        SimpleXMLTransfer *child = xml->getChildAt(i);
        if (!child->getName().compare("graphics") && (graphics-- == 0))
        {
          modelFile = child->getString("model");
          break;
        }
      }
      delete xml;
    }
    catch (XMLException e)
    {
      if (xml) 
      {
        delete xml;
      }
      #ifdef DEBUG_GUI
      std::string msg = "Error opening airplane specification file: ";
      msg += fname;
      msg += ": ";
      msg += e.what();
      #endif
    }

    std::string objectFile = FileSysTools::getDataPath("objects/" + modelFile);
    std::string texturePath  = objectFile.substr(0, objectFile.length() - modelFile.length() - 1 - 7) + "textures";
#ifdef DEBUG_GUI
    std::cout << "updatePreview(): graphics " << graphics << ", entry " << entry << ", file " << fname << std::endl;
    std::cout << "   model File  : " << modelFile << std::endl;
    std::cout << "   object File : " << objectFile << std::endl;
    std::cout << "   texture Path: " << texturePath << std::endl;
#endif
    preview->loadGeometry(objectFile.c_str(), texturePath.c_str());
  }
}


/**
 *  Get the selected file name and options, and put them back
 *  into the configuration file.
 *
 *  \return true if the selected file exists
 */
bool CGUIPlaneSelectDialog::saveSelection() const
{
  int graphics = 0;
  int config   = 0;
  std::string fname;
  bool bRet  =  false;
  int entry;
  
  entry = planes->getIntegerValue();
  if ((entry >= 0) && (entry < planes->getNumItems()))
  {
    fname = plane_paths[entry];
  }

  if (FileSysTools::fileExists(fname))
  {
    graphics = gbox->getCurrentItem();
    config = cbox->getCurrentItem();

    #ifdef DEBUG_GUI
    std::cout << "saveConfigOptions(): graphics " << graphics << ", config " << config << std::endl;
    #endif

    SimpleXMLTransfer* ap = cfgfile->getChild("airplane");
    
    ap->setAttributeOverwrite("graphics", graphics);
    ap->setAttributeOverwrite("config", config);
    ap->setAttributeOverwrite("file", fname);
    bRet = true;
  }

  return bRet;
}


/**
 *  This callback is invoked when a category is selected from
 *  the combo box.
 */
void CGUIPlaneSelCategoryCallback(puObject *obj)
{
  CGUIPlaneSelectDialog *dlg = (CGUIPlaneSelectDialog*)obj->getUserData();
  dlg->updatePlaneList();
}


/**
 *  This callback is invoked when a new plane is selected from
 *  the file list
 */
void CGUIPlaneSelPlaneListCallback(puObject *obj)
{
  CGUIPlaneSelectDialog *dlg = (CGUIPlaneSelectDialog*)obj->getUserData();
  dlg->updateFileInfo();
  dlg->updatePreview();
}


/**
 *  This callback is invoked when a new plane is selected from
 *  the file list
 */
void CGUIPlaneSelGraphicsCallback(puObject *obj)
{
  CGUIPlaneSelectDialog *dlg = (CGUIPlaneSelectDialog*)obj->getUserData();
  dlg->updatePreview();
}


/** \brief The dialog's callback.
 *
 *  Determine if a plane was selected and load the new model.
 */
void CGUIPlaneSelCallback(puObject *obj)
{
  CGUIPlaneSelectDialog *dlg = (CGUIPlaneSelectDialog*)obj;
    
  if (obj->getIntegerValue() == CRRC_DIALOG_OK)
  {
    // Dialog left by clicking OK
    if (dlg->saveSelection())
    {
      // User selected an existing airplane, load the new model
            
      //~ std::cout << "selected: " << fname << std::endl;
      Global::aircraft->setModel(NULL);

      cfgfile->setAttributeOverwrite("airplane.use_default_launch",
                                      (dlg->getLoadLaunchDefault() == 0) ? "0" : "1");
      
      try
      {
        loadAirplane();
        
        // check if the user wants to load the default launch settings
        // for this airplane
        if (dlg->getLoadLaunchDefault() == 1)
        {
          // first check if there's a default at all...
          SimpleXMLTransfer *presets;
          presets = Global::aircraft->getFDMInterface()->getLaunchPresets();
          if (presets != NULL)
          {
            // o.k., take the values from the first preset
            SimpleXMLTransfer *def_launch = presets->getChildAt(0);
            cfgfile->setAttributeOverwrite("launch.altitude", 
                                           def_launch->getString("altitude", "0.0"));

            cfgfile->setAttributeOverwrite("launch.velocity_rel", 
                                           def_launch->getString("velocity_rel", "0.0"));
            
            cfgfile->setAttributeOverwrite("launch.angle", 
                                           def_launch->getString("angle", "0.0"));
            
            cfgfile->setAttributeOverwrite("launch.sal", 
                                           def_launch->getString("sal", "0"));
            cfgfile->setAttributeOverwrite("launch.rel_to_player", 
                                           def_launch->getString("rel_to_player", "1"));
            cfgfile->setAttributeOverwrite("launch.rel_front",     
                                           def_launch->getString("rel_front", doubleToString(MODELSTART_REL_FRONT)));
            cfgfile->setAttributeOverwrite("launch.rel_right", 
                                           def_launch->getString("rel_right", doubleToString(MODELSTART_REL_RIGHT)));
          }
          
        }
                
        initialize_flight_model();
        if (Global::soundserver != (CRRCAudioServer*)0)
          Global::soundserver->pause(false);
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
  
  Global::gui->hide();
  puDeleteObject(obj);
}
