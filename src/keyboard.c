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

char key[256], spec[256];

static void Key (unsigned char k, int x, int y)
{
	/* Are we reading in some text? */
	if (text.yes)
	{
		if ( ((k == 127) || (k == 8)) && (text.index > 0) )		/* Backspace */
		{
			text.index--;
			text.buf[text.index] = 0;
			Mprint ("%s%s", text.prompt, text.buf);
			return;
		}
		if (k == 27)	/* Escape */
		{
			text.yes = 0;
			Mprint (" ");
			return;
		}
		else if (k == 13)	/* Return */
		{
			Mprint (" ");
			text.func();
			text.yes = 0;
			return;
		}
		else
		{
			/* Add character to buffer */
			if (text.index >= (TEXTSIZE-1))
			{
				text.func();
				text.yes = 0;
				return;
			}
			text.buf[text.index++] = k;
			text.buf[text.index] = 0;
			Mprint ("%s%s", text.prompt, text.buf);
			return;
		}
	}

	if (state == STATE_DEAD1) return;

	if (state == STATE_DEAD2)
	{
		state = STATE_NORMAL;
		Mprint (" ");
		return;
	}

	if (paused)
	{
		paused = 0;
		Mprint ("");
		if ( (state != STATE_LOADGAME) ||
		     (k < '0') ||
		     (k >= '0'+nsaves) )
		{
			if (state == STATE_LOADGAME) state = STATE_NORMAL;
			return;
		}
	}
	key[k] = 0x81;
}

static void KeyUp (unsigned char k, int x, int y)
{
	key[k] &= 0x7f;

	if (k == 'a') key['A'] &= 0x7f;
	if (k == 'z') key['Z'] &= 0x7f;
	if (k == 'A') key['a'] &= 0x7f;
	if (k == 'Z') key['z'] &= 0x7f;
}

static void Spec (int k, int x, int y)
{
	if (state == STATE_DEAD1) return;

	if (state == STATE_DEAD2)
	{
		state = STATE_NORMAL;
		return;
	}

	if (paused)
	{
		paused = 0;
		Mprint ("");
		return;
	}
	spec[k] = 0x81;
}

static void SpecUp (int k, int x, int y)
{
	spec[k] &= 0x7f;
}

int KeyState (k)
int k;
/*
 *  bit 7 will be set if key is down now
 *  bit 0 will be set if key has been pressed since last time
 *        we were asked
 */
{
	int j;

	j = key[k];

	/* Clear since-last-time bit */
	key[k] &= 0xfe;

	return (j);
}

int SpecKeyState (k)
int k;
/*
 *  bit 7 will be set if key is down now
 *  bit 0 will be set if key has been pressed since last time
 *        we were asked
 */
{
	int j;

	j = spec[k];

	/* Clear since-last-time bit */
	spec[k] &= 0xfe;

	return (j);
}

InitKeyboard()
/*
 *  Initialize the keyboard
 */
{
	int i;

	/* Clear all the key-pressed flags */
	for (i=0; i<256; i++)
	{
		key[i] = 0;
		spec[i] = 0;
	}

	/* Set up call backs */
	glutKeyboardFunc (Key);
	glutSpecialFunc (Spec);
	glutKeyboardUpFunc (KeyUp);
	glutSpecialUpFunc (SpecUp);
}

