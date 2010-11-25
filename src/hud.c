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

static GLubyte throt_stipple[] = {
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
	};

InitHud()
/*
 *  Initialize the HUD
 */
{
	InitRadar();
	MakeRadarCircleList();

	/* Figure out throttle coords */
	hud.throt_min[0] = radar.fcenter[0] - 1.4*radar.fradius;
	hud.throt_min[1] = radar.fcenter[1] - 0.9*radar.fradius;
	hud.throt_max[0] = hud.throt_min[0] + 0.2*radar.fradius;
	hud.throt_max[1] = hud.throt_min[1] + 1.8*radar.fradius;
	hud.throt_mid[0] = hud.throt_min[0];
	hud.throt_mid[1] = (hud.throt_min[1] + hud.throt_max[1]) / 2.0;

	/* Target name */
	hud.targ_name[0] = radar.fcenter[0] + 2.0*radar.fradius;
	hud.targ_name[1] = radar.fcenter[1] - radar.fradius + 10.0;

	/* Target range */
	hud.targ_range[0] = hud.targ_name[0];
	hud.targ_range[1] = hud.targ_name[1] - 10.0;

	/* Waypoint */
	hud.waypoint[0] = hud.targ_range[0];
	hud.waypoint[1] = hud.targ_range[1];

	/* Weapon name */
	hud.weapon[0] = hud.targ_name[0];
	hud.weapon[1] = radar.fcenter[1] + radar.fradius - 20.0;

	/* Velocity */
	hud.vel[0] = radar.fcenter[0] - 3.0 * radar.fradius;
	hud.vel[1] = hud.throt_mid[1] - 5.0;

	/* Shields display */
	hud.shields_min[0] = radar.fcenter[0] + 1.2*radar.fradius;
	hud.shields_max[0] = hud.shields_min[0] + 0.2*radar.fradius;
	hud.shields_min[1] = hud.throt_min[1];
	hud.shields_max[1] = hud.throt_max[1];

	/* Target shields */
	hud.targshields_min[0] = radar.fcenter[0] + 1.5*radar.fradius;
	hud.targshields_max[0] = hud.targshields_min[0] + 0.2*radar.fradius;
	hud.targshields_min[1] = hud.throt_min[1];
	hud.targshields_max[1] = hud.throt_max[1];

	lock.target = -1;
	lock.type = LOCK_ENEMY;
}

static GLubyte center_cursor[8] =
	{ 0x10, 0x10, 0x00, 0xc6, 0x00, 0x10, 0x10, 0x00 };

Hud()
/*
 *  Show the Heads Up Display
 */
{
	char buf[256];

	/* Set up viewing matrix */
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	glOrtho (0.0, ((double)ScreenWidth), 0.0, ((double)ScreenHeight), -1.0, 1.0);

	/* Disable three-D stuff */
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_LIGHTING);

	/* Try displaying a crosshair */
	if (drawhud)
	{
		glColor3d (0.8, 0.8, 0.8);
		glRasterPos2i (ScreenWidth/2, ScreenHeight/2);
		glBitmap (8, 8, 3.0, 5.0, 0.0, 0.0, center_cursor);

		/* Display the radar */
		Radar();

		/* Display the throttle */
		Throttle();

		/* Velocity */
		PlayerVel();

		/* Show target stuff */
		TargetStuff();

		/* Current weapon */
		ShowWeapon();

		/* Waypoint stuff */
		WaypointStuff();

		/* Shields display */
		Shields();

		/* Print the performance monitor */
		if (showfps)
		{
			glColor3d (1.0, 1.0, 1.0);
			glRasterPos2i (1, 5);

			sprintf (buf, "fps: %2.1f  ", fps);
			Print (GLUT_BITMAP_HELVETICA_10, buf);

			/* Show bandwidth */
			if (am_client || am_server)
			{
				if (recv_bps > 0.0)
				{
					glColor3d (1.0, 0.0, 0.0);
					glRasterPos2i (1, 15);
					sprintf (buf, "R:%4.0lf  ", recv_bps);
					Print (GLUT_BITMAP_HELVETICA_10, buf);
				}

				if (xmit_bps > 0.0)
				{
					glColor3d (0.0, 1.0, 0.0);
					glRasterPos2i (1, 25);
					sprintf (buf, "S:%4.0lf  ", xmit_bps);
					Print (GLUT_BITMAP_HELVETICA_10, buf);
				}
			}
		}
	}

	/* Display the message console */
	DisplayConsole();

	/* Display ceter-of-screen messages */
	DrawMessage();

	glMatrixMode (GL_PROJECTION);
	glPopMatrix();

	glMatrixMode (GL_MODELVIEW);
	glPopMatrix();

	/* Draw various on-screen cursors */
	if (drawhud)
	{
		/* The locked target */
		DrawLock();

		/* Motion cursors */
		DrawMotionCursors();

		/* Waypoint cursor */
		DrawWaypoint();
	}

	/* Re-enable three-D stuff */
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);
	glEnable (GL_LIGHTING);
}

