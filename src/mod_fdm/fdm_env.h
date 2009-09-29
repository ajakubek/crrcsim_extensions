/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2008 - Jens Wilhelm Wulf (original author)
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
#ifndef ENVIROMENT_H
# define ENVIROMENT_H

class FDMBase;
class TSimInputs;

/**
 * This is the interface used by the (various) FDMs to get information from the outside:
 *   - scenery for collision detection
 *   - windfield data
 *   - enviromental data like air density and gravitational acceleration
 * An instance of an implementation of this is passed to the FDM upon creation. This seems
 * quite expensive, but helps to separate this module (FDM) quite well.
 * It makes it possible to combine any FDM with any windfield and any enviroment (earth, any 
 * other planet). 
 * In contrast to original CRRCSim it assumes earth has a flat surface and 
 *    air density
 *    gravitational acceleration
 * do only depend on altitude.
 * 
 * Additionally it allows a callback from the fdm to include controllers.
 * 
 * @author Jens Wilhelm Wulf
 */
class FDMEnviroment
{
public:
  
  virtual ~FDMEnviroment() {};
  
  /**
   *  Get the height at a distinct point.
   *  \param x x coordinate (positive north)
   *  \param y y coordinate (positive east)
   *  \return terrain height at this point in ft
   */
  virtual float GetSceneryHeight(float x_north, float y_east) = 0;
  
  /**
   * Calculate the wind velocities in all three axes in the given position.
   * Returns 1 if this position is outside of the grid.
   * X/Y/Z -- north/east/down
   */
  virtual int CalculateWind(double  X_cg,      double  Y_cg,     double  Z_cg,
                            double& Vel_north, double& Vel_east, double& Vel_down) = 0;

  /**
   * Returns gravitational acceleration at height 'altitude'
   */
  virtual double GetG(double altitude) = 0;
  
  /**
   * Returns air density at height 'altitude'
   */
  virtual double GetRho(double altitude) = 0;
  
  /**
   * This can be used to integrate one or many controllers into the simulation loop.
   * If you don't want to do this, simply copy the contents of pInputsFromUser to pInputsToFDM.
   * There might be FDMs which do not use this callback, because their author thought 
   * using them with a control loop is not of much use.
   */
  virtual void ControllerCallback(double dt, FDMBase* fdm, TSimInputs* pInputsFromUser, TSimInputs* pInputsToFDM) = 0;
};

#endif
