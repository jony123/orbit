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

/* Ignore beyond this distance */
#define THINK_CUTOFFA (20000.0 / KM_TO_UNITS1)
#define THINK_CUTOFFA2 (THINK_CUTOFFA * THINK_CUTOFFA)

/* Pursue beyond this distance */
#define THINK_CUTOFFB (2000.0 / KM_TO_UNITS1)
#define THINK_CUTOFFB2 (THINK_CUTOFFB * THINK_CUTOFFB)

/* Maintain distance beyond this distance */
#define THINK_CUTOFFC (500.0 / KM_TO_UNITS1)
#define THINK_CUTOFFC2 (THINK_CUTOFFC * THINK_CUTOFFC)

ThinkTarget (t)
int t;
/*
 *  Give this target a chance to think about what it wants to do
 */
{
	/* Targets do their own thinking in network games */
	if (am_client || am_server) return;

	switch (target[t].strategy)
	{
	case STRAT_DONOTHING:
		break;

	case STRAT_SIT1:
		ThinkSit1 (t);
		break;

	case STRAT_SIT2:
		ThinkSit2 (t);
		break;

	case STRAT_SIT3:
		ThinkSit3 (t);
		break;

	case STRAT_SIT4:
		ThinkSit4 (t);
		break;

	case STRAT_HUNT1:
		ThinkHunt1 (t);
		break;

	case STRAT_HUNT2:
		ThinkHunt2 (t);
		break;

	case STRAT_HUNT3:
		ThinkHunt3 (t);
		break;

	case STRAT_HUNT4:
		ThinkHunt4 (t);
		break;
	}
}

ThinkSit1 (t)
int t;
/*
 *  Don't move, just turn toward player and shoot
 */
{
	TurnToward (t, player.pos);
}

ThinkSit2 (t)
int t;
/*
 *  Don't move, just turn toward player and shoot
 */
{
	double v[3];

	/* Aim at player */
	if (!Aim (v, target[t].pos, target[t].vel, player.pos, player.vel,
	    weapon[target[t].weapon].speed))
	{
		TurnToward (t, player.pos);
		return;
	}

	TurnToward (t, v);
}

ThinkSit3 (t)
int t;
/*
 *  Don't move, just turn toward nearest enemy and shoot poorly
 */
{
	double pos[3], vel[3];

	if (!FindEnemy (t, pos, vel)) return;
	TurnToward (t, pos);
}

ThinkSit4 (t)
int t;
/*
 *  Don't move, just turn toward nearest enemy and shoot well
 */
{
	double v[3], pos[3], vel[3];

	if (!FindEnemy (t, pos, vel)) return;
	if (!Aim (v, target[t].pos, target[t].vel, pos, vel,
	    weapon[target[t].weapon].speed))
	{
		TurnToward (t, pos);
		return;
	}

	TurnToward (t, v);
}

ThinkHunt1 (t)
int t;
/*
 *  Move toward player and shoot
 */
{
	MoveToward (t, player.pos);
}

ThinkHunt2 (t)
int t;
/*
 *  Move toward aiming point and shoot
 */
{
	double v[3];

	/* Aim at player */
	if (!Aim (v, target[t].pos, target[t].vel, player.pos, player.vel,
	    weapon[target[t].weapon].speed))
	{
		ThinkHunt1 (t);
		return;
	}

	MoveToward (t, v);
}

ThinkHunt3 (t)
int t;
/*
 *  Move toward closest enemy and shoot poorly
 */
{
	double pos[3], vel[3];

	if (!FindEnemy (t, pos, vel)) return;
	MoveToward (t, pos);
}

ThinkHunt4 (t)
int t;
/*
 *  Move toward closest enemy and shoot well
 */
{
	double v[3], pos[3], vel[3];

	if (!FindEnemy (t, pos, vel)) return;
	if (!Aim (v, target[t].pos, target[t].vel, pos, vel,
	    weapon[target[t].weapon].speed))
	{
		ThinkHunt3 (t);
		return;
	}

	MoveToward (t, v);
}

