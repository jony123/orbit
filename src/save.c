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
 *  Routines to save and load missions
 */

ReadSaves()
/*
 *  Write the save file
 */
{
	FILE *fd;

	nsaves = 0;

	/* Open the save file */
	if (NULL == (fd = fopen ("saves.txt", "rt")))
	{
		/* No saves */
		return;
	}

	/* Read through the file */
	while (2 == (fscanf (fd, "%s %d", save[nsaves].fn,
			&save[nsaves].time)))
	{
		nsaves++;

		/* Too many? */
		if (nsaves >= NSAVES)
		{
			nsaves = NSAVES;
			fclose (fd);
			return;
		}
	}

	/* That's it! */
	fclose (fd);
	return;
}

WriteSaves()
/*
 *  Write the save file
 */
{
	FILE *fd;
	int s;

	/* Open the file */
	if (NULL == (fd = fopen ("saves.txt", "wt")))
	{
		Mprint ("Can't open save file!");
		return;
	}

	for (s=0; s<nsaves; s++)
	{
		fprintf (fd, "%s %d\n", save[s].fn, save[s].time);
	}

	/* All done */
	fclose (fd);

	return;
}

AddSave (fn)
char *fn;
/*
 *  Add a mission to the list of saves
 */
{
	int s, sm;

	/* Read the file */
	ReadSaves();

	/* See if we can find one with same mission name */
	sm = -1;
	for (s=0; s<nsaves; s++)
	{
		if (!strcmp (fn, save[s].fn))
		{
			/* Found one */
			sm = s;
			break;
		}
	}

	/* Didn't find one */
	if (sm < 0)
	{
		if (nsaves < NSAVES) nsaves++;
		sm = nsaves - 1;
	}

	/* Make room */
	for (s=sm; s>0; s--)
	{
		strcpy (save[s].fn, save[s-1].fn);
		save[s].time = save[s-1].time;
	}

	/* Plop this one in */
	strcpy (save[0].fn, fn);
	save[0].time = time(NULL);

	/* Write out the file */
	WriteSaves();
}

LoadGame()
/*
 *  Sparky wants to load a game
 */
{
	int s;

	char buf[4096], buf2[32];

	/* Read the saves file */
	ReadSaves();

	/* Don't bother if no saves */
	if (nsaves == 0)
	{
		Mprint ("No saved games!");
		return;
	}

	/* Pause the game */
	paused = 1;

	/* Set the game state */
	state = STATE_LOADGAME;

	/* Construct the selection list */
	strcpy (buf, "Select mission to load:\\\\");

	for (s=0; s<nsaves; s++)
	{
		sprintf (buf2, "%d) ", s);
		strcat (buf, buf2);
		strcat (buf, save[s].fn);
		strcat (buf, " ");
		sprintf (buf2, "%s", ctime((const time_t *)&save[s].time));
		strcat (buf, buf2);
		strcat (buf, "\\");
	}

	Mprint (buf);
}

LoadGameByKey (k)
int k;
/*
 *  Load game depending on the key Sparky pressed
 */
{
	if ( (k < 0) || (k >= nsaves) ) return;

	/* Set state back to normal */
	state = STATE_NORMAL;

	/* Read the mission */
	ReadMission (save[k].fn);

	/* That's it! */
	return;
}

