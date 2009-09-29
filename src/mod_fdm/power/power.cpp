/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2008, 2009 - Jens Wilhelm Wulf (original author)
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
#include "power.h"

#include "values_step.h"

Power::Power::Power()
{
  dPropFreq = 0;
}

Power::Power::Power(SimpleXMLTransfer* xml, int nVerbosity)
{
  SimpleXMLTransfer* power = xml->getChild("power");
  SimpleXMLTransfer* bat;

  for (int n=0; n<power->getChildCount(); n++)
  {
    bat = power->getChildAt(n);
    if (bat->getName().compare("battery") == 0)
    {
      Battery *b;
      b = new Battery(bat);
      batteries.push_back(b);
    }
    else if (bat->getName().compare("automagic") == 0)
    {
      Battery *b;
      
      if (nVerbosity > 4)
      {
        std::cout << "  --- automatic power configuration: input -------------------------\n";
        bat->print(std::cout, 2);
      }

      b = new Battery();
      b->automagic(bat);
      batteries.push_back(b);

      {
        // remove automagic entries, rename
        SimpleXMLTransfer* pa;
        SimpleXMLTransfer* gr;
        int                idx;

        pa  = bat->getChild("battery");
        idx = pa->indexOfChild("automagic");
        gr  = pa->getChildAt(idx);
        pa->removeChildAt(idx);
        delete gr;

        pa  = bat->getChild("battery.shaft.engine");
        idx = pa->indexOfChild("automagic");
        gr  = pa->getChildAt(idx);
        pa->removeChildAt(idx);
        delete gr;

        pa  = bat->getChild("battery.shaft.propeller");
        idx = pa->indexOfChild("automagic");
        gr  = pa->getChildAt(idx);
        pa->removeChildAt(idx);
        delete gr;
      }

      if (nVerbosity > 3)
      {
        std::cout << "  --- automatic power configuration: output ------------------------\n";
        bat->getChild("battery")->print(std::cout, 2);
        std::cout << "  --- automatic power configuration: end ---------------------------\n";
      }
    }
  }

  dPropFreq = 0;
}


Power::Power::~Power()
{
  // deallocate all batteries
  for (unsigned int i = 0; i < batteries.size(); i++)
  {
    delete batteries[i];
    batteries[i] = NULL;
  }
}


void Power::Power::step(double      dt,
                        TSimInputs* inputs,
                        double      V_X,
                        CRRCMath::Vector3*    force,
                        CRRCMath::Vector3*    moment)
{
  const int       mul = 2;
  unsigned int    size = batteries.size();
  PowerValuesStep values;
  CRRCMath::Vector3         f = CRRCMath::Vector3();
  CRRCMath::Vector3         m = CRRCMath::Vector3();

  values.force          = &f;
  values.moment         = &m;
  values.inputs         = inputs;
  values.V_X            = V_X;
  values.dPropFreq      = 0; // in case of an empty system
  values.dBatCapLeftMin = 1;

  // Looks like this needs to be simulated at a higher frequency.
  values.dt     = dt/mul;

  for (unsigned int m=0; m<mul-1; m++)
  {
    for (unsigned int n=0; n<size; n++)
    {
      batteries[n]->step(&values);
    }
  }

  values.force  = force;
  values.moment = moment;
  for (unsigned int n=0; n<size; n++)
  {
    batteries[n]->step(&values);
  }

  dPropFreq      = values.dPropFreq;
  dBatCapLeftMin = values.dBatCapLeftMin;
}

void Power::Power::reset(CRRCMath::Vector3 vInitialVelocity)
{
  unsigned int    size = batteries.size();
  
  for (unsigned int n=0; n<size; n++)
  {
    batteries[n]->reset(vInitialVelocity);
  }
  dPropFreq = 0;
}
