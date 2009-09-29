/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2008 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2005, 2008 - Jan Reucker
 *   Copyright (C) 2007 - Martin Herrmann
 *   Copyright (C) 2006 - Todd Templeton
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
#include "inputdev.h"

#include "../mod_misc/lib_conversions.h"

#include <math.h>

// 1/exp(exp(1)), needed for mix_exp
#ifndef M_1_E_E
# define M_1_E_E 0.065988
#endif

// exp(1), needed for mix_exp
#ifndef M_E
# define M_E 2.7182818284590452354
#endif

/**
 * Values given here have to correspond to enum values in mouse_kbd.h.
 * todo: this cross-reference is not nice...
 */
const char* ButtonStrings[] =
{
  "NOTHING", "RESUME", "PAUSE", "RESET", "INCTHROTTLE",
    "DECTHROTTLE", "ZOOMIN", "ZOOMOUT"
};

/// Don't change the order of the following strings unless you know exactly what
/// you're doing!
/// \todo: has to become a member of TSimInputs? Or do enum and strings belong somewhere else?
const char* AxisStrings[]   = {"NOTHING", "AILERON", "ELEVATOR", "RUDDER", "THROTTLE",
                               "FLAP", "SPOILER", "RETRACT", "PITCH"};
/// same as AxisStrings[], but lower case for attribute names in XML file
const char* AxisStringsXML[] = {"nothing", "aileron", "elevator", "rudder", "throttle",
                                "flap", "spoiler", "retract", "pitch"};
/// same as AxisStrings[], but with a "nice" appearance for the GUI
const char* AxisStringsGUI[] = {"nothing", "Aileron", "Elevator", "Rudder", "Throttle",
                                "Flap", "Spoiler", "Retract", "Pitch"};

/**
 * Strings ordered like radio type enums in T_TX_Interface
 */
const char* RadioTypeStrings[] = {"Futaba", "Airtronics", "Hitec", "JR", "Cockpit", "Walkera", "Custom"};

/**
 * Strings ordered like input method enums in T_TX_Interface
 */
const char* InputMethodStrings[] = {"Keyboard", "Mouse",   "Joystick",
                                    "RCTran", "Audio",
                                    "Parallel", "Serial2", "RCTran2",
                                    "FMSPIC", "MNAV", "ZhenHua" };


int T_TX_Interface::init(SimpleXMLTransfer* config)
{
#if DEBUG_TX_INTERFACE > 0
  printf("int T_TX_Interface::init(SimpleXMLTransfer* config)\n");
#endif

  return(0);
}

std::string T_TX_Interface::getErrMsg() 
{
  return(errMsg);
};

void T_TX_Interface::getInputData(TSimInputs* inputs)
{
#if DEBUG_TX_INTERFACE > 1
  printf("void T_TX_Interface::getInputData(TSimInputs* inputs)\n");
#endif
}

T_TX_Interface::T_TX_Interface()
  : mixer(NULL), calib(NULL), map(NULL), errMsg("")
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_TX_Interface::T_TX_Interface\n");
#endif
}

T_TX_Interface::~T_TX_Interface()
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_TX_Interface::~T_TX_Interface\n");
#endif
}

void T_TX_Interface::putBackIntoCfg(SimpleXMLTransfer* config)
{
}

float T_TX_Interface::limit(float in)
{
  if (in < -0.5)
  {
    return -0.5;
  }
  else if (in > 0.5)
  {
    return 0.5;
  }
  else
  {
    return in;
  }
}

/** Limit unsigned axis values
 *
 *  Limits unsigned axis values and suppresses jitter just above zero.
 */
float T_TX_Interface::limit_unsigned(float in)
{
  float out;
  
  if (in > 1.0)
  {
    out = 1.0;
  }
  else if (in < 0.05)
  {
    out = 0.0;
  } 
  else
  {
    out = in;
  }

  return out;
}