RadarCoords (v, x, y)
double v[3], *x, *y;
/*
 *  Compute the two-D radar coordinates of an object
 */
{
	double v2[3], v3[3], v4[3], t, r;

	/* Compute coords of object relative to player */
	Vsub (v2, v, player.pos);

	/* Normalize that */
	Normalize (v2);

	/* Distance of blip from center of radar is cosine of angle
	   between view direction and direction to object */
	radarR = Dotp (player.view, v2);

	/* Find magnitude of projection of v2 onto view vector */
	t = Dotp (v2, player.view);

	/* Determine projection of object on plane perpendicular
	   to viewing plane */
	Vmul (v3, player.view, -t);
	Vadd (v4, v3, v2);

	/* Handle special case of null v4, meaning object is on
	   line of sight */
	if (0.0 == Mag2(v4))
	{
		radarCOS = 1.0;
		radarSIN = 0.0;
		return;
	}

	/* Else normalize v4 */
	Normalize (v4);

	/* v4 is now a vector from player's viewpoint to projection
	   of object on the player's plane.  Compute angle from player's
	   up vector to object */
	radarCOS = Dotp (player.up, v4);
	radarSIN = Dotp (player.right, v4);

	/* Convert to cartesian */
	r = (1.0 - radarR) / 2.0;
	*x = radar.fcenter[0] + radar.fradius * -r * radarSIN;
	*y = radar.fcenter[1] + radar.fradius * r * radarCOS;
}

Radar()
/*
 *  Display the Wing Commander-like radar
 */
{
	/* Draw the circle around the radar */
	DrawRadarCircle();

	/* Draw the objects */
	RadarPlanets();
	RadarTargets();
	RadarMissiles();
	RadarWaypoint();
}

InitRadar()
/*
 *  Set up the radar coordinates
 */
{
	radar.fcenter[0] = ScreenWidth / 2.0;
	radar.center[0] = (int) radar.fcenter[0];

	radar.fcenter[1] = ScreenHeight / 8.0;
	radar.center[1] = (int) radar.fcenter[1];

	radar.fradius = ScreenHeight / 8.0 - 10.0;
	radar.radius = (int) radar.fradius;
}

DrawRadarCircle()
{
	glCallList (radar.list);
}

MakeRadarCircleList()
/*
 *  Define the radar circle list
 */
{
	double x, y, theta;

	/* Delete the list if it already exists */
	if (glIsList (radar.list)) glDeleteLists (radar.list, 1);

	radar.list = glGenLists (1);
	glNewList (radar.list, GL_COMPILE);

	glEnable (GL_POLYGON_STIPPLE);
	glPolygonStipple (throt_stipple);

	/* Fill in radar circle */
	glColor3d (0.0, 0.0, 0.1);
	glBegin (GL_POLYGON);
	for (theta=0.0; theta<6.29; theta+=0.314)
	{
		y = (double) sin ((double) theta);
		x = (double) cos ((double) theta);

		x = radar.fcenter[0] + x * radar.fradius;
		y = radar.fcenter[1] + y * radar.fradius;

		glVertex2d (x, y);
	}
	glEnd();

	/* Draw yellow border */
	glColor3d (0.3, 0.3, 0.0);
	glBegin (GL_LINE_LOOP);
	for (theta=0.0; theta<6.29; theta+=0.314)
	{
		y = (double) sin ((double) theta);
		x = (double) cos ((double) theta);

		x = radar.fcenter[0] + x * radar.fradius;
		y = radar.fcenter[1] + y * radar.fradius;

		glVertex2d (x, y);
	}
	glEnd();

	/* Draw inner circle */
	glBegin (GL_LINE_LOOP);
	for (theta=0.0; theta<6.29; theta+=0.314)
	{
		y = (double) sin ((double) theta);
		x = (double) cos ((double) theta);

		x = radar.fcenter[0] + x * radar.fradius / 2.0;
		y = radar.fcenter[1] + y * radar.fradius / 2.0;

		glVertex2d (x, y);
	}
	glEnd();

	/* Draw connecting lines */
	glBegin (GL_LINES);
	x = radar.fcenter[0] + 0.7071 * radar.fradius;
	y = radar.fcenter[1] + 0.7071 * radar.fradius;
	glVertex2d (x, y);
	x = radar.fcenter[0] + 0.7071 * radar.fradius / 2.0;
	y = radar.fcenter[1] + 0.7071 * radar.fradius / 2.0;
	glVertex2d (x, y);

	glBegin (GL_LINES);
	x = radar.fcenter[0] - 0.7071 * radar.fradius;
	y = radar.fcenter[1] + 0.7071 * radar.fradius;
	glVertex2d (x, y);
	x = radar.fcenter[0] - 0.7071 * radar.fradius / 2.0;
	y = radar.fcenter[1] + 0.7071 * radar.fradius / 2.0;
	glVertex2d (x, y);

	glBegin (GL_LINES);
	x = radar.fcenter[0] - 0.7071 * radar.fradius;
	y = radar.fcenter[1] - 0.7071 * radar.fradius;
	glVertex2d (x, y);
	x = radar.fcenter[0] - 0.7071 * radar.fradius / 2.0;
	y = radar.fcenter[1] - 0.7071 * radar.fradius / 2.0;
	glVertex2d (x, y);

	glBegin (GL_LINES);
	x = radar.fcenter[0] + 0.7071 * radar.fradius;
	y = radar.fcenter[1] - 0.7071 * radar.fradius;
	glVertex2d (x, y);
	x = radar.fcenter[0] + 0.7071 * radar.fradius / 2.0;
	y = radar.fcenter[1] - 0.7071 * radar.fradius / 2.0;
	glVertex2d (x, y);

	glEnd();

	glDisable (GL_POLYGON_STIPPLE);

	glEndList();
}

