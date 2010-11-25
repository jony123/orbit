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

InitRings()
/*
 *  Initialize rings
 */
{
	int r;

	/* Jupiter */
	ring[0].primary = 8;
	ring[0].r1 = 102000.0 / KM_TO_UNITS1;
	ring[0].r2 = 129130.0 / KM_TO_UNITS1;
	strcpy (ring[0].fn, "maps/jupring.ppm");

	/* Saturn */
	ring[1].primary = 13;
	ring[1].r1 = 74400.0 / KM_TO_UNITS1;
	ring[1].r2 = 140154.0 / KM_TO_UNITS1;
	strcpy (ring[1].fn, "maps/satring.ppm");

	/* Uranus */
	ring[2].primary = 21;
	ring[2].r1 = 38949.0 / KM_TO_UNITS1;
	ring[2].r2 = 50271.0 / KM_TO_UNITS1;
	strcpy (ring[2].fn, "maps/uraring.ppm");

	/* Neptune */
	ring[3].primary = 27;
	ring[3].r1 = 42000.0 / KM_TO_UNITS1;
	ring[3].r2 = 63000.0 / KM_TO_UNITS1;
	strcpy (ring[3].fn, "maps/nepring.ppm");

	/* Read the textures, make display lists */
	for (r=0; r<NRINGS; r++)
	{
		ReadRingTexture (r);
		MakeRingList (r);
	}
}

ReadRingTexture (r)
int r;
/*
 *  Read texture for planetary rings
 */
{
	int x, y, c, i;
	FILE *fd;

	/* Get id for this texture */
	glGenTextures (1, &ring[r].texid);
	glBindTexture (GL_TEXTURE_2D, ring[r].texid);
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

	/* Open the file */
	if (NULL == (fd = fopen (ring[r].fn, "rb")))
	{
		Log ("Can't open %s, giving up!", ring[r].fn);
		FinishSound();
		CloseLog();
		exit (0);
	}

	/* Read past PPM header */
	while (10 != fgetc(fd));
	while (10 != fgetc(fd));
	while (10 != fgetc(fd));

	/* Read in the color data */
	for (y=0; y<8; y++)
	{
		for (x=0; x<255; x++)
		{
			for (i=0; i<3; i++)
			{
				c = fgetc (fd);
				ring[r].tex[x][y][i] = 0xff & c;
			}

			/* Don't forget alpha! */
			ring[r].tex[x][y][3] = ring[r].tex[x][y][2];
		}
		
		for (i=0; i<4; i++)
		{
			ring[r].tex[255][y][i] = 0;
		}
	}

	fclose (fd);

	/* Set the texture */
	glTexImage2D (GL_TEXTURE_2D, 0, 4, 8, 256, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, ring[r].tex);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static float MaterialColor[] = {1.0, 1.0, 1.0, 1.0};

MakeRingList (r)
int r;
/*
 *  Make the display list for planetary rings
 */
{
	double x, y, r1, r2, th, a;
	int i, parity;

	/* Which side of the texture are we on */
	parity = 0;

	/* Inner, outer radii */
	r1 = ring[r].r1;
	r2 = ring[r].r2;

	/* Angle of each sector */
	a = 6.28 / ((double) ring_sectors);

	/* Get a display list */
	ring[r].list = glGenLists (1);
	glNewList (ring[r].list, GL_COMPILE);

	/* Don't write to depth buffer */
	glDepthMask (GL_FALSE);

	/* Set up texture */
	glBindTexture (GL_TEXTURE_2D, ring[r].texid);

	/* Do alpha blending here */
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Rings are a quad strip */
	glBegin (GL_QUAD_STRIP);

	for (i=0, th=0.0; i<=ring_sectors; i++, th+=a)
	{
		if (i == ring_sectors) th = 0.0;

		x = r2 * sin (th);
		y = r2 * cos (th);
		glNormal3f (0.0, 0.0, 1.0);
		if (parity)
			glTexCoord2f (0.0, 1.0);
		else
			glTexCoord2f (1.0, 1.0);
		glVertex3d (x, y, 0.0);

		x = r1 * sin (th);
		y = r1 * cos (th);
		glNormal3f (0.0, 0.0, 1.0);
		if (parity)
			glTexCoord2f (0.0, 0.0);
		else
			glTexCoord2f (1.0, 0.0);
		glVertex3d (x, y, 0.0);
		
		parity = !parity;
	}
	glEnd ();	/* Quad strip */

	/* Now do one upside down */

/***********
	glPushMatrix();
	glRotated (180.0, 0.0, 1.0, 0.0);
	glBegin (GL_QUAD_STRIP);

	parity = 0;

	for (i=0, th=0.0; i<=ring_sectors; i++, th+=a)
	{
		if (i == ring_sectors) th = 0.0;

		x = r2 * sin (th);
		y = r2 * cos (th);
		glNormal3f (0.0, 0.0, 1.0);
		if (parity)
			glTexCoord2f (0.0, 1.0);
		else
			glTexCoord2f (1.0, 1.0);
		glVertex3d (x, y, 0.0);

		x = r1 * sin (th);
		y = r1 * cos (th);
		glNormal3f (0.0, 0.0, 1.0);
		if (parity)
			glTexCoord2f (0.0, 0.0);
		else
			glTexCoord2f (1.0, 0.0);
		glVertex3d (x, y, 0.0);
		
		parity = !parity;
	}
	glEnd ();
	glPopMatrix();
***************/

	glDisable (GL_BLEND);
	glDepthMask (GL_TRUE);
	glEndList();
}

float TransColor[4] = {1.0, 1.0, 1.0, 0.5};
	
DrawRings()
/*
 *  Render those pretty rings
 */
{
	int r, p;
	double v[3];

	/* Don't bother if Sparky doesn't want them */
	if (!rings) return;

	/* Turn off lighting so rings are constant brightness */
	glDisable (GL_LIGHTING);

	/* Set color to white, will be modulated with texture */
	if (textures)
	{
		glColor4fv (MaterialColor);
/*		glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialColor);	*/
	}
	else
	{
/*		glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, TransColor);	*/
		glColor4fv (TransColor);
	}

	for (r=0; r<NRINGS; r++)
	{
		p = ring[r].primary;

		/* Forget it if primary is hidden */
		if (planet[p].hidden) continue;

		/* Don't bother if too far away */
		if (planet[p].range2 > MAX_RING_RANGE) continue;

		glPushMatrix();

		/* Translate to planet */
		Vsub (v, planet[p].pos, player.pos);
		glTranslated (v[0], v[1], v[2]);

		/* Rotate for oblicity */
		glRotated (planet[p].oblicity, 1.0, 0.0, 0.0);

		/* Turn off backface removal so we only have to draw rings
		   on one side */
		glDisable (GL_CULL_FACE);

		/* Draw, pahdner! */
		if (textures)
			glEnable (GL_TEXTURE_2D);
		else
			glDisable (GL_TEXTURE_2D);
		glCallList (ring[r].list);

		glDisable (GL_TEXTURE_2D);
		if (textures) glEnable (GL_LIGHTING);
		glEnable (GL_CULL_FACE);

		glPopMatrix();
	}
}

