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

/* File descriptor stack */
#define NFDS (16)
FILE *fd[NFDS];
int sp;

char token[128], tokens[10][128];
int iact;
int planet_index;
int weapon_index;

int TAflag;
#define TRIGGER 0
#define ACTION 1

ReadMission (msn)
char *msn;
/*
 *  Read a mission file
 */
{
	char fn[128], buf[128];
	int i;

	/* Bye if no file name */
	if ( (msn == NULL) || (msn[0] == 0) )
	{
		Mprint ("No mission to load!");
		Log ("ReadMission: No mission to load!");
		return;
	}

	/* Construct file name */
	sprintf (fn, "missions/%s", msn);

	Log ("ReadMission: reading %s", fn);

	/* Init some stuff */
	mission.cursor[0] = mission.cursor[1] = mission.cursor[2] = 0.0;
	mission.verbose = 0;
	strcpy (mission.fn, msn);
	player.score = 0;
	mission.briefing[0] = 0;
	for (i=0; i<10; i++) tokens[i][0] = 0;
	sp = 0;

	/* Reset targets, events, weapons and planets, etc; */
	InitTargets();
	ResetEvents();
	InitWeapons();
	ResetPlanets();
	InitWaypoints();
	lock.target = (-1);

	/* Open the file */
	if (NULL == (fd[0] = fopen (fn, "r")))
	{
		Log ("ReadMission: Can't open %s", fn);
		sprintf (buf, "Can't open mission file %s", fn);
		Mprint (buf);
		return;
	}

	/* Process tokens */
	while (GetToken())
	{
		if (!strcasecmp (token, "cursor")) Cursor();
		else if (!strcasecmp (token, "player")) Player();
		else if (!strcasecmp (token, "waypoint")) Waypoint();
		else if (!strcasecmp (token, "object")) Object();
		else if (!strcasecmp (token, "briefing")) Briefing();
		else if (!strcasecmp (token, "verbose")) mission.verbose = 1;
		else if (!strcasecmp (token, "terse")) mission.verbose = 0;
		else if (!strcasecmp (token, "event")) Event();
		else if (!strcasecmp (token, "planet")) Planet();
		else if (!strcasecmp (token, "weapon")) Weapon();
		else if (!strcasecmp (token, "include")) Include();
		else UnrecognizedToken();
	}

	/* Add mission to saved games list */
	AddSave (mission.fn);

	/* Show mission briefing */
	Mprint (mission.briefing);

	Log ("ReadMission: Done reading mission");
}

int GetToken()
/*
 *  Read next token from file
 */
{
	/* Get next token */
	if (1 != fscanf (fd[0], "%s", token))
	{
		fclose (fd[0]);

		/* Inside include? */
		if (!PopFD()) return 0;

		/* Recurse */
		return (GetToken());
	}

	RotateTokens();

	/* Start of comment? */
	if (strcasecmp (token, "/*")) return (1);

	/* Look for end of comment */
	do
	{
		if (1 != fscanf (fd[0], "%s", token))
		{
			fclose (fd[0]);

			/* Inside include? */
			if (!PopFD()) return 0;
		}
		RotateTokens();
	} while (strcasecmp (token, "*/"));

	/* Recurse to get next token */
	return (GetToken());		
}

RotateTokens()
{
	int i;

	for (i=9; i>0; i--) strcpy (tokens[i], tokens[i-1]);
	strcpy (tokens[0], token);
}

ShowTokens()
{
	Log ("Recent tokens: %s %s %s %s %s %s %s %s %s %s",
		tokens[9], tokens[8], tokens[7], tokens[6], tokens[5],
		tokens[4], tokens[3], tokens[2], tokens[1], tokens[0]);
}

int GetBrace()
/*
 *  Get open brace, error otherwise
 */
{
	if (!GetToken())
	{
		Log ("GetBrace: Unexpected end of file in %s!", mission.fn);
		ShowTokens();
		FinishSound();
		CloseLog();
		exit (0);
	}

	if (strcmp (token, "{"))
	{
		Log ("GetBrace: Expected '{', found '%s'", token);
		ShowTokens();
		FinishSound();
		CloseLog();
		exit (0);
	}

	return (1);
}

int GetRequiredToken()
/*
 *  Get a token, error if EOF
 */
{
	if (!GetToken())
	{
		Log ("GetRequiredToken: Unexpected end of file in %s!", mission.fn);
		ShowTokens();
		FinishSound();
		CloseLog();
		exit (0);
	}
}

