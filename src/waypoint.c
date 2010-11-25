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
 *  Routines to implement waypoints
 */

InitWaypoints()
/*
 *  Reset waypoints
 */
{
	int w;

	nwaypoints = 0;
	player.waypoint = (-1);

	for (w=0; w<NWAYPOINTS; w++)
	{
		waypoint[w].pos[0] = 0.0;
		waypoint[w].pos[1] = 0.0;
		waypoint[w].pos[2] = 0.0;
	}
}

AddWaypoint (v)
double v[3];
/*
 *  Define a new waypoint
 */
{
	if (nwaypoints == NWAYPOINTS)
	{
		Log ("AddWaypoint: Too many waypoints.  Increase NWAYPOINTS in orbit.h");
		return;
	}

	Vset (waypoint[nwaypoints].pos, v);
	nwaypoints++;
}

NextWaypoint()
/*
 *  Advance to next waypoint
 */
{
	if (nwaypoints == 0) return;

	if (player.waypoint == (-1))
	{
		player.waypoint = 0;
		return;
	}

	player.waypoint++;

	if (player.waypoint == nwaypoints)
	{
		player.waypoint = (-1);
		return;
	}
}

PrevWaypoint()
/*
 *  Back to previous waypoint
 */
{
	if (nwaypoints == 0) return;

	if (player.waypoint == 0)
	{
		player.waypoint = (-1);
		return;
	}

	if (player.waypoint == (-1))
	{
		player.waypoint = nwaypoints - 1;
		return;
	}

	player.waypoint--;
}