RadarPlanets()
/*
 *  Draw the planets on the radar
 */
{
	int p;
	double x, y;

	for (p=0; p<NPLANETS; p++)
	{
		if (!planet[p].hidden)
		{
			if ((!planet[p].is_moon) || (planet[p].range2 < (1000.0 * 1000.0)))
			{
				/* Convert to radar coords */
				RadarCoords (planet[p].pos, &x, &y);

				/* Draw it */
				if ( (lock.type == LOCK_PLANET) &&
				     (lock.target == p) )
				{
					glPointSize (4.0);
				}
				else
				{
					glPointSize (3.0);
				}
				glBegin (GL_POINTS);
				if (planet[p].is_moon)
					glColor3d (0.0, 1.0, 1.0);
				else
					glColor3d (0.0, 0.0, 1.0);
				glVertex2d (x, y);
				glEnd();
			}
		}
	}
}

RadarTargets()
/*
 *  Draw the targets on the radar
 */
{
	double x, y;
	int i;

/*	glColor3d (0.9, 0.0, 0.0);	*/

	for (i=0; i<NTARGETS; i++)
	{
		/* Is this a network game and this target is us? */
/*		if (am_client && (i == client[clientme.client].target)) continue;	*/
		if (am_server && (i == client[server.client].target)) continue;

		if ( (target[i].age > 0.0) &&
		     (target[i].range2 < TARG_MAXRANGE2) &&
		     (!target[i].hidden) &&
		     (!target[i].invisible) )
		{
			/* Convert to radar coords */
			RadarCoords (target[i].pos, &x, &y);

			/* Select color */
			if (target[i].friendly)
				glColor3d (0.0, 0.9, 0.0);
			else
				glColor3d (0.9, 0.0, 0.0);

			/* Draw locked target bigger */
			if (i == lock.target)
			{
				glPointSize (3.0);
				glBegin (GL_POINTS);
				glVertex2d (x, y);
			}
			else
			{
				/* Just Draw it */
				glPointSize (2.0);
				glBegin (GL_POINTS);
				glVertex2d (x, y);
			}

			glEnd();
		}
	}
	glEnd();
}

RadarMissiles()
/*
 *  Draw the missiles on the radar
 */
{
	double x, y;
	int i;

	glPointSize (1.0);
	glBegin (GL_POINTS);

	/* Missiles are yellow */
	glColor3d (1.0, 1.0, 0.0);

	for (i=0; i<NMSLS; i++)
	{
		if (msl[i].age > 0.0)
		{
			/* Convert to radar coords */
			RadarCoords (msl[i].pos, &x, &y);

			/* Draw it */
			glVertex2d (x, y);
		}
	}
	glEnd();
}

RadarWaypoint()
/*
 *  Draw the current waypoint
 */
{
	double x, y;
	int w;

	if ((-1) == (w = player.waypoint)) return;

	glPointSize (2.0);
	glBegin (GL_POINTS);
	glColor3d (1.0, 1.0, 1.0);
	RadarCoords (waypoint[w].pos, &x, &y);
	glVertex2d (x, y);
	glEnd();
}

LockNearest()
/*
 *  Lock onto the nearest target
 */
{
	int t, ok;
	double d;

	d = -1.0;

	if (lock.type == LOCK_ENEMY)
	{
		for (t=0; t<NTARGETS; t++)
		{
			if (am_client || am_server)
			{
				/* Can lock distant targets in network games */
				ok = ( (target[t].age > 0.0) &&
					 (!target[t].hidden) &&
					 (!target[t].invisible) &&
					 (!target[t].friendly) );
			}
			else
			{
				ok = ( (target[t].age > 0.0) &&
					 (!target[t].hidden) &&
					 (!target[t].invisible) &&
					 (!target[t].friendly) &&
					 (target[t].range2 < TARG_MAXRANGE2) );
			}

			if (ok)
			{
				if ( (d < 0.0) || (target[t].range2 < d) )
				{
					d = target[t].range2;
					lock.target = t;
				}
			}
		}
	}
	else if (lock.type == LOCK_FRIENDLY)
	{
		for (t=0; t<NTARGETS; t++)
		{
			if ( (target[t].age > 0.0) &&
				 (!target[t].hidden) &&
				 (!target[t].invisible) &&
				 (target[t].range2 < TARG_MAXRANGE2) &&
			     (target[t].friendly) )
			{
				if ( (d < 0.0) || (target[t].range2 < d) )
				{
					d = target[t].range2;
					lock.target = t;
				}
			}
		}
	}
	else if (lock.type == LOCK_PLANET)
	{
		for (t=0; t<NPLANETS; t++)
		{
			if (!planet[t].hidden)
			{
				if ( (d < 0.0) || (planet[t].absrange2 < d) )
				{
					d = planet[t].absrange2;
					lock.target = t;
				}
			}
		}
	}
}

