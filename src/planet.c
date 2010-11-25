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

static float MaterialColor[] = {1.0, 1.0, 1.0, 1.0};
static float specular[] = {0.0, 0.0, 0.0, 1.0};
static float emission[] = {0.0, 0.0, 0.0, 1.0};

DrawPlanets()
/*
 *  Draw all the planets
 */
{
	int p;

	/* Draw orbits */
	if (draw_orbits) DrawOrbits();

	for (p=0; p<NPLANETS; p++)
	{
		if (!planet[p].hidden) DrawPlanet (p);
	}
}

DrawPlanet (p)
int p;
/*
 *  Draw the pretty planet
 */
{
	double th, r2, v[3];
	int pr;

	/* Set color to white, will be modulated with texture */
	if (textures)
	{
		glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialColor);
		glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, MaterialColor);
	}
	else
	{
		glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, planet[p].color);
		glMaterialfv (GL_FRONT_AND_BACK, GL_AMBIENT, planet[p].color);
	}

	glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, specular);
	glMaterialfv (GL_FRONT_AND_BACK, GL_EMISSION, emission);
	glMaterialf  (GL_FRONT_AND_BACK, GL_SHININESS, 0.0);

	glPushMatrix();

	/* How far away from view is this planet? */
	Vsub (v, planet[p].pos, player.pos);
	planet[p].absrange2 = Mag2(v);
	planet[p].range2 = r2 = planet[p].absrange2 / planet[p].radius2;

	/* Translate to where the planet is */
	Vsub (v, planet[p].pos, player.pos);
	glTranslated (v[0], v[1], v[2]);
	th = fmod (absT, 120.0);
	th = 360.0*th / 120.0;

	/* Don't bother with oblicity or rotation if it's just a point */
	if (r2 < 200.0*200.0)
	{
		/* Planet oblicity */
		glRotated (planet[p].oblicity, 1.0, 0.0, 0.0);

		/* Moons get their primary's oblicity as well */
		if (planet[p].is_moon)
		{
			pr = planet[p].primary;
			glRotated (planet[pr].oblicity, 1.0, 0.0, 0.0);
		}
	}

	glPushMatrix();		/* Save un-rotated matrix */

	/* Planet rotation */
	if (r2 < 200.0*200.0) glRotated (th, 0.0, 0.0, 1.0);

	if (r2 < 5.0 * 5.0)
	{
		if (textures) glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, planet[p].texid);
		glCallList (planet[p].list320);
	}
	else if (r2 < 25.0 * 25.0)
	{
		if (textures) glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, planet[p].texid);
		glCallList (planet[p].list80);
	}
	else if (r2 < 200.0 * 200.0)
	{
		if (textures) glEnable (GL_TEXTURE_2D);
		glBindTexture (GL_TEXTURE_2D, planet[p].texid);
		glCallList (planet[p].list20);
	}
	else if (r2 < (5000.0 * 5000.0))
	{
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_LIGHTING);
		glPointSize (2.0);
		glBegin (GL_POINTS);
		glColor3fv (planet[p].color);
		glVertex3d (0.0, 0.0, 0.0);
		glEnd();
	}
	else if (r2 < (50000.0 * 50000.0))
	{
		glDisable (GL_TEXTURE_2D);
		glDisable (GL_LIGHTING);
		glPointSize (1.0);
		glBegin (GL_POINTS);
		glColor3fv (planet[p].color);
		glVertex3d (0.0, 0.0, 0.0);
		glEnd();
	}

	glDisable (GL_TEXTURE_2D);
	glEnable (GL_LIGHTING);

	/* Pop rotate matrix */
	glPopMatrix();

	glPopMatrix();
}

MakePlanetLists()
/*
 *  Define planet display lists
 */
{
	int p;

	for (p=0; p<NPLANETS; p++)
	{
		/* Sun is special */
		if (p == 0)
		{
			MakeSunList();
		}
		else
		{
			MakePlanetList (p);
		}
	}

	/* Make orbit lists */
	MakeOrbitLists();
}