void T_TX_Interface::CalibMixMapValues(TSimInputs* inputs, float* myArrayOfValues)
{
  float calibrated;
  int   axisnum;
  
  axisnum    = map->func[T_AxisMapper::ELEVATOR];
  if (axisnum >= 0)
  {
    calibrated = calib->calibrate(axisnum, myArrayOfValues[axisnum])
                  * map->inv[T_AxisMapper::ELEVATOR];
    inputs->elevator = mixer->mix_signed(calibrated, T_AxisMapper::ELEVATOR);
  }
  
  axisnum    = map->func[T_AxisMapper::AILERON];
  if (axisnum >= 0)
  {
    calibrated = calib->calibrate(axisnum, myArrayOfValues[axisnum])
                  * map->inv[T_AxisMapper::AILERON];
    inputs->aileron = mixer->mix_signed(calibrated, T_AxisMapper::AILERON);
  }

  axisnum    = map->func[T_AxisMapper::RUDDER];
  if (axisnum >= 0)
  {
    calibrated = calib->calibrate(axisnum, myArrayOfValues[axisnum])
                  * map->inv[T_AxisMapper::RUDDER];
    inputs->rudder = mixer->mix_signed(calibrated, T_AxisMapper::RUDDER);
  }

  axisnum    = map->func[T_AxisMapper::THROTTLE];
  if (axisnum >= 0)
  {
    calibrated = calib->calibrate(axisnum, myArrayOfValues[axisnum])
                  * map->inv[T_AxisMapper::THROTTLE];
    inputs->throttle = 
      limit_unsigned(mixer->mix_unsigned(calibrated + 0.5, T_AxisMapper::THROTTLE));
  }

  axisnum    = map->func[T_AxisMapper::FLAP];
  if (axisnum >= 0)
  {
    calibrated = calib->calibrate(axisnum, myArrayOfValues[axisnum])
                  * map->inv[T_AxisMapper::FLAP];
    inputs->flap = mixer->mix_unsigned(calibrated + 0.5, T_AxisMapper::FLAP);
  }

  axisnum    = map->func[T_AxisMapper::SPOILER];
  if (axisnum >= 0)
  {
    calibrated = calib->calibrate(axisnum, myArrayOfValues[axisnum])
                  * map->inv[T_AxisMapper::SPOILER];
    inputs->spoiler = 
      limit_unsigned(mixer->mix_unsigned(calibrated + 0.5, T_AxisMapper::SPOILER));
  }

  axisnum    = map->func[T_AxisMapper::RETRACT];
  if (axisnum >= 0)
  {
    calibrated = calib->calibrate(axisnum, myArrayOfValues[axisnum])
                  * map->inv[T_AxisMapper::RETRACT];
    inputs->retract = 
      limit_unsigned(mixer->mix_unsigned(calibrated + 0.5, T_AxisMapper::RETRACT));
  }

  axisnum    = map->func[T_AxisMapper::PITCH];
  if (axisnum >= 0)
  {
    calibrated = calib->calibrate(axisnum, myArrayOfValues[axisnum])
                  * map->inv[T_AxisMapper::PITCH];
    inputs->pitch = mixer->mix_signed(calibrated, T_AxisMapper::PITCH);
  }


  
}

// --- Implementation of class T_TX_Mixer -------------------------------------

/** \brief Create a mixer object
 *
 *  The mixer will be initialized with default values: scaling of all
 *  axes is set to 1.0, all other parameters are set to 0.0.
 */
T_TX_Mixer::T_TX_Mixer(T_TX_Interface *parent) 
{
  baseInit(parent);
#if DEBUG_TX_INTERFACE > 0
  printf("T_TX_Mixer::T_TX_Mixer()\n");
#endif
}


