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

InitMissiles()
/*
 *  Set up missile structures
 */
{
	int i;

	/* Mark all missiles as unused */
	for (i=0; i<NMSLS; i++)
	{
		msl[i].age = 0.0;
	}
}

int FindMissile()
/*
 *  Find unused missile
 */
{
	int i, oldest;
	double maxage;

	/* Look for free missile, or oldest */
	for (i=0; i<NMSLS; i++)
	{
		if (msl[i].age == 0.0) return (i);

		if (i == 0)
		{
			maxage = msl[i].age;
			oldest = i;
		}
		else
		{
			if (msl[i].age > maxage)
			{
				maxage = msl[i].age;
				oldest = i;
			}
		}
	}

	/* None free, return oldest */
	return (oldest);
}

FireMissile (pos, vel, dir, friendly, wep, owner)
double pos[3], vel[3], dir[3];
int friendly, wep, owner;
/*
 *  Fire missile at the given location and velocity, in
 *  the specified direction
 */
{
	int m;
	double v[3];

	/* Find a free missile */
	m = FindMissile();

	/* Set it up */
	msl[m].age = deltaT;
	Vset (msl[m].pos, pos);

	if (friendly && !am_client && !am_server)
	{
		Vmul (v, player.up, -0.005);
		Vadd (msl[m].pos, msl[m].pos, v);
	}

	Vmul (msl[m].vel, dir, weapon[wep].speed);
	Vadd (msl[m].vel, msl[m].vel, vel);

	msl[m].friendly = friendly;
	msl[m].weapon = wep;
	msl[m].owner = owner;

	/* Play the sound */
/*	if (sound) PlaySound ("phaser.wav", NULL, SND_ASYNC | SND_FILENAME);	*/
	if (sound) PlayAudio (SOUND_FIRE);
}

MoveMissiles()
/*
 *  Move all the missiles
 */
{
	int m, t, p;
	double v[3], deltav[3];

	/* Loop through missiles */
	for (m=0; m<NMSLS; m++)
	{
		/* If missile in use */
		if (msl[m].age > 0.0)
		{
			/* Bump age */
			msl[m].age += deltaT;

			/* See if expired */
			if (msl[m].age > weapon[msl[m].weapon].expire)
			{
				DestroyMissile (m);
				return;
			}

			/* See if missile has hit planet */
			if ((-1) != (p = InsidePlanet (msl[m].pos)))
			{
				/* Missile has hit planet */

				/* Move point of impact just above planet surface */
				Vsub (v, msl[m].pos, planet[p].pos);
				Normalize (v);
				Vmul (v, v, planet[p].radius*1.05);
				Vadd (v, v, planet[p].pos);
				Boom (v, weapon[msl[m].weapon].yield/100.0);
				DestroyMissile (m);
				return;
			}

			/* See if missile hit a target */
			if ((-1) != (t = HitTarget (m)))
			{
				/* Missle hit target */
				MissileHitTarget (m, t);
				return;
			}

			/* See if missile hit player */
			if (!msl[m].friendly && !am_client &&
			    (state != STATE_DEAD1) && (state != STATE_DEAD2) )
			{
				if (TARGDIST2 > Dist2 (msl[m].pos[0], msl[m].pos[1], msl[m].pos[2],
									   player.pos[0], player.pos[1], player.pos[2]))
				{
					/* Missile hit player */
					MissileHitPlayer (m);
					return;
				}
			}

			/* Else update its position */
			if (gravity)
			{
				Gravity (deltav, msl[m].pos);
				Vadd (msl[m].vel, msl[m].vel, deltav);
			}
			Vmul (v, msl[m].vel, deltaT);
			Vadd (msl[m].pos, msl[m].pos, v);
		}
	}
}

DestroyMissile (m)
int m;
/*
 *  Missile has expired 
 */
{
	msl[m].age = 0.0;
}

DrawMissiles ()
/*
 *  Draw all the missiles
 */
{
	int m;

	for (m=0; m<NMSLS; m++)
	{
		if (msl[m].age > 0.0) DrawMissile (m);
	}
}

DrawMissile (m)
int m;
/*
 *  Draw this missile
 */
{
	glPushMatrix();
	glDisable (GL_LIGHTING);
	glDisable (GL_CULL_FACE);

	switch (weapon[msl[m].weapon].renderer)
	{
	case 0:	DrawMissile0 (m);
			break;

	case 1: DrawMissile1 (m);
			break;

	case 2: DrawMissile2 (m);
			break;

	case 3: DrawMissile3 (m);
			break;

	case 4:	DrawMissile4 (m);
			break;
	}

	glEnable (GL_CULL_FACE);
	glEnable (GL_LIGHTING);
	glPopMatrix();
}

DrawMissile0 (m)
int m;
{
	double r, s, v[3];

	/* Figure how much to spin missile */
	r = absT - ((double)(int)absT);
	s = 360.0 * r;

	glColor3fv (weapon[msl[m].weapon].color);

	Vsub (v, msl[m].pos, player.pos);
	glTranslated (v[0], v[1], v[2]);
	glRotated (s, 0.3, 1.0, 0.6);

	glBegin (GL_LINES);
	glVertex3d (-0.01, 0.0, 0.0);
	glVertex3d ( 0.01, 0.0, 0.0);
	glVertex3d (0.0, -0.01, 0.0);
	glVertex3d (0.0,  0.01, 0.0);
	glVertex3d (0.0, 0.0, -0.01);
	glVertex3d (0.0, 0.0,  0.01);
	glEnd();
}

