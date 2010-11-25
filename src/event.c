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
 *  Stuff to implement events
 */

InitEvents ()
/*
 *  Initialize events
 */
{
	int e, a;

	for (e=0; e<NEVENTS; e++)
	{
		event[e].name[0] = 0;
		event[e].pending = 0;
		event[e].enabled = 1;
		event[e].trigger = EVENT_NULL;
		event[e].ivalue = 0;
		event[e].fvalue = 0.0;
		event[e].cvalue = NULL;
		event[e].pos[0] = event[e].pos[1] = event[e].pos[2] = 0.0;

		for (a=0; a<ACTIONS_PER_EVENT; a++)
		{
			event[e].action[a].active = 0;
			event[e].action[a].action = EVENT_NULL;
			event[e].action[a].ivalue = 0;
			event[e].action[a].fvalue = 0.0;
			event[e].action[a].cvalue = NULL;
		}
	}
}

ResetEvents ()
/*
 *  Re-Initialize events
 */
{
	int e, a;

	for (e=0; e<NEVENTS; e++)
	{
		event[e].name[0] = 0;
		event[e].pending = 0;
		event[e].enabled = 1;
		event[e].trigger = EVENT_NULL;
		if (event[e].cvalue != NULL) free (event[e].cvalue);
		event[e].cvalue = NULL;
		event[e].ivalue = 0;
		event[e].fvalue = 0.0;
		event[e].pos[0] = event[e].pos[1] = event[e].pos[2] = 0.0;

		for (a=0; a<ACTIONS_PER_EVENT; a++)
		{
			event[e].action[a].active = 0;
			event[e].action[a].action = EVENT_NULL;
			event[e].action[a].ivalue = 0;
			event[e].action[a].fvalue = 0.0;
			if (event[e].action[a].cvalue != NULL)
				free (event[e].action[a].cvalue);
			event[e].action[a].cvalue = NULL;
		}
	}
}

int FindEvent()
/*
 *  Find unused event
 */
{
	int e, a;

	for (e=0; e<NEVENTS; e++)
	{
		if (!event[e].pending)
		{
			event[e].pending = 1;
			event[e].enabled = 1;

			/* Free any memory previously used by this event */
			if (NULL != event[e].cvalue)
			{
				free (event[e].cvalue);
				event[e].cvalue = NULL;
			}

			for (a=0; a<ACTIONS_PER_EVENT; a++)
			{
				if (NULL != event[e].action[a].cvalue)
				{
					free (event[e].action[a].cvalue);
					event[e].action[a].cvalue = NULL;
				}
			}

			return (e);
		}
	}

	Log ("Out of events!  Increase NEVENTS in orbit.h");
	FinishSound();
	CloseLog();
	exit (0);
}

int FindEventByName (name)
char *name;
{
	int e;

	for (e=0; e<NEVENTS; e++)
	{
		if (!strcasecmp (name, event[e].name)) return (e);
	}
	return (-1);
}

DoEvents()
/*
 *  Process events
 */
{
	int e;

	for (e=0; e<NEVENTS; e++)
	{
		if (event[e].pending && event[e].enabled)
		{
			switch (event[e].trigger)
			{
			case EVENT_NULL:
				break;

			case EVENT_TRUE:
				EventAction (e);
				break;

			case EVENT_APPROACH:
				EventTrigApproach (e);
				break;

			case EVENT_DEPART:
				EventTrigDepart (e);
				break;

			case EVENT_DESTROY:
				EventTrigDestroy (e);
				break;

			case EVENT_SCORE:
				EventTrigScore (e);
				break;

			case EVENT_ALARM:
				EventTrigAlarm (e);
				break;

			case EVENT_STOPNEAR:
				EventTrigStopnear (e);
				break;

			case EVENT_SHIELDS:
				EventTrigShields (e);
				break;

			default:
				Log ("Unknown event trigger type: %d, event %d",
				        event[e].trigger, e);
				FinishSound();
				CloseLog();
				exit (0);
				break;
			}
		}
	}
}

EventTrigApproach (e)
int e;
{
	double v[3], r;

	/* Find range to destination */
	Vsub (v, event[e].pos, player.pos);
	r = Mag2 (v);

	/* There yet? */
	if (r <= event[e].fvalue*event[e].fvalue) EventAction (e);
}

EventTrigStopnear (e)
int e;
{
	double v[3], r;

	if ( (player.vel[0] != 0.0) ||
	     (player.vel[1] != 0.0) ||
	     (player.vel[2] != 0.0) ) return;

	/* Find range to destination */
	Vsub (v, event[e].pos, player.pos);
	r = Mag2 (v);

	/* There yet? */
	if (r <= event[e].fvalue*event[e].fvalue) EventAction (e);
}

