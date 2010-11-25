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

InitPlayer()
/*
 *  Set up initial player position, etc.
 */
{
	/* Initial position */
	player.pos[0] = planet[3].pos[0] - 2.0*planet[3].radius;
	player.pos[1] = planet[3].pos[1];
	player.pos[2] = planet[3].pos[2];

	/* Up vector */
	player.up[0] = 0.0;
	player.up[1]= 0.0;
	player.up[2] = 1.0;

	/* Viewing vector */
	player.view[0] = 1.0;
	player.view[1] = 0.0;
	player.view[2] = 0.0;

	/* Right-hand vector */
	Crossp (player.right, player.up, player.view);
	Normalize (player.right);

	/* Velocity components */
	player.vel[0] = player.vel[1] = player.vel[2] = 0.0;
	player.throttle = 0.0;

	/* Shields */
	player.shields = player.maxshields = 100.0;

	/* Weapon */
	player.weapon = 0;
	player.msl_idle = 0.0;
}

void MovePlayer()
{
	double v[3], theta, deltav[3];
	int p, t, was_still;
	char buf[128];

	/* Aim player if view is locked */
	if (player.viewlock) ViewLock();

	/* Compute angle to move, given time change */
	theta = THETA * deltaT;

	/* Deltav will be change in velocity components due to thrust */
	deltav[0] = deltav[1] = deltav[2] = 0.0;

	/* Keep track if we've gone from still to not still or vice versa */
	was_still = player.still;
	player.still = 1;

	if (player.move_left > 0.0)
	{		
		RotateAbout (v, player.view, player.up, -theta*player.move_left);
		Vset (player.view, v);
		Normalize (player.view);
		Crossp (player.right, player.up, player.view);
		Normalize (player.right);
		player.still = 0;
	}

	if (player.move_right > 0.0)
	{
		RotateAbout (v, player.view, player.up, theta*player.move_right);
		Vset (player.view, v);
		Normalize (player.view);
		Crossp (player.right, player.up, player.view);
		Normalize (player.right);
		player.still = 0;
	}

	if (player.move_up > 0.0)
	{
		RotateAbout (v, player.view, player.right, theta*player.move_up);
		Vset (player.view, v);
		Normalize (player.view);
		Crossp (player.up, player.view, player.right);
		Normalize (player.up);
		player.still = 0;
	}

	if (player.move_down > 0.0)
	{
		RotateAbout (v, player.view, player.right, -theta*player.move_down);
		Vset (player.view, v);
		Normalize (player.view);
		Crossp (player.up, player.view, player.right);
		Normalize (player.up);
		player.still = 0;
	}

	if (player.move_pitchright > 0.0)
	{
		RotateAbout (v, player.up, player.view, -theta*player.move_pitchright);
		Vset (player.up, v);
		Normalize (player.up);
		Crossp (player.right, player.up, player.view);
		Normalize (player.right);
		player.still = 0;
	}

	if (player.move_pitchleft > 0.0)
	{
		RotateAbout (v, player.up, player.view, theta*player.move_pitchleft);
		Vset (player.up, v);
		Normalize (player.up);
		Crossp (player.right, player.up, player.view);
		Normalize (player.right);
		player.still = 0;
	}

	if (player.move_forward > 0.0)
	{
		if (player.flightmodel == FLIGHT_NEWTONIAN)
		{
			if (warpspeed)
			{
				if (superwarp && !am_client && !am_server)
					Vmul (v, player.view, 100.0*player.move_forward*DELTAV*deltaT);
				else
					Vmul (v, player.view, deltaT * (player.move_forward*DELTAV + WARP_COEFF*Mag(player.vel)));
			}
			else
				Vmul (v, player.view, player.move_forward*DELTAV*deltaT);
			Vadd (deltav, deltav, v);
		}
		else
		{
			if (player.throttle < MAX_THROTTLE)
			{
				if (warpspeed)
					player.throttle += 10.0*player.move_forward*DELTAV*deltaT;
				else
					player.throttle += player.move_forward*DELTAV*deltaT;
			}
			else
			{
				if (warpspeed)
					player.throttle += 10.0*100.0*player.move_forward*DELTAV*deltaT;
				else
					player.throttle += 100.0*player.move_forward*DELTAV*deltaT;
			}
			if (player.throttle > MAX_WARP_THROTTLE)
				player.throttle = MAX_WARP_THROTTLE;
		}
		player.still = 0;
	}

	if (player.move_backward > 0.0)
	{
		if (player.flightmodel == FLIGHT_NEWTONIAN)
		{
			if (warpspeed)
			{
				if (superwarp && !am_client && !am_server)
					Vmul (v, player.view, -100.0*player.move_backward*DELTAV*deltaT);
				else
					Vmul (v, player.view, -deltaT * (player.move_backward*DELTAV + WARP_COEFF*Mag(player.vel)));
			}
			else
				Vmul (v, player.view, -player.move_backward*DELTAV*deltaT);
			Vadd (deltav, deltav, v);
		}
		else
		{
			if (player.throttle < MAX_THROTTLE)
			{
				if (warpspeed)
					player.throttle -= 10.0*player.move_backward*DELTAV*deltaT;
				else
					player.throttle -= player.move_backward*DELTAV*deltaT;
			}
			else
			{
				if (warpspeed)
					player.throttle -= 100.0*10.0*player.move_backward*DELTAV*deltaT;
				else
					player.throttle -= 100.0*player.move_backward*DELTAV*deltaT;
			}
			if (player.throttle < 0.0) player.throttle = 0.0;
		}
		player.still = 0;
	}

	/* Compute change to velocity */
	if (player.flightmodel == FLIGHT_NEWTONIAN)
	{
		Vadd (player.vel, player.vel, deltav);
	}
	else
	{
		Vmul (player.vel, player.view, player.throttle);
	}

	/* Compute gravity's contribution */
	if (gravity && (player.flightmodel == FLIGHT_NEWTONIAN) )
	{
		Gravity (deltav, player.pos);
		Vadd (player.vel, player.vel, deltav);
	}

	/* Finaly, move player */
	Vmul (v, player.vel, deltaT);
	Vadd (player.pos, player.pos, v);

	/* See if player cratered on the planet */
	if (vulnerable)
	{
		if (-1 != (p = InsidePlanet (player.pos)))
		{
			if (!am_client && !am_server)
			{
				/* Make an explosion */
				Vsub (v, player.pos, planet[p].pos);
				Vmul (v, v, planet[p].radius*1.05);
				Vadd (v, v, planet[p].pos);
				Boom (v, 1.0);

				/* Send player back home */
				InitPlayer();
				gravity = 0;
				ReadMission (mission.fn);

				/* Give the bad news */
				sprintf (buf, "You hit %s and died!\\\\Restarting mission.",
					planet[p].name);
				Mprint (buf);
			}
			else if (am_server)
			{
				NetTargetCratered (client[server.client].target, p);
				NetPlayerDies();

				sprintf (buf, "You hit %s and died!", planet[p].name);
				Mprint (buf);
			}
		}
	}

	/* In network games, update position of target for the player */
	if (am_client || am_server)
	{
		if (am_client) t = client[clientme.client].target;
		if (am_server) t = client[server.client].target;

		Vset (target[t].pos, player.pos);
		Vset (target[t].vel, player.vel);
		Vset (target[t].up, player.up);
		Vset (target[t].view, player.view);

		target[t].move_up = player.move_up;
		target[t].move_down = player.move_down;
		target[t].move_right = player.move_right;
		target[t].move_left = player.move_left;
		target[t].move_pitchright = player.move_pitchright;
		target[t].move_pitchleft = player.move_pitchleft;
	}

	/* If we've gone from still to not still, or other way around,
	   send position report */
	if (was_still != player.still)
	{
		if (am_client) clientme.urgent = 1;
		QueuePositionReport();
	}
}

