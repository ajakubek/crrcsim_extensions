/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *   Copyright (C) ? - Bruce Jackson (original author)
 *   Copyright (C) 2007, 2008 - Jan Reucker
 *   Copyright (C) 2008 - Jens Wilhelm Wulf
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

#include "../mod_misc/ls_constants.h"
#include "ls_geodesy.h"
#include <math.h>

/* ONE_SECOND is pi/180/60/60, or about 100 feet at earths' equator */
#define ONE_SECOND  4.848136811E-6
#define HALF_PI     (0.5*M_PI)

/* Value of earth radius from [1], ft */
#define EQUATORIAL_RADIUS 20925650.
#define RESQ 437882827922500.

/* Value of earth flattening parameter from ref [1]
	
	Note: FP = f
	      E  = 1-f
	      EPS = sqrt(1-(1-f)^2)			*/
	      
//#define FP	.003352813178         unused
#define E   .996647186
#define EPS .081819221
//#define INVG .031080997             unused

/**
 * Written as part of LaRCSim project by E. B. Jackson
 * 
 * [ 1]  Stevens, Brian L.; and Lewis, Frank L.: "Aircraft
 *       Control and Simulation", Wiley and Sons, 1992.
 *       ISBN 0-471-61397-5
 */
void ls_geoc_to_geod(double lat_geoc, double radius, double * lat_geod,
                     double * alt, double * sea_level_r)
{
  double t_lat, x_alpha, mu_alpha, delt_mu, r_alpha, l_point, rho_alpha;
  double sin_mu_a, denom, delt_lambda, lambda_sl, sin_lambda_sl;

  if (((HALF_PI - lat_geoc) < ONE_SECOND) /* near North pole */
      || ((HALF_PI + lat_geoc) < ONE_SECOND)) /* near South pole */
  {
    *lat_geod = lat_geoc;
    *sea_level_r = EQUATORIAL_RADIUS * E;
    *alt = radius - *sea_level_r;
  }
  else
  {
    t_lat = tan(lat_geoc);
    x_alpha = E * EQUATORIAL_RADIUS / sqrt(t_lat * t_lat + E * E);
    mu_alpha = atan2(sqrt(RESQ - x_alpha * x_alpha), E * x_alpha);
    if (lat_geoc < 0)
      mu_alpha = -mu_alpha;
    sin_mu_a = sin(mu_alpha);
    delt_lambda = mu_alpha - lat_geoc;
    r_alpha = x_alpha / cos(lat_geoc);
    l_point = radius - r_alpha;
    *alt = l_point * cos(delt_lambda);
    denom = sqrt(1 - EPS * EPS * sin_mu_a * sin_mu_a);
    rho_alpha = EQUATORIAL_RADIUS * (1 - EPS) / (denom * denom * denom);
    delt_mu = atan2(l_point * sin(delt_lambda), rho_alpha + *alt);
    *lat_geod = mu_alpha - delt_mu;
    lambda_sl = atan(E * E * tan(*lat_geod)); /* SL geoc. latitude */
    sin_lambda_sl = sin(lambda_sl);
    *sea_level_r = sqrt(RESQ
                        / (1 +
                           ((1 / (E * E)) -
                            1) * sin_lambda_sl * sin_lambda_sl));
  }
}


void ls_geod_to_geoc(double lat_geod, double alt,
                     double * sl_radius, double * lat_geoc)
{
  double lambda_sl, sin_lambda_sl, cos_lambda_sl, sin_mu, cos_mu, px, py;

  lambda_sl = atan(E * E * tan(lat_geod));  /* sea level geocentric latitude */
  sin_lambda_sl = sin(lambda_sl);
  cos_lambda_sl = cos(lambda_sl);
  sin_mu = sin(lat_geod);       /* Geodetic (map makers') latitude */
  cos_mu = cos(lat_geod);
  *sl_radius = sqrt(RESQ
                    / (1 +
                       ((1 / (E * E)) - 1) * sin_lambda_sl * sin_lambda_sl));
  py = *sl_radius * sin_lambda_sl + alt * sin_mu;
  px = *sl_radius * cos_lambda_sl + alt * cos_mu;
  *lat_geoc = atan2(py, px);
}