Keyboard()
/*
 *  Read the keyboard
 */
{
	int k, i;

	/* If we're waiting for Sparky to hit a key to load a game,
	   check the appropriate digit keys */
	if (state == STATE_LOADGAME)
	{
		for (k=0; k<nsaves; k++)
		{
			if (1 & KeyState ('0' + k))
				LoadGameByKey (k);
		}
	}

	if (KeyState (27))	/* Escape */
	{
		if (strcasecmp (gamemode, "no")) glutLeaveGameMode();

		/* Rewrite the preferences file before leaving */
		Log ("Escape key, exiting...");
		ShutdownNetwork();
		FinishSound();
		WritePrefs();
		CloseLog();
		exit (0);
	}

	if (KeyState ('q'))
	{
		if (strcasecmp (gamemode, "no")) glutLeaveGameMode();
		Log ("Quitting...");
		ShutdownNetwork();
		FinishSound();
		WritePrefs();
		CloseLog();
		exit (0);
	}

	if (KeyState ('Q'))
	{
		if (strcasecmp (gamemode, "no")) glutLeaveGameMode();
		Log ("Quitting. no save...");
		ShutdownNetwork();
		FinishSound();
		CloseLog();
		exit (0);
	}

	player.move_right =
		player.move_left =
		player.move_up =
		player.move_down =
		player.move_forward =
		player.move_backward =
		player.move_pitchleft =
		player.move_pitchright = 0.0;
	warpspeed = 0;
	
	if (SpecKeyState (GLUT_KEY_RIGHT))  player.move_right = 1.0;
	if (SpecKeyState (GLUT_KEY_LEFT))   player.move_left = 1.0;
	if (SpecKeyState (GLUT_KEY_UP))     player.move_up = 1.0;
	if (SpecKeyState (GLUT_KEY_DOWN))   player.move_down = 1.0;
	if (KeyState ('a'))             player.move_forward = 0.75;
	if (KeyState ('z'))             player.move_backward = 0.75;
	if (SpecKeyState (GLUT_KEY_INSERT)) player.move_pitchleft = 1.0;
	if (KeyState (127))             player.move_pitchright = 1.0;

	if (KeyState ('A'))
	{
		player.move_forward = 0.75;
		warpspeed = 1;
	}

	if (KeyState ('Z'))
	{
		player.move_backward = 0.75;
		warpspeed = 1;
	}

	if (1 & KeyState ('h'))
	{
		drawhud = !drawhud;
	}
		
	if (1 & KeyState ('s'))
	{
		switch (starfield)
		{
		case 0:	starfield = 1;
				star_list = star_list_sparse;
				Cprint ("SPARSE Starfield");
				break;

		case 1:	starfield = 2;
				star_list = star_list_dense;
				Cprint ("DENSE Starfield");
				break;

		case 2:	starfield = 0;
				Cprint ("Starfield OFF");
				break;
		}
	}

	if (1 & KeyState (' '))
	{
		if (fullstop)
		{
			player.vel[0] = player.vel[1] = player.vel[2] = 0.0;
			player.throttle = 0.0;
			if (am_client) clientme.urgent = 1;
			QueuePositionReport();
		}
	}

	if (1 & KeyState ('c'))
	{
		for (i=0; i<console.next; i++) console.age[i] = 0.0;
	}

	if (1 & KeyState ('g'))
	{
		if (!am_client)
		{
			gravity = !gravity;
			if (gravity)
			{
				Cprint ("Gravity ON");
			}
			else
			{
				Cprint ("Gravity OFF");
			}
			if (am_server) SendFlags();
		}
	}

	if (1 & KeyState ('j'))
	{
		junk = !junk;
		if (junk)
			Cprint ("Space junk ON");
		else
			Cprint ("Space junk OFF");
	}

	if (1 & KeyState ('T'))
	{
		joy_throttle = !joy_throttle;
		if (joy_throttle)
			Cprint ("Joystick throttle ON");
		else
			Cprint ("Joystick throttle OFF");
	}

	if (1 & KeyState ('e'))
	{
		sound = !sound;
		if (sound)
			Cprint ("Sound effects ON");
		else
			Cprint ("Sound effects OFF");
	}

	if (1 & KeyState ('i'))
	{
		if (!am_client && !am_server)
		{
			vulnerable = !vulnerable;
			if (vulnerable)
				Cprint ("Vulnerable");
			else
				Cprint ("Invulnerable");
		}
	}

	if (1 & KeyState ('P'))
	{
		ScreenShot();
	}

	if (1 & KeyState ('n'))
	{
		show_names = !show_names;
		if (show_names)
			Cprint ("Names ON");
		else
			Cprint ("Names OFF");
	}

	if (1 & KeyState ('m'))
	{
		if (message.age < MSG_MAXAGE)
			message.age = MSG_MAXAGE + 1.0;
		else
			message.age = 0.0;
	}

	if (1 & KeyState ('x'))
	{
		textures = !textures;
		if (textures)
			Cprint ("Textures ON");
		else
			Cprint ("Textures OFF");
	}

	if (1 & KeyState ('r'))
	{
		rings = !rings;
		if (rings)
			Cprint ("Rings ON");
		else
			Cprint ("Rings OFF");
	}

	if (1 & KeyState ('u')) LockNearest();

	if (1 & KeyState ('y')) LockNext();

	if (1 & KeyState ('Y')) LockPrev();

	if (1 & KeyState ('b')) 
	{
		if (!am_client && !am_server) Mprint (mission.briefing);
	}

	if (1 & KeyState ('w'))
	{
		player.weapon = (player.weapon + 1) % NPLAYER_WEAPONS;
	}

	if (1 & KeyState ('W'))
	{
		player.weapon = (player.weapon + NPLAYER_WEAPONS - 1) %
		                 NPLAYER_WEAPONS;
	}

	if (1 & KeyState ('1')) player.weapon = 0;
	if (1 & KeyState ('2')) player.weapon = 1;
	if (1 & KeyState ('3')) player.weapon = 2;
	if (1 & KeyState ('4')) player.weapon = 3;

	if (1 & KeyState ('p'))
	{
		if (!am_client && !am_server)
		{
			Mprint ("Paused");
			paused = 1;
		}
	}

	/* Control-P for silent pause */
	if (1 & KeyState (16))
	{
		if (!am_client && !am_server) paused = 1;
	}

	if (1 & KeyState ('M'))
	{
		mouse_control = !mouse_control;
		if (mouse_control)
		{
			Cprint ("Mouse ON");
			InitMouse();
		}
		else
		{
			Cprint ("Mouse OFF");
			glutSetCursor (GLUT_CURSOR_INHERIT);
		}
	}		

	if (KeyState ('\t')) PlayerFires();

	if (1 & KeyState ('l'))
	{
		lock.type = (lock.type + 1) % 3;
		lock.target = -1;
		LockNearest();

		switch (lock.type)
		{
		case LOCK_ENEMY:
			Cprint ("Locking ENEMIES");
			break;

		case LOCK_FRIENDLY:
			Cprint ("Locking FRIENDLIES");
			break;

		case LOCK_PLANET:
			Cprint ("Locking PLANETS");
			break;
		}
	}

	if (1 & KeyState ('f'))
	{
		if (!am_client)
		{
			player.flightmodel = (player.flightmodel + 1) % 2;

			if (player.flightmodel == FLIGHT_NEWTONIAN)
				Cprint ("NEWTONIAN flight model");
			else
				Cprint ("ARCADE flight model");

			if (am_server) SendFlags();
		}
	}

	if (1 & KeyState ('L'))
	{
		if (!am_client && !am_server) LoadGame();
	}

	if (1 & KeyState (']')) NextWaypoint();
	if (1 & KeyState ('[')) PrevWaypoint();

	if (1 & KeyState ('F'))
	{
		showfps = !showfps;

		if (showfps)
			Cprint ("Framerate ON");
		else
			Cprint ("Framerate OFF");
	}

	if (1 & KeyState ('S'))
	{
		if (am_client)
		{
			Mprint ("Alread a client");
		}
		else if (am_server)
		{
			ShutdownServer();
		}
		else
		{
			BecomeServer();
		}
	}

	if (1 & KeyState ('C'))
	{
		if (am_client)
		{
			ShutdownClient();
		}
		else if (am_server)
		{
			Mprint ("Already a server");
		}
		else
		{
			GetText ("Connect to: ", DoConnect);
		}
	}

	if (1 & KeyState ('U')) ShowClients();
	if (1 & SpecKeyState (GLUT_KEY_F1)) ShowClients();

	if (1 & KeyState ('>'))
	{
		if (fov < 180.0)
		{
			fov += 5.0;
			Reshape (ScreenWidth, ScreenHeight);
		}
		Cprint ("Field of view %3.0lf", fov);
	}

	if (1 & KeyState ('<'))
	{
		if (fov > 5.0)
		{
			fov -= 5.0;
			Reshape (ScreenWidth, ScreenHeight);
		}
		Cprint ("Field of view %3.0lf", fov);
	}

	if (1 & KeyState ('t'))
	{
		GetText ("Say: ", DoChat);
	}

	if (1 & KeyState (4))	/* control-D */
	{
		if (am_server) GetText ("Drop client: ", DoDrop);
	}

	if (1 & KeyState (12))	/* control-L */
	{
		if (!am_server && !am_client) GetText ("Load mission: ", DoLoad);
	}

	if (1 & KeyState (14))	/* control-N */
	{
		if (!am_client && !am_server)
		{
			GetText ("Player name: ", DoName);
		}
	}

	if (1 & KeyState ('o'))
	{
		draw_orbits = !draw_orbits;

		if (draw_orbits)
			Cprint ("Orbits ON");
		else
			Cprint ("Orbits OFF");
	}

	if (1 & KeyState ('O'))
	{
		if (!am_client && !am_server)
		{
			orbit = !orbit;

			if (orbit)
				Cprint ("Orbiting ON");
			else
				Cprint ("Orbiting OFF");
		}
	}

	if (1 & KeyState ('k'))
	{
		if (!am_client && !am_server)
		{
			compression *= 2.0;
			PrintCompression();
		}
	}

	if (1 & KeyState ('K'))
	{
		if (!am_client && !am_server)
		{
			compression /= 2.0;
			PrintCompression();
		}
	}

	if (1 & KeyState ('v'))
	{
		player.viewlock = !player.viewlock;

		if (player.viewlock)
			Cprint ("View LOCKED");
		else
			Cprint ("View UNLOCKED");
	}

	if (1 & KeyState ('{'))
	{
		clipnear *= 0.75;
		Cprint ("Near = %lf", clipnear);
		Reshape (ScreenWidth, ScreenHeight);
	}
	if (1 & KeyState ('}'))
	{
		clipnear *= 1.25;
		Cprint ("Near = %lf", clipnear);
		Reshape (ScreenWidth, ScreenHeight);
	}
	if (1 & KeyState ('('))
	{
		clipfar *= 0.75;
		Cprint ("Far = %lf", clipfar);
		Reshape (ScreenWidth, ScreenHeight);
	}
	if (1 & KeyState (')'))
	{
		clipfar *= 1.25;
		Cprint ("Far = %lf", clipfar);
		Reshape (ScreenWidth, ScreenHeight);
	}
}

PrintCompression()
{
	if (compression >= 1.0)
	{
		Cprint ("Compression %.0lfX", compression);
	}
	else
	{
		Cprint ("Compression %lfX", compression);
	}
}

