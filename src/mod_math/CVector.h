/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) 2004 - Jan Reucker (original author)
 *   Copyright (C) 2005, 2007, 2008 - Jens Wilhelm Wulf
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
/** \brief Simple class for 3D-vectors.
 *
 *  This class implements a "vector" data type in 3D-space.
 *  It provides overloaded operators for some basic
 *  vector operations in cartesian coordinates (adding, subtracting,
 *  inner and outer products...).
 *
 *  Author: Jan Reucker
 */

#ifndef CVECTOR_H
#define CVECTOR_H

class CVector
{
  public:
    CVector(float px = 0.0, float py = 0.0, float pz = 0.0);
    ~CVector();
    void set(float px, float py, float pz);
    float length();
    float distance(float px, float py, float pz);
    void normalize();
    bool isZero();

    // overloaded operators
    CVector operator+ (CVector);  // Vector addition
    CVector operator- (CVector);  // Vector subtraction
    CVector operator* (CVector);  // cross (outer) product
    float   operator% (CVector);  // scalar (inner) product
    CVector operator/ (float);    // scalar division
    CVector operator* (float);    // 
  
    CVector mul (CVector);   
  
    /**
     * phi is the angle between both of the vectors and
     * this method returns ( cos(phi) )^2
     */
    float angle_cos_sqr(CVector);

    float x;  /**< x component of the vector */
    float y;  /**< y component of the vector */
    float z;  /**< z component of the vector */
  
  private:

};

#endif /* CVECTOR_H */