LockNext()
/*
 *  Lock onto next target
 */
{
	int t, tt, old, ok;

	old = lock.target;

	/* Check if we're not locked yet */
	if (lock.target == -1) lock.target = 0;

	if (lock.type == LOCK_ENEMY)
	{
		for (t=lock.target; t<lock.target+NTARGETS; t++)
		{
			tt = t % NTARGETS;

			if (am_client || am_server)
			{
				/* Can lock distant targets in network games */
				ok = ( (target[tt].age > 0.0) &&
					 (!target[tt].hidden) &&
					 (!target[tt].invisible) &&
					 (!target[tt].friendly) &&
					 (tt != old) );
			}
			else
			{
				ok = ( (target[tt].age > 0.0) &&
					 (!target[tt].hidden) &&
					 (!target[tt].invisible) &&
					 (!target[tt].friendly) &&
					 (target[tt].range2 < TARG_MAXRANGE2) &&
					 (tt != old) );
			}

			if (ok)
			{
				lock.target = tt;
				return;
			}
		}
	}
	else if (lock.type == LOCK_FRIENDLY)
	{
		for (t=lock.target; t<lock.target+NTARGETS; t++)
		{
			tt = t % NTARGETS;

			if ( (target[tt].age > 0.0) &&
				(!target[tt].hidden) &&
				(!target[tt].invisible) &&
				(target[tt].friendly) &&
				(target[tt].range2 < TARG_MAXRANGE2) &&
				(tt != old) )
			{
				lock.target = tt;
				return;
			}
		}
	}
	else if (lock.type == LOCK_PLANET)
	{
		do
		{
			lock.target = (lock.target + 1) % NPLANETS;
		} while ( (planet[lock.target].is_moon &&
			   (planet[lock.target].range2 > (5000.0*5000.0)) ) ||
		          planet[lock.target].hidden);
		return;
	}

	/* Nothing new to lock onto */
	lock.target = old;
}

LockPrev()
/*
 *  Lock onto previous target
 */
{
	int t, tt, old, ok;

	old = lock.target;

	/* Check if we're not locked yet */
	if (lock.target == -1) lock.target = 0;

	if (lock.type == LOCK_ENEMY)
	{
		for (t=lock.target+NTARGETS; t>lock.target; t--)
		{
			tt = t % NTARGETS;

			if (am_client || am_server)
			{
				ok = ( (target[tt].age > 0.0) &&
					 (!target[tt].hidden) &&
					 (!target[tt].invisible) &&
					 (!target[tt].friendly) &&
					 (tt != old) );
			}
			else
			{
				ok = ( (target[tt].age > 0.0) &&
					 (!target[tt].hidden) &&
					 (!target[tt].invisible) &&
					 (!target[tt].friendly) &&
					 (target[tt].range2 < TARG_MAXRANGE2) &&
					 (tt != old) );
			}

			if (ok)
			{
				lock.target = tt;
				return;
			}
		}
	}
	else if (lock.type == LOCK_FRIENDLY)
	{
		for (t=lock.target+NTARGETS; t>lock.target; t--)
		{
			tt = t % NTARGETS;

			if ( (target[tt].age > 0.0) &&
				(!target[tt].hidden) &&
				(!target[tt].invisible) &&
				(target[tt].friendly) &&
				(target[tt].range2 < TARG_MAXRANGE2) &&
				(tt != old) )
			{
				lock.target = tt;
				return;
			}
		}
	}
	else if (lock.type == LOCK_PLANET)
	{
		do
		{
			lock.target = (lock.target + NPLANETS - 1) % NPLANETS;
		} while ( (planet[lock.target].is_moon &&
			   (planet[lock.target].range2 > (5000.0*5000.0)) ) ||
		          planet[lock.target].hidden);
		return;
	}

	/* Nothing new to lock onto */
	lock.target = old;
}

