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

InitTargets()
/*
 *  Set up targets
 */
{
	int t;

	for (t=0; t<NTARGETS; t++)
	{
		target[t].age = target[t].msl_idle = 0.0;
		target[t].vel[0] = target[t].vel[1] = target[t].vel[2] = 0.0;

		target[t].hidden = 0;
		target[t].friendly = 0;
		target[t].invisible = 0;

		target[t].move_forward = target[t].move_backward =
			target[t].move_up = target[t].move_down =
			target[t].move_pitchleft = target[t].move_pitchright =
			target[t].move_left = target[t].move_right = 0.0;

		target[t].weapon = NPLAYER_WEAPONS;
		target[t].shields = 100.0;
		target[t].maxshields = 100.0;

		strcpy (target[t].name, "Sparky");
	}
}

int FindTarget()
/*
 *  Find an unsed target
 */
{
	int i, oldest;
	double maxage;

	for (i=0; i<NTARGETS; i++)
	{
		if (target[i].age == 0.0) return (i);

		if (i == 0)
		{
			oldest = i;
			maxage = target[i].age;
		}
		else
		{
			if (target[i].age > maxage)
			{
				maxage = target[i].age;
				oldest = i;
			}
		}
	}

	return (oldest);
}

int FindTargetByName (name)
char *name;
{
	int t;

	for (t=0; t<NTARGETS; t++)
	{
		if ( (target[t].age > 0.0) &&
		     (!strcasecmp (name, target[t].name)) )
		{
			return (t);
		}
	}

	return (-1);
}

PlaceTarget (x, y, z)
double x, y, z;
/*
 *  Put a brand new target somewhere
 */
{
	int t;

	t = FindTarget();

	target[t].pos[0] = x;
	target[t].pos[1] = y;
	target[t].pos[2] = z;

	target[t].view[0] = 1.0;
	target[t].view[1] = target[t].view[2] = 0.0;

	target[t].up[0] = 0.0;
	target[t].up[1] = 0.0;
	target[t].up[2] = 1.0;

	Crossp (target[t].right, target[t].up, target[t].view);

	if (deltaT == 0.0)
	{
		target[t].age = 0.1;
	}
	else
	{
		target[t].age = deltaT;
	}

	target[t].msl_idle = 0.0;

	target[t].vel[0] = target[t].vel[1] = target[t].vel[2] = 0.0;

	target[t].move_forward = target[t].move_backward =
		target[t].move_up = target[t].move_down =
		target[t].move_pitchleft = target[t].move_pitchright =
		target[t].move_left = target[t].move_right = 0.0;

	strcpy (target[t].name, "Target");
	target[t].list = model[0].list;
}

PlaceRandomTarget()
/*
 *  Put a target somewhere reasonable, randomly
 */
{
	double x, y, z, th;

	th = 2.0 * rnd (6.28);

	x = planet[3].pos[0] + 2.0 * planet[3].radius * sin (th);
	y = planet[3].pos[1] + 2.0 * planet[3].radius * cos (th);
	z = planet[3].pos[2];

	PlaceTarget (x, y, z);
}

DestroyTarget (t)
int t;
/*
 *  Destroy specified target
 */
{
	int e;

	target[t].age = 0.0;

	player.score += target[t].score;

	/* If this was the locked target, unset lock */
	CheckLock();

	/* Check to see if destroying this target triggers an event */
	for (e=0; e<NEVENTS; e++)
	{
		if (event[e].pending &&
		    event[e].enabled &&
			(event[e].trigger == EVENT_DESTROY) &&
		    (!strcasecmp (event[e].cvalue, target[t].name)) )
		{
			EventAction (e);
		}
	}
}

static float lmodel_ambient[] = { 0.1, 0.1, 0.1, 1.0 };

DrawTargets ()
/*
 *  Draw all the targets
 */
{
	int i;

	glShadeModel (GL_FLAT);

	/* Need some ambient light for targets */
	glLightfv (GL_LIGHT0, GL_AMBIENT, lmodel_ambient);

	for (i=0; i<NTARGETS; i++)
	{
		if (target[i].age > 0.0) DrawTarget (i);
	}

	glShadeModel (GL_SMOOTH);
}