MakePlanetList (p)
int p;
/*
 *  Make planet lists for planet p
 */
{
	/* We make three different versions of the planet for display at various
	   distances */
	if (glIsList (planet[p].list20)) glDeleteLists (planet[p].list20, 1);
	planet[p].list20 = glGenLists (1);

	glNewList (planet[p].list20, GL_COMPILE);
	maxtdiff = 0.47;
	Sphere (planet[p].radius, stacks0, slices0);
	glEndList();

	if (glIsList (planet[p].list80)) glDeleteLists (planet[p].list80, 1);
	planet[p].list80 = glGenLists (1);

	glNewList (planet[p].list80, GL_COMPILE);
	maxtdiff = 0.609384;
	Sphere (planet[p].radius, stacks1, slices1);
	glEndList();

	if (glIsList (planet[p].list320)) glDeleteLists (planet[p].list320, 1);
	planet[p].list320 = glGenLists (1);

	glNewList (planet[p].list320, GL_COMPILE);
	maxtdiff = 0.414069;
	Sphere (planet[p].radius, stacks2, slices2);
	glEndList();
}

MakeSunList()
/*
 *  Make Sol's display list
 */
{
	if (glIsList (planet[0].list20)) glDeleteLists (planet[0].list20, 1);
	planet[0].list20 = glGenLists (1);

	glNewList (planet[0].list20, GL_COMPILE);
	glDisable (GL_LIGHTING);
	glDisable (GL_TEXTURE_2D);
	glColor3d (0.8, 0.8, 0.6);
	Sphere (planet[0].radius, stacks0, slices0);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_LIGHTING);
	glEndList();

	if (glIsList (planet[0].list80)) glDeleteLists (planet[0].list80, 1);
	planet[0].list80 = glGenLists (1);

	glNewList (planet[0].list80, GL_COMPILE);
	glDisable (GL_LIGHTING);
	glDisable (GL_TEXTURE_2D);
	glColor3d (0.8, 0.8, 0.6);
	Sphere (planet[0].radius, stacks1, slices1);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_LIGHTING);
	glEndList();

	if (glIsList (planet[0].list320)) glDeleteLists (planet[0].list320, 1);
	planet[0].list320 = glGenLists (1);

	glNewList (planet[0].list320, GL_COMPILE);
	glDisable (GL_LIGHTING);
	glDisable (GL_TEXTURE_2D);
	glColor3d (0.8, 0.8, 0.6);
	Sphere (planet[0].radius, stacks2, slices2);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_LIGHTING);
	glEndList();
}

ReadPlanetTexture (p)
int p;
/*
 *  Read in the planet's texture
 */
{
	FILE *fd;
	int x, y, c, i;
	char fn[64];

	/* Sun is special */
	if (p == 0)
	{
		planet[p].color[0] = planet[p].color[1] = planet[p].color[2] = 1.0;
		return;
	}

	/* Zero planet average color */
	planet[p].color[0] = planet[p].color[1] = planet[p].color[2] = 0.0;

	/* Get id for this texture */
	if (planet[p].texid == (-1)) glGenTextures (1, &planet[p].texid);
	glBindTexture (GL_TEXTURE_2D, planet[p].texid);

	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

	/* Put up a progress message */
	Mprint ("Reading %s", planet[p].texfname);
	Log ("ReadPlanetTexture: Reading texture file %s", planet[p].texfname);
	DrawSplash();

	/* Open file */
	sprintf (fn, "maps/%s", planet[p].texfname);
	if (NULL == (fd = fopen (fn, "rb")))
	{
		Log ("ReadPlanetTexture: Can't open %s", planet[p].texfname);
		FinishSound();
		CloseLog();
		exit (0);
	}

	/* Read past PPM header */
	while (10 != fgetc(fd));
	while (10 != fgetc(fd));
	while (10 != fgetc(fd));

	/* Read in the color data */
	for (y=0; y<256; y++)
	{
		for (x=0; x<256; x++)
		{
			for (i=0; i<3; i++)
			{
				c = fgetc (fd);
				if ( (c < 0) || (c > 255) )
				{
					Log ("ReadPlanetTexture: Error reading texture file %s", planet[p].texfname);
					Log ("ReadPlanetTexture: x,y,i,c; %d %d %d %d", x, y, i, c);
					FinishSound();
					CloseLog();
					exit (0);
				}

				planet[p].tex[255-y][x][i] = 0xff & c;
				planet[p].color[i] += (float) c;
			}
		}
	}

	/* Compute average color */
	for (i=0; i<3; i++) planet[p].color[i] /= (256.0 * 256.0 * 256.0);

	/* Set the texture */
	glTexImage2D (GL_TEXTURE_2D, 0, 3, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, planet[p].tex);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	fclose (fd);
}