CheckLock()
/*
 *  Sanity checks on the locked target
 */
{
	int l;

	l = lock.target;

	if (lock.type == LOCK_PLANET)
	{
		/* Can't lock hidden planets */
		if (planet[l].hidden) lock.target = -1;
	}

	if (lock.type == LOCK_FRIENDLY)
	{
		if (!target[l].friendly) lock.type = LOCK_ENEMY;
		if (target[l].hidden) lock.target = -1;
		if (target[l].invisible) lock.target = -1;
		if (target[l].age == 0.0) lock.target = -1;
		if (target[l].range2 > TARG_MAXRANGE2) lock.target = -1;
	}

	if (lock.type == LOCK_ENEMY)
	{
		if (target[l].friendly) lock.type = LOCK_FRIENDLY;
		if (target[l].hidden) lock.target = -1;
		if (target[l].invisible) lock.target = -1;
		if (target[l].age == 0.0) lock.target = -1;

		if (!am_client && !am_server)
		{
			if (target[l].range2 > TARG_MAXRANGE2) lock.target = -1;
		}
	}
}

static GLubyte lock_cursor[32] =
	{ 0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
	  0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
	  0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
	  0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xff, 0xff };

static GLubyte aim_cursor[8] =
	{ 0x81, 0x42, 0x24, 0x00, 0x00, 0x24, 0x42, 0x81 };

static GLubyte motion_cursor[8] =
	{ 0x18, 0x24, 0x42, 0x81, 0x81, 0x42, 0x24, 0x18 };

static GLubyte waypoint_cursor[8] =
	{ 0x3c, 0x00, 0x81, 0x81, 0x81, 0x81, 0x00, 0x3c };

DrawLock()
/*
 *  Draw the lock cursor
 */
{
	char v;
	double vtarg[3], v1[3];

	/* Don't bother if we're not locked */
	if (lock.target == -1) return;

	/* Planets are special */
	if (lock.type == LOCK_PLANET)
	{
		DrawPlanetLock();
		return;
	}

	/* Or if out of range */
	if (!am_client && !am_server)
	{
		if (target[lock.target].range2 > TARG_MAXRANGE2) return;
	}

	/* Select color */
	if (target[lock.target].friendly)
		glColor3d (0.0, 0.9, 0.0);
	else
		glColor3d (0.9, 0.0, 0.0);

	/* Set raster position, check if valid */
	Vsub (v1, target[lock.target].pos, player.pos);
	glRasterPos3dv (v1);
	glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &v);
	if (v)
	{
		/* Draw cursor */
		glBitmap (16, 16, 8.0, 8.0, 0.0, 0.0, lock_cursor);
	}

	/* Try to draw the aiming cursor */
	if (target[lock.target].range2 > weapon[player.weapon].range2) return;
	if (!target[lock.target].friendly)
	{
		if (!am_client && !am_server)
		{
			Vmul (v1, player.up, -0.01);
			Vadd (v1, v1, player.pos);
		}
		else
		{
			Vset (v1, player.pos);
		}
		if (Aim (vtarg, v1, player.vel,
				target[lock.target].pos, target[lock.target].vel,
				weapon[player.weapon].speed) )
		{
			glColor3d (1.0, 1.0, 0.0);
			Vsub (vtarg, vtarg, player.pos);
			glRasterPos3dv (vtarg);
			glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &v);
			if (v)
			{
				glBitmap (8, 8, 4.0, 4.0, 0.0, 0.0, aim_cursor);
			}
		}
	}
}

DrawPlanetLock()
{
	int p;
	char v;
	double v1[3];

	p = lock.target;

	if (planet[p].is_moon)
		glColor3d (0.0, 1.0, 1.0);
	else
		glColor3d (0.0, 0.0, 1.0);

	/* Set raster postition, check if valid */
	Vsub (v1, planet[p].pos, player.pos);
	glRasterPos3dv (v1);
	glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &v);
	if (v)
	{
		/* Draw cursor */
		glBitmap (16, 16, 8.0, 8.0, 0.0, 0.0, lock_cursor);
	}
}

DrawWaypoint()
{
	int w;
	char v;
	double v1[3];

	w = player.waypoint;
	if (w == (-1)) return;

	glColor3d (0.8, 0.8, 1.8);

	/* Set raster postition, check if valid */
	Vsub (v1, waypoint[w].pos, player.pos);
	glRasterPos3dv (v1);
	glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &v);
	if (v)
	{
		/* Draw cursor */
		glBitmap (8, 8, 3.0, 4.0, 0.0, 0.0, waypoint_cursor);
	}
}

DrawMotionCursors()
/*
 *  Show forward and reverse motion
 */
{
	double vn[3];
	char v;

	/* Not in arcade mode */
	if (player.flightmodel == FLIGHT_ARCADE) return;

	/* Not unless we're moving */
	if ( (0.0 == player.vel[0]) &&
	     (0.0 == player.vel[1]) &&
	     (0.0 == player.vel[2]) ) return;

	/* Forward cursor */
	Vset (vn, player.vel);
	Normalize (vn);

	glColor3d (0.0, 0.8, 0.0);
	glRasterPos3dv (vn);
	glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &v);
	if (v)
	{
		/* Draw cursor */
		glBitmap (8, 8, 4.0, 4.0, 0.0, 0.0, motion_cursor);
	}

	/* Reverse cursor */
	Vmul (vn, vn, -1.0);
	
	glColor3d (0.8, 0.0, 0.0);
	glRasterPos3dv (vn);
	glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &v);
	if (v)
	{
		/* Draw cursor */
		glBitmap (8, 8, 4.0, 4.0, 0.0, 0.0, motion_cursor);
	}
}