Cursor()
{
	int xyzflag, relflag;
	double x;

	GetBrace();

	xyzflag = 0;

	while (GetToken())
	{
		if (!strcmp (token, "}")) return;

		if ( (token[0] == '+') || (token[0] == '-') ||
		     ((token[0] >= '0') && (token[0] <= '9')) )
		{
			/* It's a number */

			/* See if abs or relative */
			relflag = 0;
			if (token[0] == '+') relflag = 1;
			if (token[0] == '-') relflag = 1;

			sscanf (token, "%lf", &x);
			x = x / KM_TO_UNITS1;

			if (relflag)
				mission.cursor[xyzflag] += x;
			else
				mission.cursor[xyzflag] = x;

			xyzflag = (xyzflag + 1) % 3;

			if (mission.verbose)
				Log ("Set cursor to %lf %lf %lf",
				mission.cursor[0],
				mission.cursor[1],
				mission.cursor[2]);
		}
		else
		{
			/* It's a planet */
			if (!CursorPlanet()) CursorObject();

			if (mission.verbose)
			{
				Log ("Cursor: Setting cursor to %s",
					token);
				Log ("Cursor: (Set cursor to %lf %lf %lf)",
				mission.cursor[0],
				mission.cursor[1],
				mission.cursor[2]);
			}
		}
	}
}

CursorPlanet()
{
	int p;

	for (p=0; p<NPLANETS; p++)
	{
		if (!strcasecmp (token, planet[p].name))
		{
			Vset (mission.cursor, planet[p].pos);
			return 1;
		}
	}

	Log ("CursorPlanet: No such planet: %s", token);
	return 0;
}

CursorObject()
{
	int t;

	for (t=0; t<NTARGETS; t++)
	{
		if (target[t].age > 0.0)
		{
			if (!strcasecmp (token, target[t].name))
			{
				Vset (mission.cursor, target[t].pos);
				return 1;
			}
		}
	}

	Log ("CursorObject: No such object: %s", token);
	return 0;
}

Player()
{
	GetBrace();

	mission.player[0] = mission.cursor[0];
	mission.player[1] = mission.cursor[1];
	mission.player[2] = mission.cursor[2];

	player.vel[0] = player.vel[1] = player.vel[2] = 0.0;

	if (mission.verbose)
		Log ("Player: Set player position to %lf %lf %lf",
		mission.player[0], mission.player[1], mission.player[2]);

	Vset (player.pos, mission.player);

	while (GetToken())
	{
		if (!strcmp (token, "}")) return;
		else
		{
			UnrecognizedToken();
		}
	}
}

Waypoint()
{
	GetBrace();

	if (mission.verbose) Log ("Waypoint: Adding waypoint %d", nwaypoints);
	AddWaypoint (mission.cursor);

	while (GetToken())
	{
		if (!strcmp (token, "}")) return;
		else
		{
			UnrecognizedToken();
		}
	}
}

Briefing()
{
	mission.briefing[0] = 0;

	GetBrace();

	while (GetToken())
	{
		if (!strcmp (token, "}"))
		{
			if (mission.verbose)
				Log ("Briefing: %s", mission.briefing);
			return;
		}
		strcat (mission.briefing, " ");
		strcat (mission.briefing, token);
	}
}

UnrecognizedToken()
{
	Log ("UnrecognizedToken: Skipping unrecognized token \"%s\" in %s",
		token, mission.fn);
	ShowTokens();
}