InitPlanets()
/*
 *  All sorts of planet initialization
 */
{
	int p;

	for (p=0; p<NPLANETS; p++)
	{
		planet[p].custom = 1;
		planet[p].texid = (-1);
		planet[p].orbitlist = (-1);
	}

	/* Randomize theta for each planet */
	RandomPlanets();

	/* Reset planets */
	ResetPlanets();
}

ResetPlanets()
/*
 *  Set planets back to the way they should be at start
 */
{
	int p;

	p = 0;

	Log ("ResetPlanets: resetting planets");

	strcpy (planet[p].name, "Sol");
	strcpy (planet[p].texfname, "sol.ppm");
	planet[p].radius = 696000.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.000000 / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.0;
	p++;

	strcpy (planet[p].name, "Mercury");
	strcpy (planet[p].texfname, "mercury.ppm");
	planet[p].radius = 2440.000000 / KM_TO_UNITS1;
	planet[p].dist = (realdistances ? 50.79 : 05.7900002) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].angvel = 0.004166 / 87.969;
	p++;

	strcpy (planet[p].name, "Venus");
	strcpy (planet[p].texfname, "venus.ppm");
	planet[p].radius = 6052.000000 / KM_TO_UNITS1;
	planet[p].dist = (realdistances ? 108.2 : 10.8199997) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 177.3;
	planet[p].angvel = 0.004166 / 224.7;
	p++;

	strcpy (planet[p].name, "Earth");
	strcpy (planet[p].texfname, "earth.ppm");
	planet[p].radius = 6371.000000 / KM_TO_UNITS1;
	planet[p].dist = (realdistances ? 149.6 : 14.9600006) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 23.45;
	planet[p].angvel = 0.004166 / 365.256;
	p++;

	strcpy (planet[p].name, "Moon");
	strcpy (planet[p].texfname, "moon.ppm");
	planet[p].radius = 1737.500000 / KM_TO_UNITS1;
	planet[p].dist = 0.384400 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 3;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 27.3;
	p++;

	strcpy (planet[p].name, "Mars");
	strcpy (planet[p].texfname, "mars.ppm");
	planet[p].radius = 3390.000000 / KM_TO_UNITS1;
	planet[p].dist = (realdistances ? 227.9 : 22.7899994) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 25.19;
	planet[p].angvel = 0.004166 / 686.98;
	p++;

	strcpy (planet[p].name, "Phobos");
	strcpy (planet[p].texfname, "phobos.ppm");
	planet[p].radius = 13.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.009380 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 5;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 0.319;
	p++;

	strcpy (planet[p].name, "Deimos");
	strcpy (planet[p].texfname, "deimos.ppm");
	planet[p].radius = 8.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.022340 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 5;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 1.262;
	p++;

	strcpy (planet[p].name, "Jupiter");
	strcpy (planet[p].texfname, "jupiter.ppm");
	planet[p].radius = 69911.000000 / KM_TO_UNITS1;
/*	planet[p].dist = 77.8400024 / KM_TO_UNITS2;	*/
	planet[p].dist = (realdistances ? 778.4 : 50.0) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 3.12;
	planet[p].angvel = 0.004166 / 4332.0;
	p++;

	strcpy (planet[p].name, "Io");
	strcpy (planet[p].texfname, "io.ppm");
	planet[p].radius = 1821.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.421000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 8;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 1.769;
	p++;

	strcpy (planet[p].name, "Europa");
	strcpy (planet[p].texfname, "europa.ppm");
	planet[p].radius = 1565.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.671000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 8;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 3.552;
	p++;

	strcpy (planet[p].name, "Ganymede");
	strcpy (planet[p].texfname, "ganymede.ppm");
	planet[p].radius = 2634.000000 / KM_TO_UNITS1;
	planet[p].dist = 1.070000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 8;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 7.155;
	p++;

	strcpy (planet[p].name, "Callisto");
	strcpy (planet[p].texfname, "callisto.ppm");
	planet[p].radius = 2403.000000 / KM_TO_UNITS1;
	planet[p].dist = 1.883000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 8;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 16.69;
	p++;

	strcpy (planet[p].name, "Saturn");
	strcpy (planet[p].texfname, "saturn.ppm");
	planet[p].radius = 58232.000000 / KM_TO_UNITS1;
