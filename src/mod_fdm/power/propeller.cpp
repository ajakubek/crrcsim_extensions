/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2006, 2008, 2009 - Jens Wilhelm Wulf (original author)
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
#include "propeller.h"

#include <iostream>
#include "../../mod_misc/filesystools.h"
#include "../../mod_misc/lib_conversions.h"


#define RHO       1.225 // kg/m^3
#define ETA_PROP  0.65

Power::Propeller::Propeller() : Gearing()
{
  filter.init(0, 0);
  omega_fold = 5;
  fFolded    = true;
  J          = 0;
  thrustdir  = CRRCMath::Vector3(1, 0, 0);
}

Power::Propeller::Propeller(SimpleXMLTransfer* xml) : Gearing(xml)
{
  SimpleXMLTransfer* prop;
  bool               fExtern = true;
  double             dSturz;
  
  if (xml->indexOfAttribute("filename") >= 0)
  {
    prop = new SimpleXMLTransfer(FileSysTools::getDataPath("models/propeller/" + xml->getString("filename") + ".xml", true));
  }
  else
  {
    prop    = xml;
    fExtern = false;
  }

  // Der Sturz wird in jedem Fall aus der Modelldatei gelesen, ansonsten muss man ja eine Propeller
  // Datei für jeden Sturz extra haben.
  dSturz  = M_PI * xml->attributeAsDouble("downthrust", 0) / 180;
  thrustdir  = CRRCMath::Vector3(cos(dSturz), 0, sin(dSturz));
  
    
  D          = prop->getDouble("D");
  H          = prop->getDouble("H");
  J          = prop->getDouble("J");
  omega_fold = prop->attributeAsDouble("n_fold", -1)*2*M_PI;
  fFolded    = false;
  
  std::cout << "      Propeller: D=" << D << " m, H=" << H << " m, J=" << J << " kg m^2";
  thrustdir.print(", thrustdir=", "\n");
   
  if (fExtern)
    delete prop;
  
  
  filter.init(0, 0);
  //  filter.init(0, 10E-3);
}

void Power::Propeller::step(PowerValuesStep* values)
{
  double  omega = i*values->omega;
  double  n     = omega/(2*M_PI);
    
  if (omega < omega_fold && omega_fold > 0)
  {
    fFolded           = true;
    values->dPropFreq = 0;
  }
  else
  {
    fFolded           = false;
    values->dPropFreq = n;
    
    // force
    double V_p = values->inputs->pitch * H * n;
    double V_X = values->V_X;
    filter.step(values->dt, V_p - V_X);
    double F_X = M_PI * 0.25 * D*D * RHO * fabs(V_X + filter.val/2) * filter.val;
  
    double P = F_X * (V_X + filter.val/2);
    double M = 0;

    *values->force += thrustdir * (F_X * ETA_PROP);
  
    if (fabs(omega) > 1E-5)
      M = P/omega *i;
    
    if (M < 0)
    {
      // Wirkungsgrad als Generator ist schlecht:
      M *= 0.5*eta;
    }
    else
    {
      M /= eta;
    }
    
    values->moment_shaft -= M;
    *values->moment += thrustdir * M;

#define _BLA
#ifdef BLA
    static int blacnt = 0;
    if (blacnt++ > 200)
    {
      blacnt = 0;
      
      std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);
      std::cout.precision(4);
      std::cout.width(7);
      std::cout << (F_X*ETA_PROP) << ", ";
      std::cout << omega << ", ";
      std::cout << P << ", ";
      std::cout << "\n";
    }
#endif    
    /*
    std::cout << "propeller: ";
    
    
    std::cout.width(7);
    std::cout << (values->omega*30/M_PI) << "/min, ";
    
    std::cout.width(7);
    std::cout << M << ", ";
    
     */
  }
  

  /*
  std::cout << "propeller: ";
  
   */

  /*
  std::cout.width(7);
  std::cout << V_p << " , ";
  
  std::cout.width(7);
  std::cout << V_X << ", ";
  */
  
  /*
  std::cout.width(7);
  std::cout << (P/eta) << ", ";
   */
  
  /*
  std::cout.width(7);
  std::cout << (F_X*eta_prop) << ", ";
  
  std::cout.width(7);
  std::cout << (values->omega*30/M_PI) << "/min, ";
  */ 
  
  
/*  std::cout.width(7);
  std::cout << values->U << " V, ";
  std::cout.width(7);
  std::cout << M_M << " N, ";*/
  
//  std::cout << "\n";
}

void Power::Propeller::automagic(SimpleXMLTransfer* xml)
{
  SimpleXMLTransfer* p = xml->getChild("battery.shaft.propeller");
  
  D = p->getDouble("D");
  H = p->getDouble("H");
  J = p->getDouble("J");
  
  double F = xml->getDouble("F");
  double V = xml->getDouble("V");
  
  double dSturz  = M_PI * p->attributeAsDouble("downthrust", 0) / 180;
  thrustdir  = CRRCMath::Vector3(cos(dSturz), 0, sin(dSturz));
      
  // F = M_PI * 0.25 * D*D * RHO * (V_X + filter.val/2) * filter.val * ETA_PROP;
  // F = M_PI * 0.25 * D*D * RHO * (V + (Hn-V)/2) * (Hn-V) * ETA_PROP;
  // F = M_PI * 0.25 * D*D * RHO * (V/2 + Hn/2) * (Hn-V) * ETA_PROP;
  double n = sqrt( (8*F/(M_PI*D*D*RHO*ETA_PROP)) + V*V)/H;
  p->setAttribute("automagic.n_P", doubleToString(n));
  
  double M = F * (V + (V + H*n)/2) / (2*M_PI*n) * i / eta;
  p->setAttribute("automagic.M_P", doubleToString(M));
  
  omega_fold = p->attributeAsDouble("n_fold", -1)*2*M_PI;
  fFolded = false;
}

void Power::Propeller::reset(CRRCMath::Vector3 vInitialVelocity, double& dOmega)
{
  filter.init(0, 0);  
  fFolded = true;
  if (omega_fold < 0)
    dOmega = 2 * M_PI * vInitialVelocity.r[0] / H;
}
