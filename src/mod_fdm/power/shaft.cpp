/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2006, 2008, 2009 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2006 - Jan Reucker
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
#include "shaft.h"

#include <iostream>
#include "engine_dcm.h"
#include "propeller.h"
#include "simplethrust.h"

Power::Shaft::Shaft(SimpleXMLTransfer* xml)
{
  SimpleXMLTransfer* it;
  double             J_ges;
  Gearing*           s;

  fBrake = (xml->attributeAsInt("brake", 1) != 0);
  omega.init(0, 0);
  J_ges = xml->attributeAsDouble("J", 0);
  std::cout << "  Shaft: J=" << J_ges << " kg m^2\n";
      
  for (int n=0; n<xml->getChildCount(); n++)
  {
    it = xml->getChildAt(n);
    s = (Gearing*)0;
    if (it->getName().compare("engine") == 0)
    {
      s = new Engine_DCM(it);
    }
    else if (it->getName().compare("propeller") == 0)
    {
      s = new Propeller(it);
    }
    else if (it->getName().compare("simplethrust") == 0)
    {
      s = new SimpleThrust(it);
    }
    if (s != (Gearing*)0)
    {
      gear.push_back(s);
      J_ges += s->getJ();
    }
  }
  
  J_inv = 1/J_ges;
}


Power::Shaft::~Shaft()
{
  // deallocate gears
  for (unsigned int i = 0; i < gear.size(); i++)
  {
    delete gear[i];
    gear[i] = NULL;
  }
}


void Power::Shaft::step(PowerValuesStep* values)
{
  values->omega        = omega.val;
  values->moment_shaft = 0;
  
  int size = gear.size();    
  for (int n=0; n<size; n++)
  {
    gear[n]->step(values);
  }
  
//  std::cout << omega.val << "\n";

  if ( (values->inputs->throttle < 0.05 || values->U < 0.01) && fBrake)
  {
    omega.init(0, 0);
  }
  else
  {
    omega.step(values->dt, values->moment_shaft*J_inv);
  }
}

void Power::Shaft::automagic(SimpleXMLTransfer* xml)
{
  Propeller*     p;
  Engine_DCM*    e;

  fBrake = (xml->getChild("battery.shaft")->attributeAsInt("brake", 1) != 0);
  
  p = new Propeller();
  p->automagic(xml);
  gear.push_back(p);

  e = new Engine_DCM();
  e->automagic(xml);
  gear.push_back(e);  
  
  J_inv = 1/(e->getJ() + p->getJ());
  omega.init(0, 0);
}

void Power::Shaft::reset(CRRCMath::Vector3 vInitialVelocity)
{
  // There may be more than one propeller connected to me, but
  // I just set my initial omega according to what one of the props 
  // tells me. However, if there is more than one prop, most surely all of them
  // will have the same gearbox ratio and pitch so this is not a problem
  // at all.
  double dOmega = 0;
  for (unsigned int n=0; n<gear.size(); n++)
    gear[n]->reset(vInitialVelocity, dOmega);
  
  omega.init(dOmega, 0);  
}