TurnToward (t, pos)
int t;
double pos[3];
{
/*
 *  Turn target t toward position v
 */
	double r, v[3], alpha, beta, theta;

	Vsub (v, pos, target[t].pos);
	r = Mag2 (v);

	/* Don't bother if too far away */
	if (r > THINK_CUTOFFA2)
	{
		target[t].vel[0] =
		target[t].vel[1] =
		target[t].vel[2] = 0.0;

		return;
	}

	/* v is unit vector toward player */
	Normalize (v);

	/* Determine angles from right-hand and up vectors */
	alpha = Dotp (v, target[t].right);
	beta = Dotp (v, target[t].up);
	theta = Dotp (v, target[t].view);

	/* Turn towards player */

	/* First check left or right */
	/* Is player to our right or left? */
	if (alpha > 0.0)
	{
		/* To our right */
		target[t].move_left = target[t].turnrate;
	}
	else
	{
		/* To our left */
		target[t].move_right = target[t].turnrate;
	}

	/* Up or down? */
	if (beta > 0.0)
	{
		/* Above us */
		target[t].move_up = target[t].turnrate;
	}
	else
	{
		target[t].move_down = target[t].turnrate;
	}

	/* If we are facing target and are in range, shoot
	   a missile! */
	if ( (theta > 0.9) && (r < weapon[target[t].weapon].range2) )
	{
		TargetFiresMissile (t);
	}
}

MoveToward (t, pos)
int t;
double pos[3];
{
/*
 *  Turn target t toward position v
 */
	double r, v[3], alpha, beta, theta;

	Vsub (v, pos, target[t].pos);
	r = Mag2 (v);

	/* Don't bother if too far away */
	if (r > THINK_CUTOFFA2)
	{
		target[t].vel[0] =
		target[t].vel[1] =
		target[t].vel[2] = 0.0;

		return;
	}

	/* v is unit vector toward player */
	Normalize (v);

	/* Determine angles from right-hand and up vectors */
	alpha = Dotp (v, target[t].right);
	beta = Dotp (v, target[t].up);
	theta = Dotp (v, target[t].view);

	/* Turn towards player */

	/* First check left or right */
	/* Is player to our right or left? */
	if (alpha > 0.0)
	{
		/* To our right */
		target[t].move_left = target[t].turnrate;
	}
	else
	{
		/* To our left */
		target[t].move_right = target[t].turnrate;
	}

	/* Up or down? */
	if (beta > 0.0)
	{
		/* Above us */
		target[t].move_up = target[t].turnrate;
	}
	else
	{
		target[t].move_down = target[t].turnrate;
	}

	/* Don't bother if too far away */
	if (r > THINK_CUTOFFA2)
	{
		target[t].vel[0] = target[t].vel[1] = target[t].vel[2] = 0.0;
	}

	/* If far enough away, move toward player */
	if (r > THINK_CUTOFFB2)
	{
		if (theta > 0.0)
			target[t].move_forward = target[t].maxvel;
		else
			target[t].move_backward = target[t].maxvel;
	}
	/* If too close, move away */
	if (r < THINK_CUTOFFC2)
	{
		target[t].vel[0] = target[t].vel[1] = target[t].vel[2] = 0.0;
	}

	/* If we are facing target and are in range, shoot
	   a missile! */
	if ( (theta > 0.9) && (r < weapon[target[t].weapon].range2) )
	{
		TargetFiresMissile (t);
	}
}

int FindEnemy (targ, pos, vel)
int targ;
double pos[3], vel[3];
/*
 *  Find closest enemy to target targ, return position and speed
 */
{
	int t, tt;
	double d, v[3], r;

	/* d will be distance to closest */
	d = -1.0;

	/* Loop through targets */
	for (t=0; t<NTARGETS; t++)
	{
		if ( (target[t].age > 0.0) &&
		     (!target[t].hidden) &&
		     (t != targ) &&
		     (target[t].friendly != target[targ].friendly) )
		{
			/* Closer? */
			Vsub (v, target[t].pos, target[targ].pos);
			r = Mag2 (v);
			if ( (d < 0.0) || (r < d) )
			{
				tt = t;
				d = r;
			}
		}
	}

	/* Set position and velocity if we found something */
	if (d > 0.0)
	{
		Vset (pos, target[tt].pos);
		Vset (vel, target[tt].vel);
	}

	/* Must check player? */
	if (!target[targ].friendly)
	{
		if ( (d < 0.0) || (target[targ].range2 < d) )
		{
			Vset (pos, player.pos);
			Vset (vel, player.vel);
			d = target[targ].range2;
		}
	}

	/* Found nothing? */
	if (d == (-1.0)) return 0;

	return 1;
}

