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

static float ambient[]         =   { 0.0, 0.0, 0.0, 1.0 };
static float diffuse[]         =   { 1.0, 1.0, 0.0, 1.0 };
static float position0[]       =   { 0.0, 0.0, 0.0, 1.0 };
static float lmodel_ambient[]  =   { 0.01, 0.01, 0.01, 1.0 };
static float lmodel_twoside[]  =   {GL_TRUE};
static float front_shininess[] =   {50.0};
static float front_specular[]  =   { 1.0, 1.0, 1.0, 1.0 }; 
/* static float front_specular[]  =   { 0.0, 0.7, 0.0, 1.0 }; */

Lights()
{
	int l;
	double v[3];
	float f[3];

	/* Enable depth buffer (why is this *here*?) */
	glDepthFunc (GL_LEQUAL);
	glEnable (GL_DEPTH_TEST);

	/* Turn off ambient light */
	glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightfv (GL_LIGHT0, GL_AMBIENT, lmodel_ambient);

	/* Define light0, the big, main light */
/*	glLightfv (GL_LIGHT0, GL_POSITION, position0);	*/
	Vsub (v, planet[0].pos, player.pos);
	f[0] = (float) v[0];
	f[1] = (float) v[1];
	f[2] = (float) v[2];
	glLightfv (GL_LIGHT0, GL_POSITION, f);

	/* Turn on lighting in general */
	glEnable (GL_LIGHTING);

	/* Turn on individual lights */
	glEnable (GL_LIGHT0);

	/* Check each non-permanent light */
	for (l=0; l<NLIGHTS; l++)
	{
		if (light[l].age > 0.0)
		{
			light[l].age += deltaT;
			glLightfv (light[l].gl_num, GL_DIFFUSE, light[l].color);
			glLightfv (light[l].gl_num, GL_SPECULAR, light[l].color);
/*			glLightfv (light[l].gl_num, GL_POSITION, light[l].pos);	*/
			f[0] = light[l].pos[0] - (float) player.pos[0];
			f[1] = light[l].pos[1] - (float) player.pos[1];
			f[2] = light[l].pos[2] - (float) player.pos[2];
			glLightfv (light[l].gl_num, GL_POSITION, f);
			glEnable (light[l].gl_num);
		}
		else
		{
/*			glDisable (light[l].gl_num);	*/
		}
	}

	/* Pick smooth or flat shading */
	glShadeModel (GL_SMOOTH);
/*	glShadeModel (GL_FLAT);		*/
}

InitLights()
/*
 *  Set up non-permanent light sources
 */
{
	int i;

	for (i=0; i<NLIGHTS; i++)
	{
		light[i].age = 0.0;
		light[i].pos[3] = 1.0;
		light[i].gl_num = GL_LIGHT1 + i;
	}
}

int FindLight()
/*
 *  Return free or oldest light
 */
{
	int i, oldest;
	double maxage;

	for (i=0; i<NLIGHTS; i++)
	{
		if (light[i].age == 0.0) return (i);

		if (i == 0)
		{
			oldest = i;
			maxage = light[i].age;
		}
		else
		{
			if (light[i].age > maxage)
			{
				maxage = light[i].age;
				oldest = i;
			}
		}
	}

	return (oldest);
}

DestroyLight (l)
int l;
/*
 *  Destroy specified light
 */
{
	light[l].age = 0.0;
	glDisable (light[l].gl_num);
}