int Aim (vtarg, pos0, vel0, pos1, vel1, vel)
double vtarg[3], pos0[3], vel0[3], pos1[3], vel1[3], vel;
/*
 *  Something at position pos0, with velocity vector vel0,
 *  wants to shoot a missile with velocity "vel" at a target
 *  at position pos1, moving with vector vel1.
 *
 *  Coord of place to shoot will be in vtarg, unless there is
 *  no solution, in which case we return FALSE.
 */
{
	double va[3], vb[3], A, B, C, D, t, t1, t2, rootD;

	/* va is position of target wrt shooter */
	Vsub (va, pos1, pos0);

	/* vb is relative velocity of target */
	Vsub (vb, vel1, vel0);

	/* If there's a solution, then there exists some t such
	   that target postion (va) plus t times its velocity (vb)
	   is as far away from shooter (origin) as a missile can
	   travel in time t (t*vel).  If you work it out you get
	   a pretty simple quadratic equation. */

	/* Set up coefficients of quadratic equation */
	A = vb[0]*vb[0] + vb[1]*vb[1] + vb[2]*vb[2] - vel*vel;
	B = 2.0*va[0]*vb[0] + 2.0*va[1]*vb[1] + 2.0*va[2]*vb[2];
	C = va[0]*va[0] + va[1]*va[1] + va[2]*va[2];

	/* This is the kind of stuff you thought you'd never
	   use in real life, isn't it? */

	/* A == 0 is special case */
	if (A == 0.0)
	{
		/* A==0 && B==0 implies no solution */
		if (B == 0.0) return (0);

		/* Simple solution */
		t = (-C / B);

		/* Can't shoot missiles backwards */
		if (t < 0.0) return (0);
	}
	else
	{
		/* More complicated cases */
		/* Compute discriminant */
		D = B*B - 4.0*A*C;

		/* D < 0 implies no solution */
		if (D < 0.0) return (0);

		/* D == 0 implies one solution */
		if (D == 0.0)
		{
			t = -B / (2.0 * A);
			if (t <= 0.0) return (0);
		}
		else
		{
			/* Two solutions */
			rootD = sqrt (D);
			t1 = (-B + rootD) / (2.0 * A);
			t2 = (-B - rootD) / (2.0 * A);

			/* We need a non-negative solution */
			if ( (t1 < 0.0) && (t2 < 0.0) ) return (0);

			if (t1 < 0.0)
				t = t2;
			else if (t2 < 0.0)
				t = t1;
			else
			{
				/* Both non-negative, choose lesser
				   (closer to target) */
				if (t1 < t2)
					t = t1;
				else
					t = t2;
			}
		}
	}

	/* Whew!  We have a solution in t now. */
	Vmul (vtarg, vb, t);
	Vadd (vtarg, vtarg, va);
	Vadd (vtarg, vtarg, pos0);

	return (1);
}

ArcadeThrottle()
/*
 *  Draw the throttle for arcade mode
 */
{
	double y;

	glEnable (GL_POLYGON_STIPPLE);
	glPolygonStipple (throt_stipple);

	/* First draw unshaded background */
	glColor3f (0.0, 0.25, 0.0);
	glBegin (GL_QUAD_STRIP);
	glVertex2d (hud.throt_min[0], hud.throt_min[1]);
	glVertex2d (hud.throt_max[0], hud.throt_min[1]);
	glVertex2d (hud.throt_min[0], hud.throt_max[1]);
	glVertex2d (hud.throt_max[0], hud.throt_max[1]);
	glEnd();

	if (player.throttle > MAX_THROTTLE)
	{
		y = hud.throt_min[1] + (player.throttle / MAX_WARP_THROTTLE) *
		    (hud.throt_max[1] - hud.throt_min[1]);
	}
	else
	{
		y = hud.throt_min[1] + (player.throttle / MAX_THROTTLE) *
		    (hud.throt_max[1] - hud.throt_min[1]);
	}

	glColor3f (0.0, 0.75, 0.0);
	glBegin (GL_QUAD_STRIP);
	glVertex2d (hud.throt_min[0], hud.throt_min[1]);
	glVertex2d (hud.throt_max[0], hud.throt_min[1]);
	glVertex2d (hud.throt_min[0], y);
	glVertex2d (hud.throt_max[0], y);
	glEnd();

	glDisable (GL_POLYGON_STIPPLE);

	/* If travelling at warp speed, put a border around throttle */
	if (player.throttle > MAX_THROTTLE)
	{
		glColor3f (0.5, 0.5, 0.0);
		glBegin (GL_LINE_LOOP);
		glVertex2d (hud.throt_min[0], hud.throt_min[1]);
		glVertex2d (hud.throt_max[0], hud.throt_min[1]);
		glVertex2d (hud.throt_max[0], hud.throt_max[1]);
		glVertex2d (hud.throt_min[0], hud.throt_max[1]);
		glEnd();
	}
}