Object()
{
	int t;

	GetBrace();

	/* Find an unused target */
	t = FindTarget();

	if (mission.verbose) Log ("Object: Creating object %d", t);

	/* Set it up */
	Vset (target[t].pos, mission.cursor);
	target[t].age = 0.1;
	target[t].view[0] = 1.0;
	target[t].view[1] = target[t].view[2] = 0.0;

	target[t].hidden = 0;
	target[t].invisible = 0;

	target[t].up[0] = 0.0;
	target[t].up[1] = 0.0;
	target[t].up[2] = 1.0;

	target[t].strategy = STRAT_DONOTHING;
	target[t].friendly = 0;
	target[t].weapon = NPLAYER_WEAPONS;

	Crossp (target[t].right, target[t].up, target[t].view);

	target[t].vel[0] = target[t].vel[1] = target[t].vel[2] = 0.0;

	target[t].move_forward = target[t].move_backward =
		target[t].move_up = target[t].move_down =
		target[t].move_pitchleft = target[t].move_pitchright =
		target[t].move_left = target[t].move_right = 0.0;

	target[t].model = 0;
	target[t].list = model[0].list;
	target[t].score = 0;
	strcpy (target[t].name, model[0].name);

	target[t].maxshields = 100.0;
	target[t].shieldregen = SHIELD_REGEN;
	target[t].turnrate = 0.3;
	target[t].maxvel = 0.01;

	while (GetToken())
	{
		if (!strcmp (token, "}")) return;

		if (!strcasecmp (token, "model")) ObjModel (t);
		else if (!strcasecmp (token, "score")) ObjScore (t);
		else if (!strcasecmp (token, "strategy")) ObjStrategy (t);
		else if (!strcasecmp (token, "name")) ObjName (t);
		else if (!strcasecmp (token, "hidden")) ObjHidden (t);
		else if (!strcasecmp (token, "weapon")) ObjWeapon (t);
		else if (!strcasecmp (token, "friendly")) ObjFriendly (t);
		else if (!strcasecmp (token, "maxshields")) ObjMaxshields (t);
		else if (!strcasecmp (token, "shieldregen")) ObjShieldregen (t);
		else if (!strcasecmp (token, "turnrate")) ObjTurnrate (t);
		else if (!strcasecmp (token, "speed")) ObjSpeed (t);
		else if (!strcasecmp (token, "invisible")) ObjInvisible (t);
		else UnrecognizedToken ();
	}
}

ObjMaxshields (t)
int t;
{
	GetRequiredToken();
	target[t].maxshields = atof (token);
	target[t].shields = target[t].maxshields;

	if (mission.verbose)
		Log ("ObjMaxshields: Object %d shields set to %lf", t, target[t].shields);
}

ObjShieldregen (t)
int t;
{
	GetRequiredToken();
	target[t].shieldregen = atof (token);

	if (mission.verbose)
		Log ("ObjShieldregen: Object %d regen set to %lf", t, target[t].shieldregen);
}

ObjTurnrate (t)
int t;
{
	GetRequiredToken();
	target[t].turnrate = atof (token);

	if (mission.verbose)
		Log ("ObjTurnrate: Object %d turnrate set to %lf", t, target[t].turnrate);
}

ObjSpeed (t)
int t;
{
	GetRequiredToken();
	target[t].maxvel = atof (token);

	if (mission.verbose)
		Log ("ObjSpeed: Object %d speed set to %lf", t, target[t].maxvel);
}

ObjModel (t)
int t;
{
	int m;

	GetRequiredToken();

	m = LoadModel (token);
	target[t].model = m;
	if (m == (-1))
	{
		Log ("ObjModel: Couldn't open model %s", token);
	}
	else
	{
		target[t].list = model[m].list;
	}

	if (mission.verbose)
		Log ("ObjModel: Object %d model set to %s", t, model[m].name);
}

ObjScore (t)
int t;
{
	GetRequiredToken();

	sscanf (token, "%d", &target[t].score);

	if (mission.verbose) Log ("ObjScore: Object %d score is %d",
					t, target[t].score);
}

ObjName (t)
{
	GetRequiredToken();

	strcpy (target[t].name, token);

	if (mission.verbose) Log ("ObjName: Object %d named \"%s\"",
					t, target[t].name);
}

ObjStrategy (t)
int t;
{
	GetRequiredToken();

	if (mission.verbose)
		Log ("ObjStrategy: Setting object %d strategy to %s", t, token);

	if (!strcasecmp (token, "sit1"))
	{
		target[t].strategy = STRAT_SIT1;
	}
	else if (!strcasecmp (token, "sit2"))
	{
		target[t].strategy = STRAT_SIT2;
	}
	else if (!strcasecmp (token, "sit3"))
	{
		target[t].strategy = STRAT_SIT3;
	}
	else if (!strcasecmp (token, "sit4"))
	{
		target[t].strategy = STRAT_SIT4;
	}
	else if (!strcasecmp (token, "hunt1"))
	{
		target[t].strategy = STRAT_HUNT1;
	}
	else if (!strcasecmp (token, "hunt2"))
	{
		target[t].strategy = STRAT_HUNT2;
	}
	else if (!strcasecmp (token, "hunt3"))
	{
		target[t].strategy = STRAT_HUNT3;
	}
	else if (!strcasecmp (token, "hunt4"))
	{
		target[t].strategy = STRAT_HUNT4;
	}
	else if (!strcasecmp (token, "donothing"))
	{
		target[t].strategy = STRAT_DONOTHING;
	}
	else
	{
		Log ("ObjStrategy: No such strategy: %s", token);
	}
}

