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

/*
 *  Read and write the preferences file
 */

WritePrefs()
{
	FILE *fd;

	Log ("WritePrefs: Writing preferences file");

	/* Open it */
	if (NULL == (fd = fopen ("prefs.txt", "wt")))
	{
		Log ("WritePrefs: Can't open prefs.txt");
		return;
	}

	/* Write stuff */
	fprintf (fd, "mission %s\n", mission.fn);
	fprintf (fd, "screenwidth %d\n", ScreenWidth);
	fprintf (fd, "screenheight %d\n", ScreenHeight);
	fprintf (fd, "fullscreen %d\n", fullscreen);
	fprintf (fd, "gamemode %s\n", gamemode);
	fprintf (fd, "name %s\n", player.name);
	fprintf (fd, "model %s\n", player.model);
	fprintf (fd, "vulnerable %d\n", vulnerable);
	fprintf (fd, "fov %lf\n", fov);
	fprintf (fd, "joy_throttle %d\n", joy_throttle);
	fprintf (fd, "deadzone %lf\n", deadzone);
	fprintf (fd, "starfield %d\n", starfield);
	fprintf (fd, "drawhud %d\n", drawhud);
	fprintf (fd, "showfps %d\n", showfps);
	fprintf (fd, "gravity %d\n", gravity);
	fprintf (fd, "draw_orbits %d\n", draw_orbits);
	fprintf (fd, "orbit %d\n", orbit);
	fprintf (fd, "compression %lf\n", compression);
	fprintf (fd, "junk %d\n", junk);
	fprintf (fd, "sound %d\n", sound);
	fprintf (fd, "show_names %d\n", show_names);
	fprintf (fd, "screen_shot_num %d\n", screen_shot_num);
	fprintf (fd, "rings %d\n", rings);
	fprintf (fd, "ring_sectors %d\n", ring_sectors);
	fprintf (fd, "textures %d\n", textures);
	fprintf (fd, "realdistances %d\n", realdistances);
	fprintf (fd, "weapon %d\n", player.weapon);
	fprintf (fd, "mouse_control %d\n", mouse_control);
	fprintf (fd, "mouse_flipx %d\n", mouse.flipx);
	fprintf (fd, "mouse_flipy %d\n", mouse.flipy);
	fprintf (fd, "flightmodel %d\n", player.flightmodel);
	fprintf (fd, "fullstop %d\n", fullstop);
	fprintf (fd, "superwarp %d\n", superwarp);
	fprintf (fd, "port %d\n", server.port);
	fprintf (fd, "posx %lf\n", player.pos[0]);
	fprintf (fd, "posy %lf\n", player.pos[1]);
	fprintf (fd, "posz %lf\n", player.pos[2]);
	fprintf (fd, "slices0 %d\n", slices0);
	fprintf (fd, "stacks0 %d\n", stacks0);
	fprintf (fd, "slices1 %d\n", slices1);
	fprintf (fd, "stacks1 %d\n", stacks1);
	fprintf (fd, "slices2 %d\n", slices2);
	fprintf (fd, "stacks2 %d\n", stacks2);

	/* Adios */
	fclose (fd);
}

ReadPrefs()
{
	FILE *fd;
	char c1[64], c2[64];

	/* Open it */
	if (NULL == (fd = fopen ("prefs.txt", "rt"))) return;

	Log ("ReadPrefs: Reading preferences file");
		
	/* Read Stuff */
	while (2 == fscanf (fd, "%s %s", c1, c2))
	{
		Log ("ReadPrefs: %s %s", c1, c2);

		if (!strcasecmp (c1, "vulnerable")) vulnerable = atoi(c2);
		else if (!strcasecmp (c1, "joy_throttle")) joy_throttle = atoi(c2);
		else if (!strcasecmp (c1, "deadzone")) deadzone = atof(c2);
		else if (!strcasecmp (c1, "starfield")) starfield = atoi(c2);
		else if (!strcasecmp (c1, "drawhud")) drawhud = atoi(c2);
		else if (!strcasecmp (c1, "showfps")) showfps = atoi(c2);
		else if (!strcasecmp (c1, "gravity")) gravity = atoi(c2);
		else if (!strcasecmp (c1, "junk")) junk = atoi(c2);
		else if (!strcasecmp (c1, "sound")) sound = atoi(c2);
		else if (!strcasecmp (c1, "show_names")) show_names = atoi(c2);
		else if (!strcasecmp (c1, "screen_shot_num")) screen_shot_num = atoi(c2);
		else if (!strcasecmp (c1, "mission")) strcpy (mission.fn, c2);
		else if (!strcasecmp (c1, "name")) strcpy (player.name, c2);
		else if (!strcasecmp (c1, "model")) strcpy (player.model, c2);
		else if (!strcasecmp (c1, "rings")) rings = atoi(c2);
		else if (!strcasecmp (c1, "textures")) textures = atoi(c2);
		else if (!strcasecmp (c1, "realdistances")) realdistances = atoi(c2);
		else if (!strcasecmp (c1, "screenwidth")) ScreenWidth = atoi(c2);
		else if (!strcasecmp (c1, "screenheight")) ScreenHeight = atoi(c2);
		else if (!strcasecmp (c1, "fov")) fov = atof(c2);
		else if (!strcasecmp (c1, "weapon")) player.weapon = atoi(c2);
		else if (!strcasecmp (c1, "fullscreen")) fullscreen = atoi(c2);
		else if (!strcasecmp (c1, "gamemode")) strcpy (gamemode, c2);
		else if (!strcasecmp (c1, "mouse_control")) mouse_control = atoi(c2);
		else if (!strcasecmp (c1, "mouse_flipx")) mouse.flipx = atoi(c2);
		else if (!strcasecmp (c1, "mouse_flipy")) mouse.flipy = atoi(c2);
		else if (!strcasecmp (c1, "flightmodel")) player.flightmodel = atoi(c2);
		else if (!strcasecmp (c1, "ring_sectors")) ring_sectors = atoi(c2);
		else if (!strcasecmp (c1, "fullstop")) fullstop = atoi(c2);
		else if (!strcasecmp (c1, "superwarp")) superwarp = atoi(c2);
		else if (!strcasecmp (c1, "port")) server.port = atoi(c2);
		else if (!strcasecmp (c1, "posx")) player.pos[0] = atof(c2);
		else if (!strcasecmp (c1, "posy")) player.pos[1] = atof(c2);
		else if (!strcasecmp (c1, "posz")) player.pos[2] = atof(c2);
		else if (!strcasecmp (c1, "slices0")) slices0 = atoi(c2);
		else if (!strcasecmp (c1, "stacks0")) stacks0 = atoi(c2);
		else if (!strcasecmp (c1, "slices1")) slices1 = atoi(c2);
		else if (!strcasecmp (c1, "stacks1")) stacks1 = atoi(c2);
		else if (!strcasecmp (c1, "slices2")) slices2 = atoi(c2);
		else if (!strcasecmp (c1, "stacks2")) stacks2 = atoi(c2);
		else if (!strcasecmp (c1, "draw_orbits")) draw_orbits = atoi(c2);
		else if (!strcasecmp (c1, "orbit")) orbit = atoi(c2);
		else if (!strcasecmp (c1, "compression")) compression = atof(c2);
		else
		{
			Log ("ReadPrefs: Unrecognized entry: %s %s", c1, c2);
		}
	}

	/* Sayanora */
	fclose (fd);
}