Throttle()
/*
 *  Draw the throttle
 */
{
	double y;

	/* Arcade mode is special */
	if (player.flightmodel == FLIGHT_ARCADE)
	{
		ArcadeThrottle();
		return;
	}

	glEnable (GL_POLYGON_STIPPLE);
	glPolygonStipple (throt_stipple);

	/* First draw unshaded background */
	glColor3f (0.25, 0.0, 0.0);
	glBegin (GL_QUAD_STRIP);
	glVertex2d (hud.throt_min[0], hud.throt_min[1]);
	glVertex2d (hud.throt_max[0], hud.throt_min[1]);
	glVertex2d (hud.throt_min[0], hud.throt_mid[1]);
	glVertex2d (hud.throt_max[0], hud.throt_mid[1]);
	glEnd();
	glColor3f (0.0, 0.25, 0.0);
	glBegin (GL_QUAD_STRIP);
	glVertex2d (hud.throt_min[0], hud.throt_mid[1]);
	glVertex2d (hud.throt_max[0], hud.throt_mid[1]);
	glVertex2d (hud.throt_min[0], hud.throt_max[1]);
	glVertex2d (hud.throt_max[0], hud.throt_max[1]);
	glEnd();

	/* Now show the throttle */
	if (player.move_forward > 0.0)
	{
		glColor3f (0.0, 0.75, 0.0);
		y = hud.throt_mid[1] + player.move_forward *
			(hud.throt_max[1] - hud.throt_mid[1]);
		glBegin (GL_QUAD_STRIP);
		glVertex2d (hud.throt_min[0], hud.throt_mid[1]);
		glVertex2d (hud.throt_max[0], hud.throt_mid[1]);
		glVertex2d (hud.throt_min[0], y);
		glVertex2d (hud.throt_max[0], y);
		glEnd();
	}
	else if (player.move_backward > 0.0)
	{
		glColor3f (0.75, 0.0, 0.0);
		y = hud.throt_mid[1] - player.move_backward *
			(hud.throt_mid[1] - hud.throt_min[1]);
		glBegin (GL_QUAD_STRIP);
		glVertex2d (hud.throt_min[0], hud.throt_mid[1]);
		glVertex2d (hud.throt_max[0], hud.throt_mid[1]);
		glVertex2d (hud.throt_min[0], y);
		glVertex2d (hud.throt_max[0], y);
		glEnd();
	}

	glDisable (GL_POLYGON_STIPPLE);

	/* If travelling at warp speed, put a border around throttle */
	if (warpspeed)
	{
		glColor3f (0.5, 0.5, 0.0);
		glBegin (GL_LINE_LOOP);
		glVertex2d (hud.throt_min[0], hud.throt_min[1]);
		glVertex2d (hud.throt_max[0], hud.throt_min[1]);
		glVertex2d (hud.throt_max[0], hud.throt_max[1]);
		glVertex2d (hud.throt_min[0], hud.throt_max[1]);
		glEnd();
	}
}

TargetStuff()
/*
 *  Show stuff about the locked target
 */
{
	int t;
	double r;
	char buf[32];

	/* Planet lock is special */
	if (lock.type == LOCK_PLANET)
	{
		PlanetStuff();
		return;
	}

	if (-1 != (t = lock.target))
	{
		/* Bail if too far away */
		if (!am_client && !am_server)
		{
			if (target[lock.target].range2 > TARG_MAXRANGE2) return;
		}

		/* Target name */
		if (target[t].friendly)
			glColor3f (0.0, 0.8, 0.0);
		else
			glColor3f (0.8, 0.0, 0.0);

		glRasterPos2dv (hud.targ_name);

		/* Target range */
		r = sqrt (target[t].range2) * KM_TO_UNITS1;
		sprintf (buf, "%s:%2.0lf", target[t].name, r);
		Print (GLUT_BITMAP_HELVETICA_10, buf);

		/* Target shields */
		TargetShields (t);
	}
}

WaypointStuff()
{
	int w;
	double r, v[3];
	char buf[32];

	if (player.waypoint == (-1)) return;

	w = player.waypoint;
	glColor3f (0.8, 0.8, 0.8);
	glRasterPos2dv (hud.waypoint);
	Vsub (v, waypoint[w].pos, player.pos);
	r = Mag (v) * KM_TO_UNITS1;
	sprintf (buf, "%d:%2.0lf", w+1, r);
	Print (GLUT_BITMAP_HELVETICA_10, buf);
}