DrawTarget (i)
int i;
/*
 *  Draw this target
 */
{
	double v[3];

	/* Bump this target's age */
	target[i].age += deltaT;

	/* Forget it if target is hidden or invisible */
	if (target[i].hidden || target[i].invisible) return;

	/* Don't bother if target is too far away */
	if (target[i].range2 > TARG_MAXRANGE2) return;

	/* Bail if couldn't load model */
	if (target[i].model == (-1)) return;

	glPushMatrix();

	Vsub (v, target[i].pos, player.pos);
	glTranslated (v[0], v[1], v[2]);

	/* Show up vector */
/*	glDisable (GL_LIGHTING);
	glColor3f (0.0, 1.0, 0.0);
	glBegin (GL_LINES);
	glVertex3f (0.0, 0.0, 0.0);
	glVertex3dv (target[i].up);
	glEnd();
	glEnable (GL_LIGHTING);	*/

	/* Have target look in proper direction */
	LookAt (target[i].view, target[i].up);

	/* Draw it */
	glCallList (target[i].list);

	/* Draw bounding box */
/*	if (i == lock.target)
	{
		m = target[i].model;

		glDisable (GL_LIGHTING);
		glColor3f (1.0, 1.0, 0.0);
		glScaled (model[m].hibound[0] - model[m].lobound[0],
			      model[m].hibound[1] - model[m].lobound[1],
				  model[m].hibound[2] - model[m].lobound[2]);
		glutWireCube (1.0);
		glEnable (GL_LIGHTING);
	}
*/
	glPopMatrix();
}

MoveTargets()
/*
 *  Move all the targets
 */
{
	int t;
	double v[3];

	for (t=0; t<NTARGETS; t++)
	{
		if (target[t].age > 0.0)
		{
			/* Determine distance from player */
			Vsub (v, target[t].pos, player.pos);
			target[t].range2 = Mag2 (v);

			if (!target[t].hidden)
			{
				target[t].age += deltaT;
				target[t].msl_idle += deltaT;

				/* Give this target a chance to think */
				if (target[t].range2 < TARG_MAXRANGE2) ThinkTarget (t);

				/* Now move it */
				MoveTarget (t);
			}
		}
	}
}

MoveTarget (t)
int t;
/*
 *  Move this target
 */
{
	double deltav[3], v[3], theta;
	int p;

	/* Is this a network game and this target is us? */
/*
	if (am_client && (t == client[clientme.client].target)) return;
	if (am_server && (t == client[server.client].target)) return;
*/
	/* Compute angle to move, given time change */
	theta = THETA * deltaT;

	/* Deltav will be change in velocity components due to thrust */
	deltav[0] = deltav[1] = deltav[2] = 0.0;

	if (target[t].move_left > 0.0)
	{		
		RotateAbout (v, target[t].view, target[t].up, -theta*target[t].move_left);
		Vset (target[t].view, v);
		Normalize (target[t].view);
		Crossp (target[t].right, target[t].up, target[t].view);
		Normalize (target[t].right);
	}

	if (target[t].move_right > 0.0)
	{
		RotateAbout (v, target[t].view, target[t].up, theta*target[t].move_right);
		Vset (target[t].view, v);
		Normalize (target[t].view);
		Crossp (target[t].right, target[t].up, target[t].view);
		Normalize (target[t].right);
	}

	if (target[t].move_up > 0.0)
	{
		RotateAbout (v, target[t].view, target[t].right, theta*target[t].move_up);
		Vset (target[t].view, v);
		Normalize (target[t].view);
		Crossp (target[t].up, target[t].view, target[t].right);
		Normalize (target[t].up);
	}

	if (target[t].move_down > 0.0)
	{
		RotateAbout (v, target[t].view, target[t].right, -theta*target[t].move_down);
		Vset (target[t].view, v);
		Normalize (target[t].view);
		Crossp (target[t].up, target[t].view, target[t].right);
		Normalize (target[t].up);
	}

	if (target[t].move_pitchright > 0.0)
	{
		RotateAbout (v, target[t].up, target[t].view, -theta*target[t].move_pitchright);
		Vset (target[t].up, v);
		Normalize (target[t].up);
		Crossp (target[t].right, target[t].up, target[t].view);
		Normalize (target[t].right);
	}

	if (target[t].move_pitchleft > 0.0)
	{
		RotateAbout (v, target[t].up, target[t].view, theta*target[t].move_pitchleft);
		Vset (target[t].up, v);
		Normalize (target[t].up);
		Crossp (target[t].right, target[t].up, target[t].view);
		Normalize (target[t].right);
	}

	if (target[t].move_forward > 0.0)
	{
		Vmul (v, target[t].view, target[t].move_forward*DELTAV*deltaT);
		Vadd (deltav, deltav, v);
	}

	if (target[t].move_backward > 0.0)
	{
		Vmul (v, target[t].view, -target[t].move_backward*DELTAV*deltaT);
		Vadd (deltav, deltav, v);
	}

	/* Compute change to velocity */
	Vadd (target[t].vel, target[t].vel, deltav);

	/* Compute gravity's contribution */
	if (gravity && (am_client || am_server))
	{
		Gravity (deltav, target[t].pos);
		Vadd (target[t].vel, target[t].vel, deltav);
	}

	/* Finaly, move target */
	Vmul (v, target[t].vel, deltaT);
	Vadd (target[t].pos, target[t].pos, v);

	target[t].move_forward = target[t].move_backward = 0.0;

	if ( (!am_client) && (!am_server) )
	{
		target[t].move_up = target[t].move_down =
		target[t].move_pitchleft = target[t].move_pitchright =
		target[t].move_left = target[t].move_right = 0.0;
	}

	/* See if target[t] cratered on a planet */
	if ((!am_client) && ((-1) != (p = InsidePlanet (target[t].pos))))
	{
		/* Make an explosion */
		Vmul (v, target[t].pos, 1.05);
		Boom (v, 1.0);
		if (am_server)
		{
			NetTargetCratered (t, p);
		}
		else
		{
			DestroyTarget (t);
		}
	}

	/* Regenerate target shields */
	target[t].shields += target[t].shieldregen * deltaT;
	if (target[t].shields > target[t].maxshields)
		target[t].shields = target[t].maxshields;
}

