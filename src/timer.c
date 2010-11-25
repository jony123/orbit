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

double Time ()
{
	static double oldtime=0.0;
	double newtime, elapsed;

	newtime = glutGet (GLUT_ELAPSED_TIME);
	elapsed = newtime - oldtime;
	oldtime = newtime;

	return (elapsed/1000.0);
}

InitTimer()
/*
 *  Check for and initialize the performance timer
 */
{
#ifdef WIN32
	/* This gives a compiler warning; not completely sure why */
	if (!QueryPerformanceFrequency (&ticks_per_sec)) exit (0);
#endif
	last_ticks = 0;
}

DeltaTime()
/*
 *  Figure out how much time has elapsed since the last time we were called
 */
{
#ifdef WIN32
	_int64 ticks, elapsed;
#else
	int ticks, elapsed;
#endif

	if (paused)
	{
		deltaT = 0.0;
		return 0;
	}

#ifdef WIN32
	/* This gives a compiler warning; not completely sure why */
	QueryPerformanceCounter (&ticks);
#else
	ticks = glutGet (GLUT_ELAPSED_TIME);
#endif

	if (last_ticks == 0)
	{
		deltaT = 0.0;
	}
	else
	{
		elapsed = ticks - last_ticks;
#ifdef WIN32
		deltaT = ((double) elapsed) / ((double) ticks_per_sec);
#else
		deltaT = ((double) elapsed) / 1000.0;
#endif
	}

	/* If more than maximum, slow things down */
	if (deltaT > MAXDELTAT) deltaT = MAXDELTAT;

	absT += deltaT;
	last_ticks = ticks;

	return 0;
}