/*	planet[p].dist = 142.7000000 / KM_TO_UNITS2;	*/
	planet[p].dist = (realdistances ? 1427.0 : 75.0) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 26.73;
	planet[p].angvel = 0.004166 / 10759.0;
	p++;

	strcpy (planet[p].name, "Mimas");
	strcpy (planet[p].texfname, "mimas.ppm");
	planet[p].radius = 198.800003 / KM_TO_UNITS1;
	planet[p].dist = 0.185000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 13;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 0.942;
	p++;

	strcpy (planet[p].name, "Enceladus");
	strcpy (planet[p].texfname, "enceladus.ppm");
	planet[p].radius = 249.100006 / KM_TO_UNITS1;
	planet[p].dist = 0.238000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 13;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 1.37;
	p++;

	strcpy (planet[p].name, "Tethys");
	strcpy (planet[p].texfname, "tethys.ppm");
	planet[p].radius = 529.900024 / KM_TO_UNITS1;
	planet[p].dist = 0.294000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 13;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 1.888;
	p++;

	strcpy (planet[p].name, "Dione");
	strcpy (planet[p].texfname, "dione.ppm");
	planet[p].radius = 560.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.377000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 13;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 2.737;
	p++;

	strcpy (planet[p].name, "Rhea");
	strcpy (planet[p].texfname, "rhea.ppm");
	planet[p].radius = 764.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.527000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 13;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 4.518;
	p++;

	strcpy (planet[p].name, "Titan");
	strcpy (planet[p].texfname, "titan.ppm");
	planet[p].radius = 2575.000000 / KM_TO_UNITS1;
	planet[p].dist = 1.221000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 13;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 15.9;
	p++;

	strcpy (planet[p].name, "Iapetus");
	strcpy (planet[p].texfname, "iapetus.ppm");
	planet[p].radius = 718.000000 / KM_TO_UNITS1;
	planet[p].dist = 3.561000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 13;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 79.33;
	p++;

	strcpy (planet[p].name, "Uranus");
	strcpy (planet[p].texfname, "uranus.ppm");
	planet[p].radius = 25362.000000 / KM_TO_UNITS1;
/*	planet[p].dist = 287.1000000 / KM_TO_UNITS2;	*/
	planet[p].dist = (realdistances ? 2871.0 : 100.0) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 97.86;
	planet[p].angvel = 0.004166 / 30685.0;
	p++;

	strcpy (planet[p].name, "Miranda");
	strcpy (planet[p].texfname, "miranda.ppm");
	planet[p].radius = 235.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.130000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 21;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 1.413;
	p++;

	strcpy (planet[p].name, "Ariel");
	strcpy (planet[p].texfname, "ariel.ppm");
	planet[p].radius = 580.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.191000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 21;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 2.52;
	p++;

	strcpy (planet[p].name, "Umbriel");
	strcpy (planet[p].texfname, "umbriel.ppm");
	planet[p].radius = 585.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.266000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 21;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 4.144;
	p++;

	strcpy (planet[p].name, "Titania");
	strcpy (planet[p].texfname, "titania.ppm");
	planet[p].radius = 789.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.436000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 21;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 8.706;
	p++;

	strcpy (planet[p].name, "Oberon");
	strcpy (planet[p].texfname, "oberon.ppm");
	planet[p].radius = 761.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.583000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 21;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 13.463;
	p++;

	strcpy (planet[p].name, "Neptune");
	strcpy (planet[p].texfname, "neptune.ppm");
	planet[p].radius = 24766.000000 / KM_TO_UNITS1;
/*	planet[p].dist = 449.8000000 / KM_TO_UNITS2;	*/
	planet[p].dist = (realdistances ? 4498.0 : 125.0) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 29.56;
	planet[p].angvel = 0.004166 / 60189.0;
	p++;

	strcpy (planet[p].name, "Triton");
	strcpy (planet[p].texfname, "triton.ppm");
	planet[p].radius = 1352.599976 / KM_TO_UNITS1;
	planet[p].dist = 0.355000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 27;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / -5.877;
	p++;

	strcpy (planet[p].name, "Proteus");
	strcpy (planet[p].texfname, "proteus.ppm");
	planet[p].radius = 218.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.118000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 27;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 1.122;
	p++;

	strcpy (planet[p].name, "Pluto");
	strcpy (planet[p].texfname, "pluto.ppm");
	planet[p].radius = 1137.000000 / KM_TO_UNITS1;