ObjHidden (t)
int t;
{
	target[t].hidden = 1;

	if (mission.verbose)
		Log ("ObjHidden: Object %d is hidden", t);
}

ObjInvisible (t)
int t;
{
	target[t].invisible = 1;

	if (mission.verbose)
		Log ("ObjInvisible: Object %d is invisible", t);
}

ObjFriendly (t)
int t;
{
	target[t].friendly = 1;

	if (mission.verbose)
		Log ("ObjFriendly: Object %d is friendly", t);
}

ObjWeapon (t)
int t;
{
	GetRequiredToken();
	target[t].weapon = atoi (token);
	if ( (target[t].weapon >= NWEAPONS) ||
	     (target[t].weapon < 0) )
		target[t].weapon = 0;

	if (mission.verbose)
		Log ("ObjWeapon: Object %d weapon set to %d", t, target[t].weapon);
}

Event()
{
	int e, a;

	GetBrace();

	/* Find an unused event entry */
	e = FindEvent();

	if (mission.verbose) Log ("Event: Creating event %d", e);

	/* Index to next action */
	iact = (-1);

	/* Values for triggers or actions? */
	TAflag = TRIGGER;

	/* Set up some stuff */
	event[e].name[0] = 0;
	event[e].enabled = 1;
	event[e].trigger = EVENT_NULL;
	Vset (event[e].pos, mission.cursor);
	event[e].ivalue = 0;
	event[e].fvalue = 0.0;
	for (a=0; a<ACTIONS_PER_EVENT; a++)
	{
		event[e].action[a].active = 0;
		event[e].action[a].action = EVENT_NULL;
		event[e].action[a].ivalue = 0;
		event[e].action[a].fvalue = 0.0;
	}

	/* Process rest of event tokens */
	while (1)
	{
		GetRequiredToken();
		if (!strcmp (token, "}"))
		{
			/* Convert units if this is Approach or Depart */
			if ( (event[e].trigger == EVENT_APPROACH) ||
			     (event[e].trigger == EVENT_STOPNEAR) ||
			     (event[e].trigger == EVENT_DEPART) )
			{
				event[e].fvalue /= KM_TO_UNITS1;
			}

			/* Shields is weird, has a text and floating value */
			if (event[e].trigger == EVENT_SHIELDS)
			{
				sscanf (event[e].cvalue, "%*s %lf", &event[e].fvalue);
				sscanf (event[e].cvalue, "%s", event[e].cvalue);
				if (mission.verbose)
					Log ("Event: EVENT_SHIELDS target %s value %lf",
						event[e].cvalue, event[e].fvalue);
			}

			return;
		}

		if (!strcasecmp (token, "trigger")) EvTrigger (e);
		else if (!strcasecmp (token, "name")) EvName (e);
		else if (!strcasecmp (token, "action")) EvAction (e);
		else if (!strcasecmp (token, "value")) EvValue (e);
		else if (!strcasecmp (token, "enabled")) event[e].enabled = 1;
		else if (!strcasecmp (token, "disabled")) event[e].enabled = 0;
		else UnrecognizedToken ();
	}
}

EvName (e)
int e;
{
	GetRequiredToken();
	strcpy (event[e].name, token);

	if (mission.verbose)
		Log ("EvName: Event %d named %s", e, token);
}

EvTrigger (e)
int e;
{
	GetRequiredToken();

	TAflag = TRIGGER;

	if (mission.verbose)
		Log ("EvTrigger: Setting event %d trigger to %s", e, token);

	if (!strcasecmp (token, "approach")) event[e].trigger = EVENT_APPROACH;
	else if (!strcasecmp (token, "depart")) event[e].trigger = EVENT_DEPART;
	else if (!strcasecmp (token, "true")) event[e].trigger = EVENT_TRUE;
	else if (!strcasecmp (token, "score")) event[e].trigger = EVENT_SCORE;
	else if (!strcasecmp (token, "destroy")) event[e].trigger = EVENT_DESTROY;
	else if (!strcasecmp (token, "alarm")) event[e].trigger = EVENT_ALARM;
	else if (!strcasecmp (token, "stopnear")) event[e].trigger = EVENT_STOPNEAR;
	else if (!strcasecmp (token, "shields")) event[e].trigger = EVENT_SHIELDS;
	else UnrecognizedToken();
}

