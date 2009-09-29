/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2005, 2008 - Jens Wilhelm Wulf (original author)
 *   Copyright (C) 2008 - Jan Reucker
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
#ifndef VECTOR3_H
# define VECTOR3_H

# include <math.h>
# include <string>

namespace CRRCMath
{
  /**
   * A vector with one column and three rows of double values.
   *
   * @author Jens Wilhelm Wulf
   */
  class Vector3
  {
    public:
     /**
      * Rows 0, 1 and 2.
      */
     double r[3];

    public:

     Vector3()
     {
       r[0] = r[1] = r[2] = 0;
     }

     Vector3(const Vector3& v)
     {
       r[0] = v.r[0];
       r[1] = v.r[1];
       r[2] = v.r[2];
     }

     Vector3(double i0, double i1, double i2)
     {
       r[0] = i0;
       r[1] = i1;
       r[2] = i2;
     };

     /**
      * Operator: Zuweisung
      * \todo operator= not first checking for assignment to this
      */
     Vector3& operator=(const Vector3& b)
     {
       r[0] = b.r[0];
       r[1] = b.r[1];
       r[2] = b.r[2];
       return *this;
     }

     /**
      * Operator: Scalarprodukt
      * Wichtig: n*Vector3 geht nicht, es muss Vector3*n benutzt werden!
      */
     Vector3 operator*(const double scalar) const
     {
       return Vector3(scalar*r[0], scalar*r[1], scalar*r[2]);
     }

     /**
      * Operator: Kreuzprodukt
      */
     Vector3 operator*(const Vector3& V) const
     {
       return Vector3( r[1] * V.r[2] - r[2] * V.r[1],
                       r[2] * V.r[0] - r[0] * V.r[2],
                       r[0] * V.r[1] - r[1] * V.r[0] );
     }

     /**
      * Operator: Summe
      */
     Vector3 operator+(const Vector3& V) const
     {
       return Vector3( r[0] + V.r[0],
                       r[1] + V.r[1],
                       r[2] + V.r[2] );
     }

     /**
      * Operator: Differenz
      */
     Vector3 operator-(const Vector3& V) const
     {
       return Vector3( r[0] - V.r[0],
                       r[1] - V.r[1],
                       r[2] - V.r[2] );
     }

     /**
      * Operator: Addition
      */
     Vector3 operator+=(const Vector3& V)
     {
       r[0] += V.r[0];
       r[1] += V.r[1];
       r[2] += V.r[2];

       return *this;
     }

     /**
      * Operator: Multiplication
      */
     Vector3 operator*=(const double scalar)
     {
       r[0] *= scalar;
       r[1] *= scalar;
       r[2] *= scalar;

       return *this;
     }

     /**
      * returns length of vector
      */
     double length();

     void print(std::string pre, std::string post);
  };
}
#endif