/** \brief Create and initialize a mixer object
 *
 *  The mixer will be initialized from the given child
 *  in the SimpleXMLTransfer. This does the same as if you
 *  would call <code>T_TX_Mixer::init(cfg, child)</code>
 *  after calling the empty constructor.
 *
 *  \param cfg Pointer to an XML tree containing the config info.
 *  \param child The name of the child which holds the mixer info.
 */
T_TX_Mixer::T_TX_Mixer(T_TX_Interface *parent, 
                       SimpleXMLTransfer* cfg, std::string child)
{
  baseInit(parent);
#if DEBUG_TX_INTERFACE > 0
  printf("T_TX_Mixer::T_TX_Mixer(cfg, child)\n");
#endif
  init(cfg, child);
}


/** \brief Destroy the mixer.
 *
 *  Right now the dtor is only provided for debugging purposes.
 */
T_TX_Mixer::~T_TX_Mixer()
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_TX_Mixer::~T_TX_Mixer()\n");
#endif
}


/** \brief Initialize all variables
 */
void T_TX_Mixer::baseInit(T_TX_Interface *iface_val)
{
  int i;

  for (i = 0; i < T_AxisMapper::NUM_AXISFUNCS; i++)
  {
    trim_val[i]  = 0.0;
    nrate_val[i] = 1.0;
    exp_val[i]   = 0.0;
  }

  enabled = 1;
  iface = iface_val;
}


/** \brief Initialize the mixer from a config file.
 *
 *  The mixer object will be initialized from the given config file.
 *  This file may contain more than one branch with interface settings,
 *  so the name of the child has to be specified.
 */
int T_TX_Mixer::init(SimpleXMLTransfer* cfg, std::string child)
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_TX_Mixer::init(cfg, child)\n");
  printf(" <-- %s\n", child.c_str());
#endif

  SimpleXMLTransfer* mixer;
  SimpleXMLTransfer* item;
  SimpleXMLTransfer* inter;

  // store the child's name for writing back the settings
  child_in_cfg = child;

  try
  {
    inter = cfg->getChild(child, true);
    mixer = inter->getChild("mixer", true);

    enabled = mixer->attributeAsInt("enabled", 1);
 
    for (int i = T_AxisMapper::AILERON; i <= T_AxisMapper::PITCH; i++)
    { 
      item = mixer->getChild(AxisStringsXML[i], true);
      trim_val[i]   = item->getDouble("trim",   0.0);
      exp_val[i]    = item->getDouble("exp",    0.0);
      nrate_val[i]  = item->getDouble("nrate",  1.0);
    }
  }
  catch (XMLException e)
  {
    errMsg = e.what();
    return 1;
  }

  return 0;
}


/** \brief Transfers all settings back to the config file.
 *
 *  The mixer settings will be stored in the same branch of the config
 *  file that they were read from on initialization.
 *
 *  \param config Pointer to the config file's SimpleXMLTransfer.
 */
void T_TX_Mixer::putBackIntoCfg(SimpleXMLTransfer* config)
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_TX_Mixer::putBackIntoCfg(SimpleXMLTransfer* config)\n");
  printf(" --> %s\n", child_in_cfg.c_str());
#endif

  SimpleXMLTransfer* inter = config->getChild(child_in_cfg);
  SimpleXMLTransfer* mixer = inter->getChild("mixer");
  SimpleXMLTransfer* item;

  mixer->setAttributeOverwrite("enabled", enabled);

  for (int i = T_AxisMapper::AILERON; i <= T_AxisMapper::PITCH; i++)
  { 
    item = mixer->getChild(AxisStringsXML[i], true);
    item->setAttributeOverwrite("trim",   doubleToString(trim_val[i]));
    item->setAttributeOverwrite("exp",    doubleToString(exp_val[i]));
    item->setAttributeOverwrite("nrate",  doubleToString(nrate_val[i]));
  }                                              
}


/** \brief Apply exponential component to input.
 *
 *  \param x Linear input signal (-1.0 ... 1.0).
 *  \param p Percentage of exponential component to be applied to input (0.0 ... 1.0).
 */
