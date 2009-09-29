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
#include "engine_dcm.h"

#include <iostream>
#include "../../mod_misc/filesystools.h"
#include "../../mod_misc/lib_conversions.h"
#include "../../mod_math/linearreg.h"

// Wirkungsgrad des Drehzahlstellers
#define  ETA_STELLER  0.95

#define  THR_P_S 3.0

inline double sqr(double val) { return(val*val); };


Power::Engine_DCM::Engine_DCM() : Gearing()
{
  k_r = 0;
  J   = 0;
  throttle.init(0, THR_P_S);
}

Power::Engine_DCM::Engine_DCM(SimpleXMLTransfer* xml) : Gearing(xml)
{
  SimpleXMLTransfer* eng;
  bool               fExtern = true;
  
  if (xml->indexOfAttribute("filename") >= 0)  
    eng = new SimpleXMLTransfer(FileSysTools::getDataPath("models/engine/" + xml->getString("filename") + ".xml", true));
  else
  {
    eng = xml;
    fExtern = false;
  }

  if (eng->attributeAsInt("calc", 0) != 0)
  {
    SimpleXMLTransfer* sp;
    SimpleXMLTransfer* i;
    unsigned int       size;
    double             U_K;
    double             I_M;
    double             omega;
    T_LinearReg        lr;
    
    lr.init();
    sp   = eng->getChild("data");
    size = sp->getChildCount();
    for (unsigned int uCnt=0; uCnt<size; uCnt++)
    {
      i     = sp->getChildAt(uCnt);
      U_K   = i->getDouble("U_K");
      I_M   = i->getDouble("I_M");
      omega = i->getDouble("n") * 2 * M_PI;
      
      lr.add(omega/I_M, U_K/I_M);
    }
    lr.calc();
    R_I = lr.get_a();
    k_M = lr.get_b();
        
    sp   = eng->getChild("data_idle");
    size = sp->getChildCount();
    if (size == 1)
    {
      M_r = sp->getDouble("data.I_M") * k_M;
      k_r = 0;
    }
    else
    {
      lr.init();
      for (unsigned int uCnt=0; uCnt<size; uCnt++)
      {
        i     = sp->getChildAt(uCnt);
        U_K   = i->getDouble("U_K");
        I_M   = i->getDouble("I_M");
        omega = (U_K - R_I * I_M)/k_M;
        lr.add(omega, I_M);
      }
      lr.calc();
      // I_0(omega) = a     + b     * omega
      // M_r(omega) = a*k_M + b*k_M * omega
      M_r = lr.get_a() * k_M;
      k_r = lr.get_b() * k_M;
    }
  }
  else
  {  
    R_I = eng->getDouble("R_I");
    k_M = eng->getDouble("k_M");
    // Im Leerlauf verursacht das Reibmoment einen Leerlaufstrom.
    M_r = eng->getDouble("I_0") * k_M;
    k_r = 0;
  }
  J            = eng->getDouble("J_M");

  std::cout << "      Engine_DCM: R_I=" << R_I << " Ohm, M_r = "
    << M_r << " Nm, k_r = " << k_r << ", k_M=" << k_M << " Nm/A, J_M=" << J << " kg m^2\n";

  throttle.init(0, THR_P_S);

  if (fExtern)
    delete eng;
}

