/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2008 - Jens Wilhelm Wulf (original author)
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
#ifndef RATELIM_H
# define RATELIM_H

namespace CRRCMath
{
  /**
   * Limits the rate of change of a value.
   *
   * @author Jens Wilhelm Wulf
   */
  class RateLimiter
  {
    public:

     /**
      * @param ival      initial value
      * @param rate      the value is allowed to change rate per second
      */
     void init(double ival, double rate);

     /**
      * @param dt   timestep [s]
      * @param nval new input value
      */
     void step(double dt, double nval);

     /**
      * limited value
      */
     double val;

    private:
     double ratemax;
  };
};
#endif