float T_TX_Mixer::mix_exp(const float x, const float p) const
{
  return x * (1.0 + p * (exp(fabs(x) * M_E) * M_1_E_E - 1.0));
}


/** \brief Mix a centered axis
 *
 *  Mixes an axis which delivers positive and negative values
 * (e.g. aileron, elevator, ...)
 *
 * \param in        input value
 * \param function  One of the enum values defined in T_AxisMapper
 * \return          Mixed value
 */
float T_TX_Mixer::mix_signed(float in, int function) const
{
  if (enabled)
  {
    float tmp, exp_in;
    exp_in = 2.0 * in + trim_val[function];
    tmp = 0.5 * nrate_val[function] * mix_exp(exp_in, exp_val[function]);
    return tmp;
  }
  else
  {
    return in;
  }
}


/** \brief Mix a positive-only axis
 *
 *  Mixes an axis which delivers only positive values
 * (e.g. throttle, spoiler, retract)
 *
 * \param in        input value
 * \param function  One of the enum values defined in T_AxisMapper
 * \return          Mixed value
 */
float T_TX_Mixer::mix_unsigned(float in, int function) const
{
  if (enabled)
  {
    float tmp, exp_in;
    exp_in =  in + trim_val[function];
    tmp = nrate_val[function] * mix_exp(exp_in, exp_val[function]);
    return tmp;
  }
  else
  {
    return in;
  }
}


/** \brief Get current error message
 *
 *  Returns the current error message (if any).
 */
std::string T_TX_Mixer::getErrMsg() 
{
  return(errMsg);
};


// --- Implementation of class T_AxisMapper -------------------------

T_AxisMapper::T_AxisMapper(T_TX_Interface *parent)
  : child_in_cfg(""), radio_type(0), iface(parent)
{
  for (int i = 0; i < T_AxisMapper::NUM_AXISFUNCS; i++)
  {
    func[i]   = i;
    inv[i]    = 1;
    c_func[i] = i;
    c_inv[i]  = 1;
  }
}

T_AxisMapper::T_AxisMapper(T_TX_Interface *parent,
                           SimpleXMLTransfer* cfgfile,
                           std::string child)
  : child_in_cfg(""), radio_type(0), iface(parent)
{
  for (int i = 0; i < T_AxisMapper::NUM_AXISFUNCS; i++)
  {
    func[i]   = i;
    inv[i]    = 1;
    c_func[i] = i;
    c_inv[i]  = 1;
  }
  init(cfgfile, child);
}

void T_AxisMapper::init(SimpleXMLTransfer* cfgfile, std::string childname)
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_AxisMapper::init(cfg, child)\n");
  printf(" <-- %s\n", childname.c_str());
#endif
  SimpleXMLTransfer* inter;
  SimpleXMLTransfer* bindings;
  SimpleXMLTransfer* group;
  SimpleXMLTransfer* item;
  
  child_in_cfg = childname;

  // try to load config
  try
  {
    inter = cfgfile->getChild(childname, true);
    bindings  = inter->getChild("bindings", true);
    group     = bindings->getChild("axes", true);
  
    for (int i = T_AxisMapper::AILERON; i <= T_AxisMapper::PITCH; i++)
    {
      int default_axis = -1;
      float default_polarity = 1.0;

      // special handling for some default values
      if (i == T_AxisMapper::AILERON)
      {
        default_axis = 0;
      }
      else if (i == T_AxisMapper::ELEVATOR)
      {
        default_axis = 1;
        if (iface->inputMethod() != T_TX_Interface::eIM_joystick)
        {
          default_polarity = -1.0;
        }
      }
      
      item = group->getChild(AxisStringsXML[i], true);
      c_func[i] = item->attributeAsInt("axis", default_axis);
      c_inv[i]  = item->attributeAsDouble("polarity", default_polarity);
    }

    std::string radio = 
      strU(bindings->attribute("radio_type", RadioTypeStrings[CUSTOM]));

    for (int n=0; n < NR_OF_RADIO_TYPES; n++)
    {
      if (radio.compare(strU(RadioTypeStrings[n])) == 0)
      {
        setRadioType(n);
      }
    }
  }
  catch (XMLException e)
  {
    fprintf(stderr, "*** T_AxisMapper: XMLException: %s\n", e.what());
  }
}