/*	planet[p].dist = 590.6000000 / KM_TO_UNITS2;	*/
	planet[p].dist = (realdistances ? 5906.0 : 150.0) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 122.0;
	planet[p].angvel = 0.004166 / 90465.0;
	p++;
	
	strcpy (planet[p].name, "Charon");
	strcpy (planet[p].texfname, "charon.ppm");
	planet[p].radius = 586.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.019000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 30;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 6.387;
	p++;
	
	strcpy (planet[p].name, "Charon2");
	strcpy (planet[p].texfname, "charon.ppm");
	planet[p].radius = 150.000000 / KM_TO_UNITS1;
	planet[p].dist = 0.019000 / KM_TO_UNITS2;
	planet[p].is_moon = 1;
	planet[p].primary = 31;
	planet[p].oblicity = 0.0;
	planet[p].angvel = 0.004166 / 6.387;
	p++;
	
	strcpy (planet[p].name, "New");
	strcpy (planet[p].texfname, "new.ppm");
	planet[p].radius = 856080.000000 / KM_TO_UNITS1;
/*	planet[p].dist = 590.6000000 / KM_TO_UNITS2;	*/
	planet[p].dist = (realdistances ? 15906.0 : 1000.0) / KM_TO_UNITS2;
	planet[p].is_moon = 0;
	planet[p].primary = 0;
	planet[p].oblicity = 122.0;
	planet[p].angvel = 0.004166 / 90465.0;
	p++;



	/* Randomize planet positions */
	PositionPlanets();

	/* Read textures, etc */
	for (p=0; p<NPLANETS; p++)
	{
		if (planet[p].custom)
		{
			ReadPlanetTexture (p);
			planet[p].custom = 0;
		}

		planet[p].radius2 = planet[p].radius * planet[p].radius;
		planet[p].hidden = 0;

		/* Set planet's mass */
		planet[p].mass = planet[p].radius * planet[p].radius * planet[p].radius;
	}

	/* Define planet display lists */
	MakePlanetLists();
}

RandomPlanets()
/*
 *  Randomly set solar angle for each planet
 */
{
	int p;

	Log ("RandomPlanets: Randomizing planets");

	for (p=0; p<NPLANETS; p++)
	{
		planet[p].theta = 2.0 * rnd (6.28);
	}
}

PositionPlanets()
/*
 *  Turn planet angles into Cartesian coordinates
 */
{
	int p, pr;
	double th, v[3], v1[3];

	/* Position the planets */
	for (p=0; p<NPLANETS; p++)
	{
		th = planet[p].theta;

		planet[p].pos[0] = planet[p].dist * sin (th);
		planet[p].pos[1] = planet[p].dist * cos (th);
		planet[p].pos[2] = 0.0;

		/* If moon, compute absolute position based on primary's
		   position and oblicity */
		if (planet[p].is_moon)
		{
			pr = planet[p].primary;
			th = -planet[pr].oblicity * 3.1415926535 / 180.0;
			v[0] = 1.0;
			v[1] = 0.0;
			v[2] = 0.0;
			RotateAbout (v1, planet[p].pos, v, th);
			Vset (planet[p].pos, v1);
			Vadd (planet[p].pos, planet[p].pos, planet[pr].pos);
		}
	}
}

int InsidePlanet (v)
double v[3];
/*
 *  Check if v is inside a planet.  If yes, return planet
 *  index, else -1
 */
{
	int p;
	double v1[3], r2;

	for (p=0; p<NPLANETS; p++)
	{
		if (!planet[p].hidden)
		{
			Vsub (v1, v, planet[p].pos);
			r2 = Mag2 (v1);

			/* Inside? */
			if (r2 < planet[p].radius2)
			{
				/* Yep */
				return (p);
			}
		}
	}

	/* Nope */
	return (-1);
}