void Power::Engine_DCM::step(PowerValuesStep* values)
{
  double M_M;
  double omega = i*values->omega;

  // battery empty?
  if (values->U < 0.01)
  {
    throttle.init(0, THR_P_S);
  }
  else
  {
    // limit throttle change
    throttle.step(values->dt, values->inputs->throttle);
  }

  // voltage applied to motor
  double U_K = throttle.val * values->U * ETA_STELLER;

  // Generatorspannung
  double U_Gen = omega * k_M;

  //  motor current
  double I_M = (U_K - U_Gen) / R_I;

  if (I_M < 0.001)
  {
    // Das Ding wird nicht generatorisch...
    // Und für eine EMK-Bremse waren Faktoren in der Größenordnung 20*I_M nötig,
    // daher wurde die Bremse auf die Welle verlegt.
    I_M = 0;
  }

  // Äußeres Moment
  M_M = k_M * I_M - M_r - k_r * omega;

  if (M_M > 0)
    M_M *= eta;
  else
    M_M /= eta;

  values->moment_shaft += M_M*i;
  
  // Drain battery. But it can't be charged.
  if (I_M > 0)
    values->I += I_M;

#define _BLA
#ifdef BLA
  static int blacnt = 0;
  if (blacnt++ > 400)
  {
    blacnt = 0;

    std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);
    std::cout.precision(4);
    std::cout << "engine: ";

    std::cout.width(7);
    std::cout << I_M << " A, ";

    std::cout.width(7);
    std::cout << throttle.val << ", ";

    std::cout.width(7);
    std::cout << values->U << " V, ";

    std::cout.width(7);
    std::cout << ((M_M*omega/eta)/(U_K*I_M)) << "=eta, ";

    std::cout.width(7);
    std::cout << i << "=i, ";

    std::cout.width(7);
    std::cout << (omega*30/M_PI) << "/min, ";
    std::cout.width(7);
    std::cout << M_M << " N, ";

    std::cout << "\n";
  }
#endif
}

void Power::Engine_DCM::automagic(SimpleXMLTransfer* xml)
{
  SimpleXMLTransfer* e = xml->getChild("battery.shaft.engine");
  
  // A gearing is needed anyway:
  eta = 0.95;
  e->setAttributeOverwrite("gearing.eta", doubleToString(eta));
  e->setAttributeOverwrite("gearing.J",   "0");

  // Gewünschte Drehzahl und gewünschtes Moment
  double M_P    = xml->getDouble("battery.shaft.propeller.automagic.M_P");
  double n_P    = xml->getDouble("battery.shaft.propeller.automagic.n_P");

  // Mit dem Wirkungsgrad des Getriebes ergibt sich die aufzubringende 
  // mechanische Leistung
  double P_mech = M_P * 2 * M_PI * n_P / eta;  

  // Use at least 10V, but 24V at 1kW.
  double U   = 10; // V
  if (P_mech > 60)
    U = U + P_mech * (24-U)/(1000-60);
  e->setAttribute("automagic.U", doubleToString(U/ETA_STELLER));

  // Position der Polstelle
  double omega_p = e->getDouble("automagic.omega_p");
  k_M = U/omega_p;
  e->setAttribute("k_M", doubleToString(k_M));

  // maximaler Wirkungsgrad bei dieser Spannung
  double eta_opt = e->getDouble("automagic.eta_opt");
  double R_I_mul_I_0 = U * (eta_opt - 2*sqrt(eta_opt) + 1);

  // Wirkungsgrad im geforderten Betriebspunkt
  double eta_nenn = e->getDouble("automagic.eta");
  // Winkelgeschwindigkeit, bei welcher dieser Wirkungsgrad erreicht
  // wird (und zwar vom Maximum aus in Richtung kleinerer Drehzahl).    
  double omega_nenn = (-sqrt(sqr(eta_nenn-1)*U*U-2*(eta_nenn+1)*R_I_mul_I_0*U + sqr(R_I_mul_I_0))
                       +(eta_nenn+1)*U - R_I_mul_I_0)
                      /(2*k_M);

  // Jetzt muss bei dieser Drehzahl noch die gewünschte mech. Leistung
  // erreicht werden:
  double I_M = P_mech / (U * eta_nenn);
  e->setAttribute("automagic.I", doubleToString(I_M));

  R_I = (U - k_M * omega_nenn) / I_M;
  M_r = (R_I_mul_I_0 / R_I) * k_M;
  k_r = 0;
  e->setAttribute("R_I", doubleToString(R_I));
  e->setAttribute("automagic.M_r", doubleToString(M_r));

  // calc gearing
  i   = omega_nenn/2.0/M_PI/n_P;
  e->setAttribute("gearing.i", doubleToString(i));

  // Inertia
  //  P = M * omega
  //  M = P/omega
  J = P_mech/omega_nenn * 0.5/omega_nenn;
  e->setAttribute("J_M", doubleToString(J));
}

void Power::Engine_DCM::reset(CRRCMath::Vector3 vInitialVelocity, double& dOmega)
{
  throttle.init(0, THR_P_S);
}