PlayerVel()
{
	double r;
	char buf[64];

	/* Player velocity */
	r = Mag (player.vel) * KM_TO_UNITS1;

	if (r == 0.0)
		glColor3f (0.0, 0.0, 0.8);
	else if (Dotp (player.view, player.vel) >= 0.0)
		glColor3f (0.0, 0.8, 0.0);
	else
		glColor3f (0.8, 0.0, 0.0);

	sprintf (buf, "%2.0lf", r);
	hud.vel[0] = hud.throt_min[0] - 5 - 
		glutBitmapLength (GLUT_BITMAP_HELVETICA_10, buf);
	glRasterPos2dv (hud.vel);
	Print (GLUT_BITMAP_HELVETICA_10, buf);
}

PlanetStuff()
{
	int p;
	double r;
	char buf[64];

	if (-1 == (p = lock.target)) return;

	if (planet[p].is_moon)
		glColor3d (0.0, 0.8, 0.8);
	else
		glColor3d (0.0, 0.0, 0.8);

	glRasterPos2dv (hud.targ_name);

	/* Planet range */
	r = (sqrt(planet[p].absrange2) - planet[p].radius) * KM_TO_UNITS1;
	sprintf (buf, "%s:%2.0lf", planet[p].name, r);

	Print (GLUT_BITMAP_HELVETICA_10, buf);
}

Shields()
/*
 *  Draw the shield status
 */
{
	float color[3];
	double y;

	glEnable (GL_POLYGON_STIPPLE);
	glPolygonStipple (throt_stipple);

	if (player.shields > player.maxshields*0.66)
	{
		color[0] = color[2] = 0.0;
		color[1] = 0.25;
	}
	else if (player.shields > player.maxshields*0.33)
	{
		color[0] = color[1] = 0.25;
		color[2] = 0.0;
	}
	else
	{
		color[0] = 0.25;
		color[1] = color[2] = 0.0;
	}

	y = hud.shields_min[1] +
		(player.shields / player.maxshields) *
		(hud.shields_max[1] - hud.shields_min[1]);

	/* First draw unshaded background */
	if (player.shields < player.maxshields)
	{
		glColor3fv (color);
		glBegin (GL_QUAD_STRIP);
		glVertex2d (hud.shields_min[0], y);
		glVertex2d (hud.shields_max[0], y);
		glVertex2d (hud.shields_min[0], hud.shields_max[1]);
		glVertex2d (hud.shields_max[0], hud.shields_max[1]);
		glEnd();
	}

	/* Now show the shields */
	color[0] *= 3.0;
	color[1] *= 3.0;

	glColor3fv (color);
	glBegin (GL_QUAD_STRIP);
	glVertex2d (hud.shields_min[0], hud.shields_min[1]);
	glVertex2d (hud.shields_max[0], hud.shields_min[1]);
	glVertex2d (hud.shields_min[0], y);
	glVertex2d (hud.shields_max[0], y);
	glEnd();

	glDisable (GL_POLYGON_STIPPLE);
}

ShowWeapon()
/*
 *  Show the current weapon
 */
{
	/* Set color depending if it's ready to fire */
	if (player.msl_idle >= weapon[player.weapon].idle)
	{
		glColor3f (0.8, 0.0, 0.8);
	}
	else
	{
		glColor3f (0.5, 0.5, 0.5);
	}
	glRasterPos2dv (hud.weapon);
	Print (GLUT_BITMAP_HELVETICA_10, weapon[player.weapon].name);
}

TargetShields (t)
int t;
/*
 *  Draw the target's shield status
 */
{
	float color[3];
	double y;

	glEnable (GL_POLYGON_STIPPLE);
	glPolygonStipple (throt_stipple);

	if (target[t].shields > target[t].maxshields*0.66)
	{
		color[0] = color[2] = 0.0;
		color[1] = 0.25;
	}
	else if (target[t].shields > target[t].maxshields*0.33)
	{
		color[0] = color[1] = 0.25;
		color[2] = 0.0;
	}
	else
	{
		color[0] = 0.25;
		color[1] = color[2] = 0.0;
	}

	y = hud.targshields_min[1] +
		(target[t].shields / target[t].maxshields) *
		(hud.targshields_max[1] - hud.targshields_min[1]);

	/* First draw unshaded background */
	if (target[t].shields < target[t].maxshields)
	{
		glColor3fv (color);
		glBegin (GL_QUAD_STRIP);
		glVertex2d (hud.targshields_min[0], y);
		glVertex2d (hud.targshields_max[0], y);
		glVertex2d (hud.targshields_min[0], hud.targshields_max[1]);
		glVertex2d (hud.targshields_max[0], hud.targshields_max[1]);
		glEnd();
	}

	/* Now show the shields */
	color[0] *= 3.0;
	color[1] *= 3.0;

	glColor3fv (color);
	glBegin (GL_QUAD_STRIP);
	glVertex2d (hud.targshields_min[0], hud.targshields_min[1]);
	glVertex2d (hud.targshields_max[0], hud.targshields_min[1]);
	glVertex2d (hud.targshields_min[0], y);
	glVertex2d (hud.targshields_max[0], y);
	glEnd();

	glDisable (GL_POLYGON_STIPPLE);
}