EvAction (e)
int e;
{
	/* Bump action index */
	iact++;
	if (iact >= ACTIONS_PER_EVENT)
	{
		Log ("EvAction: Too many actions in event %d!", e);
		iact--;
	}

	event[e].action[iact].active = 1;

	TAflag = ACTION;

	GetRequiredToken();

	if (mission.verbose)
		Log ("EvAction: Setting event %d.%d action to %s", e, iact, token);

	if (!strcasecmp (token, "message")) event[e].action[iact].action = EVENT_MESSAGE;
	else if (!strcasecmp (token, "hide")) event[e].action[iact].action = EVENT_HIDE;
	else if (!strcasecmp (token, "unhide")) event[e].action[iact].action = EVENT_UNHIDE;
	else if (!strcasecmp (token, "destroy")) event[e].action[iact].action = EVENT_DESTROY;
	else if (!strcasecmp (token, "score")) event[e].action[iact].action = EVENT_SCORE;
	else if (!strcasecmp (token, "enable")) event[e].action[iact].action = EVENT_ENABLE;
	else if (!strcasecmp (token, "disable")) event[e].action[iact].action = EVENT_DISABLE;
	else if (!strcasecmp (token, "loadmission")) event[e].action[iact].action = EVENT_LOADMISSION;
	else if (!strcasecmp (token, "stop")) event[e].action[iact].action = EVENT_STOP;
	else if (!strcasecmp (token, "boom")) event[e].action[iact].action = EVENT_BOOM;
	else if (!strcasecmp (token, "flash")) event[e].action[iact].action = EVENT_FLASH;
	else if (!strcasecmp (token, "moveobject")) event[e].action[iact].action = EVENT_MOVEOBJECT;
	else if (!strcasecmp (token, "moveplayer")) event[e].action[iact].action = EVENT_MOVEPLAYER;
	else if (!strcasecmp (token, "moveplanet")) event[e].action[iact].action = EVENT_MOVEPLANET;
	else if (!strcasecmp (token, "hideplanet")) event[e].action[iact].action = EVENT_HIDEPLANET;
	else if (!strcasecmp (token, "unhideplanet")) event[e].action[iact].action = EVENT_UNHIDEPLANET;
	else if (!strcasecmp (token, "betray")) event[e].action[iact].action = EVENT_BETRAY;
	else UnrecognizedToken();
}

EvValue (e)
int e;
{
	char buf[4096];

	GetRequiredToken();

	buf[0] = 0;

	if (strcmp (token, "{"))
	{
		strcpy (buf, token);
	}
	else
	{
		/* Allow text enclosed in braces */
		while (1)
		{
			GetRequiredToken();
			if (!strcmp (token, "}"))
			{
				break;
			}

			/* Add space if not first one */
			if (0 != buf) strcat (buf, " ");
			strcat (buf, token);
		}
	}
	
	/* Figure out what to do with this value */
	if (TAflag == TRIGGER)
	{
		/* It's the trigger's value */
		if (NULL == (event[e].cvalue = (char *) malloc (1+strlen(buf))))
		{
			OutOfMemory();
		}
		strcpy (event[e].cvalue, buf);
		event[e].ivalue = atoi (buf);
		event[e].fvalue = atof (buf);

		if (mission.verbose)
			Log ("EvValue: Event %d trigger value set to %s", e, buf);
	}
	else
	{
		/* It's the action's value */
		if (NULL == (event[e].action[iact].cvalue = (char *) malloc (1+strlen(buf))))
		{
			OutOfMemory();
		}
		strcpy (event[e].action[iact].cvalue, buf);
		event[e].action[iact].ivalue = atoi (buf);
		event[e].action[iact].fvalue = atof (buf);

		if (mission.verbose)
			Log ("EvValue: Event %d.%d action set to %s", e, iact, buf);
	}
}