EventTrigDepart (e)
int e;
{
	double v[3], r;

	/* Find range to destination */
	Vsub (v, event[e].pos, player.pos);
	r = Mag2 (v);

	/* There yet? */
	if (r > event[e].fvalue*event[e].fvalue)
	{
		Log ("EventTrigDepart: r=%lf, fvalue=%lf",
				r, event[e].fvalue);
		EventAction (e);
	}
}

EventTrigDestroy (e)
int e;
/*
 *  Do nothing here -- destroy triggers are checked by
 *  DestroyTarget in target.c
 */
{}

EventTrigShields (e)
int e;
/*
 *  Not checked here -- checked in MissileHitTarget in missile.c
 */
{}

EventTrigScore (e)
int e;
{
	if (player.score >= event[e].ivalue) EventAction (e);
}

EventTrigAlarm (e)
int e;
{
	event[e].fvalue -= deltaT;
	if (event[e].fvalue <= 0.0) EventAction (e);
}

EventAction (e)
int e;
/*
 *  Event e has occurred!
 */
{
	int t, ev, a, p;

	event[e].pending = 0;

	/* Process all action */
	for (a=0; a<ACTIONS_PER_EVENT; a++)
	{
	  if (event[e].action[a].active)
	  {
		Log ("EventAction: Event %d.%d has happened, trigger %d action %d",
			e, a, event[e].trigger, event[e].action[a].action);

		switch (event[e].action[a].action)
		{
		case EVENT_NULL:
			break;

		case EVENT_MESSAGE:
			Mprint (event[e].action[a].cvalue);
			break;

		case EVENT_HIDE:
			if ((-1) != (t = FindTargetByName (event[e].action[a].cvalue)))
			{
				target[t].hidden = 1;
			}
			break;

		case EVENT_UNHIDE:
			if ((-1) != (t = FindTargetByName (event[e].action[a].cvalue)))
			{
				target[t].hidden = 0;
			}
			break;

		case EVENT_DESTROY:
			if ((-1) != (t = FindTargetByName (event[e].action[a].cvalue)))
			{
				DestroyTarget (t);
			}
			break;

		case EVENT_SCORE:
			player.score += event[e].action[a].ivalue;
			break;

		case EVENT_ENABLE:
			if ((-1) != (ev = FindEventByName (event[e].action[a].cvalue)))
			{
				event[ev].enabled = 1;
			}
			break;

		case EVENT_DISABLE:
			if ((-1) != (ev = FindEventByName (event[e].action[a].cvalue)))
			{
				event[ev].enabled = 0;
			}
			break;

		case EVENT_LOADMISSION:
			Log ("EventAction is calling ReadMission(%s)", event[e].action[a].cvalue);
			ReadMission (event[e].action[a].cvalue);
			return;

		case EVENT_STOP:
			player.vel[0] = player.vel[1] = player.vel[2] = 0.0;
			break;

		case EVENT_BOOM:
			Boom (event[e].pos, event[e].action[a].fvalue);
			break;

		case EVENT_FLASH:
			palette_flash = 1;
			break;

		case EVENT_MOVEOBJECT:
			if ((-1) != (t = FindTargetByName (event[e].action[a].cvalue)))
			{
				Vset (target[t].pos, event[e].pos);
			}
			break;

		case EVENT_MOVEPLAYER:
			Vset (player.pos, event[e].pos);
			break;

		case EVENT_MOVEPLANET:
			if ((-1) != (p = FindPlanetByName (event[e].action[a].cvalue)))
			{
				Vset (planet[p].pos, event[e].pos);
			}
			break;

		case EVENT_HIDEPLANET:
			if ((-1) != (p = FindPlanetByName (event[e].action[a].cvalue)))
			{
				planet[p].hidden = 1;
			}
			break;

		case EVENT_UNHIDEPLANET:
			if ((-1) != (p = FindPlanetByName (event[e].action[a].cvalue)))
			{
				planet[p].hidden = 0;
			}
			break;

		case EVENT_BETRAY:
			if ((-1) != (t = FindTargetByName (event[e].action[a].cvalue)))
			{
				target[t].friendly = !target[t].friendly;
			}
			break;

		default:
			Log ("Unknown event action type %d, event %d",
				    event[e].action, e);
			FinishSound();
			CloseLog();
			exit (0);
		}
	  }
	}

	/* Free memory used by this event */
	if (NULL != event[e].cvalue)
	{
		free (event[e].cvalue);
		event[e].cvalue = NULL;
	}
	for (a=0; a<ACTIONS_PER_EVENT; a++)
	{
		if (NULL != event[e].action[a].cvalue)
		{
			free (event[e].action[a].cvalue);
			event[e].action[a].cvalue = NULL;
		}
	}

	/* An event might have an effect on the locked target */
	CheckLock();
}
