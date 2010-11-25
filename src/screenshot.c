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

ScreenShot()
{
	char fname[32];
	char *screen;
	FILE *fd;
	int x, y, c;

	/* Construct file name */
	sprintf (fname, "orbit%03d.ppm", screen_shot_num);
	screen_shot_num++;

	/* Allocate space for screen */
	if (NULL == (screen = (char *) malloc (ScreenWidth*ScreenHeight*3)))
	{
		Cprint ("Can't allocate memory for screen shot");
		return;
	}

	/* Get frame buffer */
	glReadPixels (0, 0, ScreenWidth, ScreenHeight, GL_RGB,
		GL_UNSIGNED_BYTE, screen);

	/* Open the file */
	if (NULL == (fd = fopen (fname, "wb")))
	{
		Cprint ("Can't open screen shot file");
		free (screen);
		return;
	}

	/* Write the PPM file */
	fprintf (fd, "P6\n%d %d\n255\n", ScreenWidth, ScreenHeight);

	for (y=0; y<ScreenHeight; y++)
	{
		for (x=0; x<ScreenWidth; x++)
		{
			c = 0xff & *screen++;
			fputc (c, fd);
			c = 0xff & *screen++;
			fputc (c, fd);
			c = 0xff & *screen++;
			fputc (c, fd);
		}
	}

	/* Close and clean up */
	fclose (fd);
	free (screen);
	Cprint ("Screen shot saved in %s", fname);
}