ShowPlanetNames()
{
	int p;
	unsigned char v;
	double v1[3];
	char buf[128];

	for (p=0; p<NPLANETS; p++)
	{
		if (!planet[p].hidden)
		{
			if (planet[p].is_moon)
				glColor3d (0.0, 0.8, 0.8);
			else
				glColor3d (0.0, 0.0, 0.8);

			Vsub (v1, planet[p].pos, player.pos);
			glRasterPos3dv (v1);
			glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &v);
			if (v)
			{
				if (!planet[p].is_moon)
				{
					if (planet[p].absrange2 < 100.0*planet[p].radius*planet[p].radius)
					{
						sprintf (buf, "%s:%.0lf", planet[p].name,
							KM_TO_UNITS1*(sqrt(planet[p].absrange2)-planet[p].radius));
					}
					else
					{
						strcpy (buf, planet[p].name);
					}
					Print (GLUT_BITMAP_HELVETICA_10, buf);
				}
				else
				{
					if (planet[p].range2 < (2000.0 * 2000.0))
					{
						if (planet[p].absrange2 < 100.0*planet[p].radius*planet[p].radius)
						{
							sprintf (buf, "%s:%.0lf", planet[p].name,
								KM_TO_UNITS1*(sqrt(planet[p].absrange2)-planet[p].radius));
						}
						else
						{
							strcpy (buf, planet[p].name);
						}
						Print (GLUT_BITMAP_HELVETICA_10, buf);
					}
				}
			}
		}
	}
}

Sphere (r, slices, sectors)
double r;
int slices, sectors;
{
	GLUquadricObj *obj;

	obj = gluNewQuadric();
	
	gluQuadricDrawStyle (obj, GLU_FILL);
	gluQuadricNormals (obj, GLU_SMOOTH);
	gluQuadricTexture (obj, GL_TRUE);
	gluSphere (obj, (float)r, sectors, slices);
}

int FindPlanetByName (c)
char *c;
/*
 *  Find planet given name, else -1
 */
{
	int p;

	for (p=0; p<NPLANETS; p++)
	{
		if (!strcasecmp (c, planet[p].name)) return (p);
	}

	return (-1);
}

MakeOrbitLists()
/*
 *  Construct display lists for orbits
 */
{
	int p;
	double th, d, s, c;

	d = 360.0 / ((double) ORBIT_SECTORS);

	/* Loop through planets */
	for (p=1; p<NPLANETS; p++)
	{
		/* Delete old list */
		if (glIsList (planet[p].orbitlist)) glDeleteLists (planet[p].orbitlist, 1);

		/* Make new list */
		planet[p].orbitlist = glGenLists (1);
		glNewList (planet[p].orbitlist, GL_COMPILE);

		glBegin (GL_LINE_LOOP);

		/* Set color */
		if (planet[p].is_moon)
		{
			glColor3f (0.0, 0.8, 0.8);
		}
		else
		{
			glColor3f (0.0, 0.0, 1.0);
		}

		/* Loop through circle */
		for (th=0.0; th<360.0; th+=d)
		{
			s = sin (th*3.1415926535/180.0);
			c = cos (th*3.1415926535/180.0);

			glVertex3d (planet[p].dist*s, planet[p].dist*c, 0.0);
		}

		glEnd();
		glEndList();
	}
}

DrawOrbits()
/*
 *  Draw orbital paths
 */
{
	int p;
	double v[3];

	glDisable (GL_TEXTURE_2D);
	glDisable (GL_LIGHTING);

	glPushMatrix();
	Vsub (v, planet[0].pos, player.pos);
	glTranslated (v[0], v[1], v[2]);

	for (p=1; p<NPLANETS; p++)
	{
		if (!planet[p].hidden)
		{
			if (planet[p].is_moon)
			{
				if (planet[planet[p].primary].range2 > 200.0*200.0) continue;

				glPushMatrix();
				Vsub (v, player.pos, planet[0].pos);
				glTranslated (v[0], v[1], v[2]);

				Vsub (v, planet[planet[p].primary].pos, player.pos);
				glTranslated (v[0], v[1], v[2]);

				glRotated (planet[planet[p].primary].oblicity, 1.0, 0.0, 0.0);
			}
			glCallList (planet[p].orbitlist);
			if (planet[p].is_moon) glPopMatrix();
		}
	}

	glPopMatrix();
}

MovePlanets()
/*
 *  Move planets in their orbits
 */
{
	int p;

	for (p=1; p<NPLANETS; p++)
	{
		planet[p].theta -= planet[p].angvel * deltaT * compression
		                   * 3.1415926535 / 180.0;
		if (planet[p].theta < 0.0) planet[p].theta += 2.0 * 3.1415926535;
	}

	PositionPlanets();
}

