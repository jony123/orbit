/*

ORBIT, a freeware space combat simulator
Copyright (C) 1999  Steve Belczyk <steve1@genesis.nred.ma.us>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "orbit.h"
#include "stars.h"

/*
 *  Stuff for the background starfield
 */

ReadStars()
/*
 *  Read in the stellar database
 */
{
	int i;
	double magmin, magmax;

	for (i=0; i<NSTARS; i++)
	{
		star[i].x = stars[i][0];
		star[i].y = stars[i][1];
		star[i].z = stars[i][2];
		star[i].mag = stars[i][3];

		/* Reverse "magnitude" of magnitude (smaller mag means brighter star!) */
		star[i].mag = 0.0 - star[i].mag;

		if (i == 0)
		{
			magmin = magmax = star[i].mag;
		}
		else
		{
			if (star[i].mag < magmin) magmin = star[i].mag;
			if (star[i].mag > magmax) magmax = star[i].mag;
		}
	}

	/* Set the magnitudes */
	for (i=0; i<NSTARS; i++)
	{
		star[i].bright = (star[i].mag - magmin) / (magmax - magmin);
	}

	/* Find magnitude limits for sparse list */
	for (i=0; i<NSTARS/10; i++)
	{
		if (i == 0)
		{
			magmin = magmax = star[i].mag;
		}
		else
		{
			if (star[i].mag < magmin) magmin = star[i].mag;
			if (star[i].mag > magmax) magmax = star[i].mag;
		}
	}

	for (i=0; i<NSTARS/10; i++)
	{
		star[i].bright2 = (star[i].mag - magmin) / (magmax - magmin);
	}
}

DrawStars()
/*
 *  "My god.  It's full of stars."
 */
{
	/* Turn off 3D stuff */
	glDisable (GL_DEPTH_TEST);
/*	glDisable (GL_CULL_FACE);	*/
	glDisable (GL_LIGHTING);
	glDepthMask (GL_FALSE);
/*	glDisable (GL_LIGHT0);	*/

	glPointSize (1);

	glCallList (star_list);

	/* Re-enable three-D stuff */
	glEnable (GL_DEPTH_TEST);
/*	glEnable (GL_CULL_FACE);	*/
	glEnable (GL_LIGHTING);
	glDepthMask (GL_TRUE);
/*	glEnable (GL_LIGHT0);	*/
}

MakeStarList()
/*
 *  Construct display list for starfield
 */
{
	int i, j;

	/* Make dense star field list */
	star_list_dense = glGenLists (1);
	glNewList (star_list_dense, GL_COMPILE);
	glBegin (GL_POINTS);

	for (j=0; j<NSTARS; j++)
	{
		/* Plot stars backwards so bright ones plotted last */
		i = (NSTARS - j) - 1;

		glColor3d (star[i].bright, star[i].bright, star[i].bright);
/*		glVertex4d (star[i].x, star[i].y, star[i].z, 0.00000); */
		glVertex4d (star[i].x, star[i].y, star[i].z, 0.001);
	}

	glEnd ();
	glEndList();

	/* Make sparse list */
	star_list_sparse = glGenLists (1);
	glNewList (star_list_sparse, GL_COMPILE);
	glBegin (GL_POINTS);

	for (j=0; j<(NSTARS/10); j++)
	{
		i = (NSTARS/10 - j) - 1;
		glColor3d (star[i].bright2, star[i].bright2, star[i].bright2);
		glVertex4d (star[i].x, star[i].y, star[i].z, 0.001);
	}

	glEnd ();
	glEndList();

	star_list = star_list_sparse;
	if (starfield == 2) star_list = star_list_dense;
}

