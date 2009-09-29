/*
 * CRRCsim - the Charles River Radio Control Club Flight Simulator Project
 *
 * Copyright (C) 2005, 2008 Jens Wilhelm Wulf (original author)
 * Copyright (C) 2005 Jan Reucker
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
  

#ifndef CRRC_SCALEINPUT_H
#define CRRC_SCALEINPUT_H

#include <plib/pu.h>

#include "crrc_dialog.h"
#include "crrc_slider.h"
#include "../mod_misc/SimpleXMLTransfer.h"
#include "../mod_inputdev/inputdev.h"

class CGUIInputScaleDialog;

/** \brief The software mixer options dialog.
 *
 */
class CGUIInputScaleDialog : public CRRCDialog
{
  public:
    enum { NrOfAxes = 4 };
   
    CGUIInputScaleDialog(T_TX_Interface* itxi);
    ~CGUIInputScaleDialog();
    
    T_TX_Interface* txi;
   
    crrcSlider*     slider_trim[NrOfAxes];
    crrcSlider*     slider_nrate[NrOfAxes];
    crrcSlider*     slider_exp[NrOfAxes];
   
    puText*         labels[3];
    
    puButton*       enable_button;

    puGroup*        all_widgets;
};

#endif // CRRC_SCALEINPUT_H