TargetFiresMissile (t)
int t;
/*
 *  Target t tries to fire missile at player
 */
{
	if (target[t].msl_idle > weapon[target[t].weapon].idle)
	{
		/* Fire away! */
		target[t].msl_idle = 0.0;
		FireMissile (target[t].pos, target[t].vel, target[t].view,
			target[t].friendly, target[t].weapon, t);
	}
}

ShowTargetNames()
{
	int t;
	char v, buf[128];
	double v1[3];

	for (t=0; t<NTARGETS; t++)
	{
		if ( (target[t].age > 0.0) &&
		     (target[t].range2 < TARG_MAXRANGE2) &&
		     (!target[t].hidden) &&
		     (!target[t].invisible) )
		{
			if (target[t].friendly)
				glColor3d (0.0, 0.8, 0.0);
			else
				glColor3d (0.8, 0.0, 0.0);

			Vsub (v1, target[t].pos, player.pos);
			glRasterPos3dv (v1);
			glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &v);
			if (v)
			{
				if (target[t].range2 <= weapon[player.weapon].range2)
				{
					sprintf (buf, "%s:%.0lf", target[t].name, 
						KM_TO_UNITS1*sqrt(target[t].range2));
				}
				else
				{
					strcpy (buf, target[t].name);
				}
				Print (GLUT_BITMAP_HELVETICA_10, buf);
			}
		}
	}
}

int HitTarget (m)
int m;
/*
 *  See if missile m hit any target.  -1 if not.
 */
{
	int t, mdl, valid;
	double v1[3], d;

	/* Loop through targets */
	for (t=0; t<NTARGETS; t++)
	{
		/* See if we should even check this target */
		if (!am_client && !am_server)
		{
			valid = (target[t].age > 0.0) &&
			        (!target[t].hidden) &&
			        (msl[m].friendly != target[t].friendly);
		}
		else
		{
			valid = (target[t].age > 0.0) &&
			        (!target[t].hidden) &&
			        (t != msl[m].owner);
		}

		if (valid)
		{
			/* Get coords relative to target */
			Vsub (v1, msl[m].pos, target[t].pos);

			/* Get target's model */
			mdl = target[t].model;

			/* Check each axis by projecting v1 and checking bounding box */
			d = Dotp (v1, target[t].view);
			if ( (d < model[mdl].lobound[0]) ||
			     (d > model[mdl].hibound[0]) ) continue;

			d = Dotp (v1, target[t].right);
			if ( (d < model[mdl].lobound[1]) ||
			     (d > model[mdl].hibound[1]) ) continue;

			d = Dotp (v1, target[t].up);
			if ( (d < model[mdl].lobound[2]) ||
			     (d > model[mdl].hibound[2]) ) continue;

			/* If we get here, it's a hit! */
			return (t);
		}
	}

	/* Nope */
	return (-1);
}