Weapon()
/*
 *  Handle weapon token
 */
{
	weapon_index = (-1);

	GetBrace();

	while (GetToken())
	{
		if (!strcmp (token, "}"))
		{
			/* Recompute weapon ranges */
			WeaponRanges();
			return;
		}

		if (!strcasecmp (token, "index")) WeaponIndex();
		else if (!strcasecmp (token, "name")) WeaponName();
		else if (!strcasecmp (token, "speed")) WeaponSpeed();
		else if (!strcasecmp (token, "yield")) WeaponYield();
		else if (!strcasecmp (token, "idle")) WeaponIdle();
		else if (!strcasecmp (token, "expire")) WeaponExpire();
		else if (!strcasecmp (token, "renderer")) WeaponRenderer();
		else if (!strcasecmp (token, "color")) WeaponColor();
		else UnrecognizedToken();
	}
}

WeaponIndex()
{
	int w;

	GetToken();

	w = atoi (token);
	if ( (w >= 0) && (w < NWEAPONS) )
	{
		weapon_index = w;

		if (mission.verbose)
			Log ("WeaponIndex: Index set to %d", w);
	}
}

WeaponName()
{
	GetToken();

	if (weapon_index == (-1)) return;
	strcpy (weapon[weapon_index].name, token);

	if (mission.verbose)
		Log ("WeaponName: Weapon %d named %s", weapon_index, token);
}

WeaponSpeed()
{
	GetToken();

	if (weapon_index == (-1)) return;
	weapon[weapon_index].speed = atof (token) / KM_TO_UNITS1;

	if (mission.verbose)
		Log ("WeaponSpeed: Weapon %d speed is %lf", weapon_index, weapon[weapon_index].speed);
}

WeaponYield()
{
	GetToken();

	if (weapon_index == (-1)) return;
	weapon[weapon_index].yield = atof (token);

	if (mission.verbose)
		Log ("WeaponYield: Weapon %d yield is %lf", weapon_index, weapon[weapon_index].yield);
}

WeaponIdle()
{
	GetToken();

	if (weapon_index == (-1)) return;
	weapon[weapon_index].idle = atof (token);

	if (mission.verbose)
		Log ("WeaponIdle: Weapon %d idle is %lf", weapon_index, weapon[weapon_index].idle);
}

WeaponExpire()
{
	GetToken();

	if (weapon_index == (-1)) return;
	weapon[weapon_index].expire = atof (token);

	if (mission.verbose)
		Log ("WeaponExpire: Weapon %d expire is %lf", weapon_index, weapon[weapon_index].expire);
}

WeaponRenderer()
{
	int r;

	GetToken();

	if (weapon_index == (-1)) return;

	r = atoi (token);
	if ( (r >= 0) && (r < 5) )
	{
		weapon[weapon_index].renderer = r;

		if (mission.verbose)
			Log ("WeaponRenderer: Weapon %d renderer set to %d", weapon_index, r);
	}
	else
	{
		Log ("WeaponRenderer: Renderer out of range: %d", r);
	}
}

WeaponColor()
{
	int c, r, g, b;

	GetToken();

	if (weapon_index == (-1)) return;

	sscanf (token, "%i", &c);

	r = (0xff0000 & c) >> 16;
	g = (0x00ff00 & c) >> 8;
	b = (0x0000ff & c);
	weapon[weapon_index].color[0] = ((float) r) / 256.0;
	weapon[weapon_index].color[1] = ((float) g) / 256.0;
	weapon[weapon_index].color[2] = ((float) b) / 256.0;
}

Planet()
/*
 *  Handle the planet token
 */
{
	planet_index = (-1);

	GetBrace();

	while (GetToken())
	{
		if (!strcmp (token, "}")) return;

		if (!strcasecmp (token, "name")) PlanetName();
		else if (!strcasecmp (token, "newname")) PlanetNewname();
		else if (!strcasecmp (token, "reposition")) PlanetReposition();
		else if (!strcasecmp (token, "hidden")) PlanetHidden();
		else if (!strcasecmp (token, "map")) PlanetMap();
		else if (!strcasecmp (token, "oblicity")) PlanetOblicity();
		else if (!strcasecmp (token, "radius")) PlanetRadius();
		else UnrecognizedToken();
	}
}

