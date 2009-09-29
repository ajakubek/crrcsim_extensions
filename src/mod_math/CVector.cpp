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
// Simple class for 3D-vectors
// Author: Jan Reucker

#include "CVector.h"
#include <math.h>

/** \brief Constructor of the CVector class.
 *
 *  Creates a new CVector object and initializes its components
 *  to the given parameters.
 *  \param px x-component
 *  \param py y-component
 *  \param pz z-component
 */
CVector::CVector(float px, float py, float pz): x(px), y(py), z(pz)
{
}

/** \brief Destructor of the CVector class.
 *
 *  Destroys the object.
 */
CVector::~CVector()
{
}

/** \brief Sets the CVector's components to the given values.
 *
 *  \param px x-component
 *  \param py y-component
 *  \param pz z-component
 */
void CVector::set(float px, float py, float pz)
{
  x = px;
  y = py;
  z = pz;
}


/** \brief The length of the vector.
 *
 *  \return Length of the vector.
 */
float CVector::length()
{
  return sqrt(x*x + y*y + z*z);
}

/** \brief Normalizes the vector.
 *
 *  This method normalizes the vector. This means that the direction
 *  the vector points to is unchanged, but the length of the vector
 *  will be 1.0 after the operation.
 *  
 *  After calling this method all components will have changed. To
 *  generate a normal vector from the current vector without 
 *  actually changing it, one might copy the vector and then divide
 *  it by its length.
 */
void CVector::normalize()
{
  float len = this->length();
  
  if (len > 0.0)
  {
    x = x/len;
    y = y/len;
    z = z/len;
  }
}


/** \brief Adds two vectors
 *
 *  Geometric addition of two vectors.
 */
CVector CVector::operator+ (CVector param) 
{
  CVector temp;
  temp.x = x + param.x;
  temp.y = y + param.y;
  temp.z = z + param.z;
  return (temp);
}

/** \brief Subtracts one vector of another
 *
 *  Geometric subtraction of two vectors.
 */
CVector CVector::operator- (CVector param) 
{
  CVector temp;
  temp.x = x - param.x;
  temp.y = y - param.y;
  temp.z = z - param.z;
  return (temp);
}

/** \brief Calculate the cross product / vector product / outer product.
 *
 *  This operator calculates the scalar product of two vectors.
 *  Note that the commutative law does not apply to this operation,
 *  so vect_a * vect_b != vect_b * vect_a! (In fact, vect_a * vect_b
 *  equals -(vect_b * vect_a).
 *
 */
CVector CVector::operator* (CVector param) 
{
  CVector temp;
  temp.x = y * param.z - z * param.y;
  temp.y = z * param.x - x * param.z;
  temp.z = x * param.y - y * param.x;
  return (temp);
}

/** \brief Calculate the scalar product / inner product.
 *
 *  This operator calculates the scalar product of two vectors.
 *
 *  The operator % may look a bit weird when using it to calculate a
 *  product, but it was free to be overloaded since modulo division
 *  does not make sense when applied to a vector.
 *
 *  \return A float value representing the area of the parallelogram
 *          defined by the two vectors.
 */
float CVector::operator% (CVector param) 
{
  return (x * param.x + y * param.y + z * param.z);
}

/** \brief Divide the vector by a scalar.
 *
 *  \return The vector (x/param; y/param; z/param).
 */
CVector CVector::operator/ (float param)
{
  CVector temp;
  temp.x = x / param;
  temp.y = y / param;
  temp.z = z / param;
  return temp;
}

CVector CVector::operator* (float param)
{
  CVector temp;
  temp.x = x * param;
  temp.y = y * param;
  temp.z = z * param;
  return temp;
}

/** \brief isZero() checks if the vector is the zero vector.
 *
 *  The zero vector is defined as (0.0; 0.0; 0.0). This method
 *  checks if the current vector is equal to the zero vector.
 *  \return true if vector is equal to (0.0; 0.0; 0.0), false otherwise
 */
bool CVector::isZero()
{
  return ((x == 0.0) && (y == 0.0) && (z == 0.0));
}

/**
 * Calculates the distance to another given point
 */
float CVector::distance(float px, float py, float pz)
{
  return(sqrt( (px-x)*(px-x) + (py-y)*(py-y) + (pz-z)*(pz-z) ));
}

float CVector::angle_cos_sqr(CVector b)
{
  float cos_phi_sqr = (*this%b)*(*this%b) / ( (*this%*this) * (b%b) );
  return(cos_phi_sqr);
}

CVector CVector::mul (CVector b)
{
  return(CVector(x*b.x, y*b.y, z*b.z));
}
