/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2008 Jan Reucker (original author)
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
  

/** \file crrc_ssgutils.h
 *
 *  Utility functions for working with PLIB SSG models.
 */

#ifndef CRRC_SSGUTILS_H_
#define CRRC_SSGUTILS_H_

#include <plib/ssg.h>
#include <string>

namespace SSGUtil
{


/**
 *  This struct stores the attributes that may be added to a
 *  node of a 3D object by appending them to the node's name.
 *  E.g. a node with the name "rotor -shadow" is interpreted by
 *  CRRCsim as a node with the name "rotor" and the attribute
 *  castsShadow set to false.
 */
struct NodeAttributes
{
  NodeAttributes()
  : castsShadow(true)
  {
  }
  
  bool  castsShadow;
};
  
/** Locate a named SSG node in a branch. */
ssgEntity* findNamedNode (ssgEntity* node, const char* name);

/** Splice a branch in between all child nodes and their parents. */
void spliceBranch (ssgBranch * branch, ssgEntity * child);

/** Get the "pure" name of a node (without any additional attributes). */
std::string getPureNodeName(ssgEntity* node);

/** Get any additional attributes that may be appended to the node's name */
SSGUtil::NodeAttributes getNodeAttributes(ssgEntity* node);

/** Remove a leaf node from a scenegraph */
void removeLeafFromGraph(ssgLeaf *leaf);

} // end namespace

#endif  // CRRC_SSGUTILS_H_