void T_AxisMapper::putBackIntoCfg(SimpleXMLTransfer* cfgfile)
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_AxisMapper::putBackIntoCfg(SimpleXMLTransfer* config)\n");
  printf(" --> %s\n", child_in_cfg.c_str());
#endif
  SimpleXMLTransfer* item;
  SimpleXMLTransfer* group;
  SimpleXMLTransfer* item2;

  try
  {
    item = cfgfile->getChild(child_in_cfg);
    group = item->getChild("bindings.axes");

    for (int i = T_AxisMapper::AILERON; i <= T_AxisMapper::PITCH; i++)
    {
      item2 = group->getChild(AxisStringsXML[i], true);
      item2->setAttributeOverwrite("axis", c_func[i]);
      item2->setAttributeOverwrite("polarity", doubleToString(c_inv[i]));
    }
  
    item2 = item->getChild("bindings");
    item2->setAttributeOverwrite("radio_type", RadioTypeStrings[radio_type]);
  }
  catch (XMLException e)
  {
    fprintf(stderr, "*** T_AxisMapper: XMLException: %s\n", e.what());
  }
}


void T_AxisMapper::setRadioType(int rtype)
{
  printf ("mapper set to radio type %d\n", rtype);
  int   saved[T_AxisMapper::NUM_AXISFUNCS];
  float saved_i[T_AxisMapper::NUM_AXISFUNCS];
  
  for (int i = 0; i < T_AxisMapper::NUM_AXISFUNCS; i++)
  {
    saved[i] = func[i];
    saved_i[i] = inv[i];
    func[i] = i;
    inv[i] = 1.0;
  }

  switch (rtype)
  {
    case AIRTRONICS:
      func[T_AxisMapper::ELEVATOR]  = 0;
      func[T_AxisMapper::AILERON]   = 1;
      func[T_AxisMapper::THROTTLE]  = 2;
      func[T_AxisMapper::RUDDER]    = 3;
      radio_type = rtype;
      break;
    
    case JR:
      func[T_AxisMapper::ELEVATOR]  = 2;
      func[T_AxisMapper::AILERON]   = 1;
      func[T_AxisMapper::THROTTLE]  = 0;
      func[T_AxisMapper::RUDDER]    = 3;
      radio_type = rtype;
      break;
    
    case COCKPIT:
      func[T_AxisMapper::ELEVATOR]  = 1;
      func[T_AxisMapper::AILERON]   = 0;
      func[T_AxisMapper::THROTTLE]  = 3;
      func[T_AxisMapper::RUDDER]    = 2;
      radio_type = rtype;
      break;
    
    case FUTABA:
    case HITEC:
      func[T_AxisMapper::ELEVATOR]  = 1;
      func[T_AxisMapper::AILERON]   = 0;
      func[T_AxisMapper::THROTTLE]  = 2;
      func[T_AxisMapper::RUDDER]    = 3;
      radio_type = rtype;
      break;

    case WALKERA:
      func[T_AxisMapper::AILERON] =2; inv[T_AxisMapper::AILERON] =-1;
      func[T_AxisMapper::ELEVATOR]=1; inv[T_AxisMapper::ELEVATOR]=-1;
      func[T_AxisMapper::RUDDER]  =3; inv[T_AxisMapper::RUDDER  ]= 1;
      func[T_AxisMapper::THROTTLE]=0; inv[T_AxisMapper::THROTTLE]= 1;
      radio_type = rtype;
      break;
    
    case CUSTOM:
      for (int i = 0; i < T_AxisMapper::NUM_AXISFUNCS; i++)
      {
        func[i] = c_func[i];
        inv[i]  = c_inv[i];
      }
      radio_type = rtype;
      break;
    
    default:
      // don't change anything
      for (int i = 0; i < T_AxisMapper::NUM_AXISFUNCS; i++)
      {
        func[i] = saved[i];
        inv[i]  = saved_i[i];
      }
      break;
  }
}