DrawMissile1 (m)
int m;
{
	double r, s, v[3];

	/* Figure how much to spin missile */
	r = absT - ((double)(int)absT);
	s = 360.0 * r;

	glColor3fv (weapon[msl[m].weapon].color);

	Vsub (v, msl[m].pos, player.pos);
	glTranslated (v[0], v[1], v[2]);
	glRotated (s, 0.3, 1.0, 0.6);

	glBegin (GL_LINES);
	glVertex3d (-0.01, 0.0, 0.0);
	glVertex3d ( 0.01, 0.0, 0.0);
	glVertex3d (0.0, -0.01, 0.0);
	glVertex3d (0.0,  0.01, 0.0);
	glVertex3d (0.0, 0.0, -0.01);
	glVertex3d (0.0, 0.0,  0.01);
	glEnd();
}

DrawMissile2 (m)
int m;
{
	double v1[3], v2[3], v3[3], v4[4], vel[3], v[3];

	glColor3fv (weapon[msl[m].weapon].color);

	Vsub (v, msl[m].pos, player.pos);
	glTranslated (v[0], v[1], v[2]);

	Vmul (vel, msl[m].vel, 0.1);

	glBegin (GL_QUADS);

	v1[0] =  0.01; v1[1] = 0.0; v1[2] = 0.0;
	v2[0] = -0.01; v2[1] = 0.0; v2[2] = 0.0;
	Vsub (v3, v2, vel);
 	Vsub (v4, v1, vel);
	glVertex3dv (v1);
	glVertex3dv (v2);
	glVertex3dv (v3);
	glVertex3dv (v4);

	v1[0] = 0.0; v1[1] =  0.01; v1[2] = 0.0;
	v2[0] = 0.0; v2[1] = -0.01; v2[2] = 0.0;
	Vsub (v3, v2, vel);
	Vsub (v4, v1, vel);
	glVertex3dv (v1);
	glVertex3dv (v2);
	glVertex3dv (v3);
	glVertex3dv (v4);

	v1[0] = 0.0; v1[1] = 0.0; v1[2] =  0.01;
	v2[0] = 0.0; v2[1] = 0.0; v2[2] = -0.01;
	Vsub (v3, v2, vel);
	Vsub (v4, v1, vel);
	glVertex3dv (v1);
	glVertex3dv (v2);
	glVertex3dv (v3);
	glVertex3dv (v4);

	glEnd();
}

DrawMissile3 (m)
int m;
{
	double r, s, v[3];

	/* Figure how much to spin missile */
	r = absT - ((double)(int)absT);
	s = 360.0 * r;

	glColor3fv (weapon[msl[m].weapon].color);

	Vsub (v, msl[m].pos, player.pos);
	glTranslated (v[0], v[1], v[2]);
	glRotated (s, 0.3, 1.0, 0.6);
	glScaled (0.01, 0.01, 0.01);

	glutSolidTetrahedron ();
}

DrawMissile4 (m)
int m;
{
	int i;
	double v[3], u[3];

	glColor3fv (weapon[msl[m].weapon].color);

	Vsub (u, msl[m].pos, player.pos);
	glTranslated (u[0], u[1], u[2]);

	glBegin (GL_LINE_LOOP);
	for (i=0; i<6; i++)
	{
		v[0] = rnd(0.04) - 0.01;
		v[1] = rnd(0.04) - 0.01;
		v[2] = rnd(0.04) - 0.01;
		glVertex3dv (v);
	}	
	glEnd();
}

MissileHitPlayer (m)
int m;
/*
 *  Missile m hit player.  Ouch!
 */
{
	/* Flash screen red */
	palette_flash = 2;

	/* Damage shields */
	player.shields -= weapon[msl[m].weapon].yield;

	/* Get rid of missile */
	DestroyMissile (m);

	/* See if player died */
	if (vulnerable && (player.shields < 0.0) && !am_client)
	{
		if (am_server)
		{
			/* Tell other clients */
			NetDestroyClient (server.client, FindClientByTarget (msl[m].owner));
			NetPlayerDies();
		}
		else
		{
			/* Single player games, reset */
			InitPlayer();
			ReadMission (mission.fn);
			Mprint ("You were killed!  Restarting mission.");
		}
	}

	if (player.shields < 0.0) player.shields = 100.0;
}

MissileHitTarget (m, t)
int m, t;
/*
 *  Missile m hit target t.  Go get 'em!
 */
{
	int e;

	Boom (msl[m].pos, weapon[msl[m].weapon].yield/100.0);

	target[t].shields -= weapon[msl[m].weapon].yield;
	if (target[t].shields < 0.0) target[t].shields = 0.0;

	/* Announce to network */
	if (am_server) NetHitTarget (t, m);

	/* See if we set off an event */
	for (e=0; e<NEVENTS; e++)
	{
		if (event[e].pending &&
		    event[e].enabled &&
		    (event[e].trigger == EVENT_SHIELDS) &&
		    (!strcasecmp (event[e].cvalue, target[t].name)) &&
		    (target[t].shields <= event[e].fvalue) )
		{
			EventAction (e);
		}
	}

	/* See if target is destroyed */
	if (!am_client && target[t].shields <= 0.0)
	{
		/* Announce to network */
		NetDestroyTarget (t, m);

		/* Kablooie! */
		Boom (target[t].pos, 1.0);
		if (!am_server) DestroyTarget (t);
		if (am_server) target[t].shields = 100.0;
	}

	DestroyMissile (m);
}
