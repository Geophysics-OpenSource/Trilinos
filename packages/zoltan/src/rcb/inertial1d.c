/*====================================================================
 * ------------------------
 * | CVS File Information |
 * ------------------------
 *
 * $RCSfile$
 *
 * $Author$
 *
 * $Date$
 *
 * $Revision$
 *
 *====================================================================*/

/* This software was developed by Bruce Hendrickson and Robert Leland   *
 * at Sandia National Laboratories under US Department of Energy        *
 * contract DE-AC04-76DP00789 and is copyrighted by Sandia Corporation. */

#include <stdio.h>
#include "lb_const.h"
#include "irb_const.h"

int LB_inertial1d(
     struct irb_dot *dotpt,     /* graph data structure for weights */
     int              dotnum,   /* number of vtxs in graph */
     int              wgtflag,  /* are vertex weights being used? */
     double           cm[3],    /* center of mass in each direction */
     double           evec[3],  /* eigenvector */
     double           *value    /* array for value to sort on */
)
{
     int       i;               /* loop counter */

     /* Copy values into double precision array. */
     for (i = 0; i < dotnum; i++)
        value[i] = dotpt[i].X[0];

     /* zero unused center of mass and eigenvector */
     cm[0] = cm[1] = cm[2] = 0.0;
     evec[0] = evec[1] = evec[2] = 0.0;

     return(LB_OK);
}