UpdatePlayer()
/*
 *  Update all sorts of stuff for the player
 */
{
	MovePlayer();

	/* Maintain shields */
	player.shields += deltaT * SHIELD_REGEN;
	if (player.shields > player.maxshields)
		player.shields = player.maxshields;

	/* Weapon idle time */
	player.msl_idle += deltaT;

	/* Dead time */
	if (state == STATE_DEAD1)
	{
		player.dead_timer -= deltaT;
		if (player.dead_timer <= 0.0)
		{
			state = STATE_DEAD2;
			player.dead_timer = 0.0;
		}
	}
}

PlayerFires()
/*
 *  Player maybe fires a missile
 */
{
	double v[3];

	paused = 0;

	/* Not if dead */
	if (state == STATE_DEAD1) return;

	/* Fire button respawns us */
	if (state == STATE_DEAD2)
	{
		state = STATE_NORMAL;
		return;
	}

	/* Can't fire too rapidly */
	if (player.msl_idle < weapon[player.weapon].idle)
	{
		return;
	}
	player.msl_idle = 0.0;

	/* Okay, fire the missile */
	Vmul (v, player.up, -0.01);
	Vadd (v, v, player.pos);
	FireMissile (v, player.vel, player.view, 1, player.weapon, -1);

	/* Send network notification */
	if (am_client)
	{
		SendASCIIPacket (clientme.socket, "FIRE %d", player.weapon);
	}

	if (am_server)
	{
		NetClientFires (server.client, player.weapon);
	}
}

void DoName (void)
/*
 *  Sparky has typed a new name
 */
{
	int i, len;

	len = strlen (text.buf);

	/* Remove spaces */
	for (i=0; i<len; i++)
	{
		if (text.buf[i] == ' ') text.buf[i] = '_';
	}
	strcpy (player.name, text.buf);
}

ViewLock()
/*
 *  Aim player toward locked object
 */
{
	double pos[3], v[3];

	v[0] = 1.0; v[1] = 0.0; v[2] = 0.0;

	/* Not if we're not locked */
	if (lock.target == (-1)) return;

	/* Figure out what we're locked on */
	if (lock.type == LOCK_ENEMY)
	{
		Vset (pos, target[lock.target].pos);
	}
	else if (lock.type == LOCK_FRIENDLY)
	{
		Vset (pos, target[lock.target].pos);
	}
	else if (lock.type == LOCK_PLANET)
	{
		Vset (pos, planet[lock.target].pos);
	}
	else
	{
		/* Can't happen */
		Log ("ViewLock: Invalid lock.type");
		return;
	}

	/* Set view vector */
	Vsub (player.view, pos, player.pos);
	Normalize (player.view);

	/* Set up vector */
/*	Perp (player.up, player.view);	*/
	Crossp (player.up, player.view, v);
	Normalize (player.up);

	/* Try to keep the up vector pointing up */
/*	if (player.up[2] < player.pos[2]) Vmul (player.up, player.up, -1.0);	*/

	/* Set right-hand vector */
	Crossp (player.right, player.up, player.view);
}