void T_AxisMapper::saveToCustom()
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_AxisMapper::saveToCustom()\n");
#endif
  int numAxis = iface->getNumAxes();
  for (int i = 0; i < T_AxisMapper::NUM_AXISFUNCS; i++)
  {
    // only store a value if it is in the valid range
    // for the current interface
    if (func[i] < numAxis)
    {
      c_func[i] = func[i];
      c_inv[i]  = inv[i];
    }
  }
}


// --- Implementation of class T_Calibration -------------------------
T_Calibration::T_Calibration(T_TX_Interface *parent)
  : child_in_cfg(""), iface(parent)
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_Calibration::T_Calibration()\n");
#endif
  for (int i = 0; i < TX_MAXAXIS; i++)
  {
    scale[i] = 1.0;
    offset[i] = 0.0;
  }
}

T_Calibration::T_Calibration(T_TX_Interface *parent,
                             SimpleXMLTransfer* cfg,
                             std::string child)
  : child_in_cfg(child), iface(parent)
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_Calibration::T_Calibration(cfg, child)\n");
#endif
  for (int i = 0; i < TX_MAXAXIS; i++)
  {
    scale[i] = 1.0;
    offset[i] = 0.0;
  }
  init(cfg, child);
}

void T_Calibration::init(SimpleXMLTransfer* cfgfile,
                         std::string childname)
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_Calibration::init(cfg, child)\n");
  printf(" <-- %s\n", childname.c_str());
#endif
  int size;
  SimpleXMLTransfer* item;
  SimpleXMLTransfer* group;
  SimpleXMLTransfer* item2;
  
  child_in_cfg = childname;

  // try to load config
  try
  {
    item = cfgfile->getChild(childname, true);
    group = item->getChild("calibration", true);
  
    size  = group->getChildCount();
    if (size > TX_MAXAXIS)
      size = TX_MAXAXIS;
    for (int n = 0; n < size; n++)
    {
      item2 = group->getChildAt(n);
      scale[n]  = item2->getDouble("scale", 1.0);
      offset[n] = item2->getDouble("offset", 0.0);
    }
  }
  catch (XMLException e)
  {
    fprintf(stderr, "*** T_Calibration::init(): %s\n", e.what());
  }
}


void T_Calibration::putBackIntoCfg(SimpleXMLTransfer* cfgfile)
{
#if DEBUG_TX_INTERFACE > 0
  printf("T_Calibration::putBackIntoCfg(cfg)\n");
  printf(" --> %s\n", child_in_cfg.c_str());
#endif
  int size;
  SimpleXMLTransfer* item;
  SimpleXMLTransfer* group;
  SimpleXMLTransfer* item2;

  item = cfgfile->getChild(child_in_cfg);
  group = item->getChild("calibration");

  // clean list
  size = group->getChildCount();
  for (int n = 0; n < size; n++)
  {
    item2 = group->getChildAt(0);
    group->removeChildAt(0);
    delete item2;
  }
  // create new list
  for (int n = 0; n < TX_MAXAXIS; n++)
  {
    item2 = new SimpleXMLTransfer();
    item2->setName("axis");
    item2->addAttribute("scale", doubleToString(scale[n]));
    item2->addAttribute("offset", doubleToString(offset[n]));
    group->addChild(item2);
  }
}


float T_Calibration::calibrate(int axis, float raw)
{
  return (raw * scale[axis] + offset[axis]);
}