PlanetOblicity()
{
	GetToken();
	if (planet_index == (-1)) return;

	planet[planet_index].oblicity = atof (token);

	if (mission.verbose)
		Log ("PlanetOblicity: %s oblicity is %lf",
			planet[planet_index].name, planet[planet_index].oblicity);
}

PlanetRadius()
{
	int p;

	GetToken();
	if (planet_index == (-1)) return;

	p = planet_index;

	planet[p].radius = atof (token) / KM_TO_UNITS1;
	planet[p].radius2 = planet[p].radius * planet[p].radius;
	planet[p].mass = planet[p].radius * planet[p].radius2;
	MakePlanetList (p);

	if (mission.verbose)
		Log ("PlanetRadius: %s radius is %lf",
			planet[planet_index].name, planet[planet_index].radius);
}

PlanetMap()
{
	GetToken();
	if (planet_index == (-1)) return;

	/* Set new texture */
	strcpy (planet[planet_index].texfname, token);
	planet[planet_index].custom = 1;

	/* Define new texture */
	ReadPlanetTexture (planet_index);

	if (mission.verbose)
		Log ("PlanetMap: %s texture map is %s",
			planet[planet_index].name, planet[planet_index].texfname);
}

PlanetHidden()
{
	if (planet_index == (-1)) return;
	planet[planet_index].hidden = 1;

	if (mission.verbose)
		Log ("PlanetHidden: %s is hidden", planet[planet_index].name);
}

PlanetReposition()
{
	if (planet_index == (-1)) return;
	Vset (planet[planet_index].pos, mission.cursor);

	if (mission.verbose)
		Log ("PlanetReposition: %s new position is (%lf,%lf,%lf)",
			planet[planet_index].name,
			mission.cursor[0], mission.cursor[1], mission.cursor[2]);
}

PlanetName()
{
	int p;

	GetToken();

	if ((-1) == (p = FindPlanetByName(token)))
	{
		Log ("PlanetName: No such planet: %s", token);
		return;
	}

	planet_index = p;

	if (mission.verbose)
		Log ("PlanetName: Modifying planet %s (index %d)", token, p);
}

PlanetNewname()
{
	GetToken();
	if (planet_index == (-1)) return;

	strcpy (planet[planet_index].name, token);

	if (mission.verbose)
		Log ("PlanetNewname: Planet %d renamed to %s", planet_index, token);
}

Include()
/*
 *  Open a new include file
 */
{
	char fn[128];

	GetRequiredToken();

	Log ("Include: Including file %s", token);

	/* Construct file name */
	sprintf (fn, "missions/%s", token);

	if (!PushFD())
	{
		Log ("Include: Can't include file %s", fn);
		return;
	}

	if (NULL == (fd[0] = fopen (fn, "rt")))
	{
		Log ("Include: Can't open include file %s", fn);
		PopFD();
		return;
	}
}

int PushFD()
/*
 *  Push a file descriptor onto the stack
 */
{
	int i;

	sp++;

	/* Overflow? */
	if (sp >= NFDS)
	{
		sp--;
		Log ("PushFD: Include files nested too deeply");
		return 0;
	}

	/* Push stack */
	for (i=sp; i>0; i--) fd[i] = fd[i-1];

	return 1;
}

int PopFD()
/*
 *  Pop and FD off the stack
 */
{
	int i;

	if (sp == 0) return 0;

	sp--;
	for (i=0; i<=sp; i++) fd[i] = fd[i+1];

	return 1;
}

#ifdef WIN32
int strcasecmp (s1, s2)
char *s1, *s2;
{
	int l1, l2, i;
	char c1, c2;

	l1 = strlen (s1);
	l2 = strlen (s2);

	if (l1 != l2) return (1);
	if (l1 == 0) return (0);

	for (i=0; i<l1; i++)
	{
		c1 = tolower (s1[i]);
		c2 = tolower (s2[i]);

		if (c1 != c2)
		{
			return (1);
		}
	}

	return (0);
}

int strncasecmp (s1, s2, n)
char *s1, *s2;
int n;
{
	int i;
	char c1, c2;

	for (i=0; i<n; i++)
	{
		c1 = tolower (s1[i]);
		c2 = tolower (s2[i]);

		if (c1 != c2)
		{
			return (1);
		}
	}

	return (0);
}
#endif

void DoLoad (void)
/*
 *  Sparky has typed a mission to load
 */
{
	ReadMission (text.buf);
}

