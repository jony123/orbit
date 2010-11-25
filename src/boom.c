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

InitBooms()
/*
 *  Initiliaze explosions
 */
{
	int i;
	double v[3];

	/* Mark all booms unused */
	for (i=0; i<NBOOMS; i++)
	{
		boom[i].age = 0.0;
		boom[i].angle = 0.0;
	}

	/* Tweak explosion data */
	for (i=0; i<12; i++)
	{
		/* Tweak explosion shape */
		Vmul (v, icos_data[i], 0.9+rnd (0.2));
		Vmul (v, v, 0.2);
		Vset (boom_data[i], v);

		/* Tweak explosion color */
		boom_color[i][0] = 0.9+rnd (0.1);
		boom_color[i][1] = rnd(1.5);
	}
}

int FindBoom()
/*
 *  Find unused or oldest boom
 */
{
	int i, oldest;
	double maxage;

	for (i=0; i<NBOOMS; i++)
	{
		if (boom[i].age == 0.0) return (i);

		if (i == 0)
		{
			maxage = boom[i].age;
			oldest = i;
		}
		else
		{
			if (boom[i].age > maxage)
			{
				maxage = boom[i].age;
				oldest = i;
			}
		}
	}

	/* No unused booms, return oldest */
	return (oldest);
}

DestroyBoom (b)
int b;
/*
 *  Mark boom as unused
 */
{
	boom[b].age = 0.0;

	/* Destroy corresponding light */
	DestroyLight (boom[b].light);
}

Boom (v, size)
double v[3], size;
/*
 *  There was an explosion at coordinates v[]
 */
{
	int b, l;

	/* Find an unused boom */
	b = FindBoom();

	/* Set palette_flash, so next screen clear flashes the screen */
	palette_flash = 1;

	/* Set it up */
	boom[b].age = deltaT;
	Vset (boom[b].pos, v);
	boom[b].size = size;

	l = FindLight();
	boom[b].light = l;
	light[l].pos[0] = (float) boom[b].pos[0];
	light[l].pos[1] = (float) boom[b].pos[1];
	light[l].pos[2] = (float) boom[b].pos[2];
	light[l].age = deltaT;
	light[l].color[0] = light[l].color[1] = light[l].color[2] = 1.0;

	boom[b].angle = rnd (360.0);

	/* Play sound */
	if (sound) PlayAudio (SOUND_BOOM);
}

DrawBooms()
/*
 *  Draw the explosions
 */
{
	int b;

	glPointSize (1.0);
	for (b=0; b<NBOOMS; b++)
	{
		if (boom[b].age > 0.0) DrawBoom (b);
	}
}

DrawBoom (b)
int b;
/*
 *  Draw this explosion
 */
{
	int i;
	double v1[3], v2[3], v3[3], v[3];
	double t, u;

	/* Bump boom time */
	boom[b].age += deltaT;

	/* See if boom has expired */
	if (boom[b].age > BOOM_TIME)
	{
		DestroyBoom (b);
		return;
	}

	/* Convert age to [0,1] */
	t = boom[b].age / BOOM_TIME;

	/* And on [1,0] */
	u = 1.0 - t;

	/* Set the intensity of this boom's light source */
	i = boom[b].light;
	light[i].color[0] = light[i].color[1] = light[i].color[2] = u;

	/* Draw the boom */
	glPushMatrix();
	glDisable (GL_LIGHTING);
	glDepthMask (GL_FALSE);

	/* Translate to where the boom is */
	Vsub (v, boom[b].pos, player.pos);
	glTranslated (v[0], v[1], v[2]);

	/* Apply rotation so they all don't look the same */
	glRotated (boom[b].angle, 1.0, 0.0, 1.0);

	/* Scale according to size of boom */
	glScaled (boom[b].size, boom[b].size, boom[b].size);

	/* Draw a smaller yellow boom first */
	glColor3d (u, u, 0.0);
	if (t < 0.5)
		/* Growing sphere */
		glutSolidSphere (t/8.0, 5, 5);
	else
		/* Shrinking sphere */
		glutSolidSphere (u/8.0, 5, 5);

	/* Now draw the fancier, alpha-blended gas cloud */
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Make a bunch of pretty sparkles */
	glBegin (GL_POINTS);
	for (i=0; i<20; i++)
	{
		Vmul (v1, boom_data[i], t);
		glColor4d (boom_color[i][0], boom_color[i][1], boom_color[i][2], u);
		glVertex3d (v1[0], v1[1], v1[2]);
	}
	glEnd();

	for (i=19; i>=0; i--)
	{
		/* Adjust size according to explosion age 
		   (should do this with glScale)*/
		Vmul (v1, boom_data[icos_index[i][2]], t/2.0);
		Vmul (v2, boom_data[icos_index[i][1]], t/2.0);
		Vmul (v3, boom_data[icos_index[i][0]], t/2.0);

		glBegin (GL_POLYGON);

		glColor4d (boom_color[icos_index[i][2]][0],
		           boom_color[icos_index[i][2]][1],
		           boom_color[icos_index[i][2]][2],
		           u);
		glVertex3d (v1[0], v1[1], v1[2]);

		glColor4d (boom_color[icos_index[i][1]][0],
		           boom_color[icos_index[i][1]][1],
		           boom_color[icos_index[i][1]][2],
		           u);
		glVertex3d (v2[0], v2[1], v2[2]);

		glColor4d (boom_color[icos_index[i][0]][0],
		           boom_color[icos_index[i][0]][1],
		           boom_color[icos_index[i][0]][2],
		           u);
		glVertex3d (v3[0], v3[1], v3[2]);

		glEnd();
	}

	glDisable (GL_BLEND);
	glDepthMask (GL_TRUE);
	glEnable (GL_LIGHTING);
	glPopMatrix();
}

