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

#define ALLOCATE
#include "orbit.h"

int tm, frames, total_frames;

main (argc, argv)
int argc;
char *argv[];
{
	/* Kick random number generator */
	srand (time(NULL));

	/* Check for joystick */
	InitJoy();

	/* Init the performance timer */
	InitTimer();

	/* Set up the player viewpoint, etc */
	InitPlayer();

    /* Initialize all sorts of other stuff */
	InitStuff();

	/* Open a window */
	OpenWindow (argc, argv);

	frames = 0;
	total_frames = 1;
	tm = time(NULL);
	
	/* Initialize the message system */
	InitMessage();
	


	/* Loop forever */
	glutMainLoop();
}

OpenWindow (argc, argv)
int argc;
char *argv[];
/*
 *  Open a window
 */
{
	char *p;

	glutInit (&argc, argv);
	glutInitDisplayMode (GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	glutInitWindowPosition (0, 0);
	glutInitWindowSize (ScreenWidth, ScreenHeight);

	if (!strcasecmp (gamemode, "no"))
	{
		glutCreateWindow ("Orbit");
	}
	else
	{
		glutGameModeString (gamemode);
		glutEnterGameMode();
	}

	if (fullscreen) glutFullScreen ();

	glutDisplayFunc (DrawScene);
	glutReshapeFunc (Reshape);

	/* Define lights, etc */
	Lights();
	glEnable (GL_DEPTH_TEST);
	glEnable (GL_CULL_FACE);

	/* Put up some driver info */
	p = (char *) glGetString (GL_VENDOR);
	Log ("OpenWindow: OpenGL vendor: %s", p);
	p = (char *) glGetString (GL_RENDERER);
	Log ("OpenWindow: OpenGL renderer: %s", p);
	p = (char *) glGetString (GL_VERSION);
	Log ("OpenWindow: OpenGL version: %s", p);
}

CreateUniverse ()
/*
 *  And Then There Was Light
 */
{
	/* Initialize keyboard */
	InitKeyboard();

	/* Initialize the mouse */
	InitMouse();

	/* Construct the starfield display list */
	MakeStarList();

	/* Initialize planets */
	InitPlanets();

	/* Init AC3D textures */
	InitTextures();

	/* Ring stuff */
	InitRings();

	/* Init the HUD */
	InitHud();

	/* Initialize sound */
	InitSound();

	/* Initialize the network */
	InitNetwork();

	/* Okay, load a mission */
	if (mission.fn[0] != 0)
	{
		/* Load mission from prefs file */
		ReadMission (mission.fn);
	}
	else
	{
		/* Read default mission */
		ReadMission ("train01.msn");
	}
}

void DrawScene()
/*
 *  Draw the scene
 */
{
	double v[3], t;

/*	if (am_client) Log ("!!! DrawScene()");	*/

	/* Are we still initializing? */
	if (state == STATE_INIT)
	{
		/* Create everything */
		CreateUniverse();

		/* Mark new state */
		state = STATE_NORMAL;
	}

	/* Figure out how many seconds (in deltaT) have elapsed since the last
	   time we were here */
/*	if (am_client) Log ("!!! DeltaTime()");	*/
	DeltaTime();

	/* Handle the network */
/*	if (am_client) Log ("!!! DoNetwork()");	*/
	DoNetwork();

	/* Read the keyboard */
/*	if (am_client) Log ("!!! Keyboard()");	*/
	Keyboard();

	/* Handle the joystick */
/*	if (am_client) Log ("!!! Joystick()");	*/
	if (joy_available && !paused) JoyStick();

	/* Do the mouse */
/*	if (am_client) Log ("!!! DoMouse()");	*/
	if (mouse_control && !paused) DoMouse();

	/* Move the planets */
	if (orbit) MovePlanets();

	/* Move the player */
/*	if (am_client) Log ("!!! UpdatePlayer()");	*/
	if (!paused) UpdatePlayer ();

	/* Move the targets */
/*	if (am_client) Log ("!!! MoveTargets()");	*/
	if (!paused) MoveTargets ();

	/* Move the missiles */
/*	if (am_client) Log ("!!! MoveMissiles()");	*/
	if (!paused) MoveMissiles ();

	/* Process any outstanding events */
/*	if (am_client) Log ("!!! DoEvents()");	*/
	DoEvents();

	/* Clear the screen */
	if (palette_flash == 2)
	{
		/* Flash red */
		palette_flash = 0;
		glClearColor (1.0, 0.0, 0.0, 0.0);
	}
	else if (palette_flash > 0)
	{
		/* Flash white */
		palette_flash = 0;
		glClearColor (1.0, 1.0, 1.0, 0.0);
	}
	else
	{
		glClearColor (0.0, 0.0, 0.0, 0.0);
	}
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Set up viewing transformation */
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	/* Compute a point along line of sight */
	Vadd (v, player.pos, player.view);

	gluLookAt (0.0, 0.0, 0.0,
		player.view[0], player.view[1], player.view[2],
		player.up[0], player.up[1], player.up[2]);

	/* Draw the star field here */
	if (starfield) DrawStars();

	Lights();

	/* Draw the world */
/*	if (am_client) Log ("!!! DrawWorld()");	*/
	DrawWorld();

	/* Do some 2-D stuff just to prove I can */
/*	if (am_client) Log ("!!! Hud()");	*/
	Hud();

	/* Swap buffers */
	glFlush();
	glutSwapBuffers();

	/* Keep on Displayin' */
	glutPostRedisplay();

	frames++;
	total_frames++;
	if (0 == (frames % 50))
	{
		t = Time();

		fps = 50.0 / t;
		frames = 0;

		recv_bps = ((double) recv_bytes) / t;
		xmit_bps = ((double) xmit_bytes) / t;
		recv_bytes = xmit_bytes = 0;

		tm = time (NULL);
	}
}

void DrawSplash()
/*
 *  Draw minimal screen for startup splash
 */
{
	double v[3];
	int tmphud;

	Reshape (ScreenWidth, ScreenHeight);

	glClearColor (0.0, 0.0, 0.0, 0.0);
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Set up viewing transformation */
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();

	/* Compute a point along line of sight */
	Vadd (v, player.pos, player.view);

	gluLookAt (player.pos[0], player.pos[1], player.pos[2],
		v[0], v[1], v[2],
		player.up[0], player.up[1], player.up[2]);

	/* Draw the star field here */
/*	if (starfield) DrawStars();	*/

	/* Do some 2-D stuff just to prove I can */
	tmphud = drawhud;
	drawhud = 0;
	Hud();
	drawhud = tmphud;

	/* Swap buffers */
	glFlush();
	glutSwapBuffers();

	/* Keep on Displayin' */
	glutPostRedisplay();
}

void Reshape (w, h)
GLsizei w, h;
{
	double ratio;

	ScreenWidth = w;
	ScreenHeight = h;
	ratio = (double) w / (double) h;

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective (fov, ratio, clipnear, clipfar);
	glMatrixMode (GL_MODELVIEW);
	glViewport (0, 0, w, h);

	/* Recompute the HUD stuff */
	InitHud();
}

static float MaterialColor[] = {1.0, 1.0, 1.0, 1.0};
static float JunkColor[] = {0.7, 0.7, 0.7, 1.0};
static double MissileColor[] = {1.0, 1.0, 0.0, 1.0};
static float TargetColor[] = {1.0, 0.5, 0.0, 1.0};

DrawWorld ()
/*
 *  Draw the world
 */
{
	/* Draw planet */
	DrawPlanets();

	/* Draw space junk */
	if (junk)
	{
		glMaterialfv (GL_FRONT_AND_BACK, GL_DIFFUSE, JunkColor);
		Junk();
	}

	/* Draw missiles */
	DrawMissiles ();

	/* Draw targets */
	DrawTargets();

	/* Draw explosions */
	DrawBooms ();

	/* Draw planetary rings */
	DrawRings();

	/* Show names of things */
	if (show_names) ShowNames();
}

Print (font, string)
void *font;
char *string;
{
	int len, i;

	len = strlen (string);
	for (i=0; i<len; i++)
	{
		glutBitmapCharacter (font, string[i]);
	}
}

InitStuff()
/*
 *  Misc initilization
 *
 *  DON'T PUT ANY OPENGL STUFF IN HERE!  THE WINDOW HASN'T EVEN BEEN OPENED YET!!!
 */
{
	vulnerable = 1;
	joy_throttle = 0;
	deadzone = DEADZONE;
	absT = 0.0;
	starfield = 1;
	drawhud = 1;
	showfps = 0;
	gravity = 0;
	junk = 1;
	palette_flash = 0;
	sound = 1;
	show_names = 0;
	screen_shot_num = 0;
	rings = 1;
	textures = 1;
	mission.fn[0] = 0;
	paused = 0;
	mouse_control = 1;
	strcpy (gamemode, "no");
	fullscreen = 0;
	player.flightmodel = FLIGHT_NEWTONIAN;
	strcpy (player.name, "Sparky");
	strcpy (player.model, "light2.tri");
	ring_sectors = RING_SECTORS;
	stacks0 = 3;
	slices0 = 7;
	stacks1 = 6;
	slices1 = 13;
	stacks2 = 12;
	slices2 = 24;
	mouse.flipx = 0;
	mouse.flipy = 0;
	state = STATE_INIT;
	realdistances = 0;
	fov = 90.0;
	text.yes = 0;
	text.buf[0] = 0;
	text.index = 0;
	text.prompt[0] = 0;
	text.func = NULL;
	fullstop = 1;
	player.still = 1;
	draw_orbits = 0;
	orbit = 0;
	compression = 1.0;
	player.viewlock = 0.0;
	server.port = ORBIT_PORT;
	superwarp = 1;
	clipnear = 0.01;
	clipfar = 100000.0;
/*	clipnear = 0.001;
	clipfar = 100000.0; */

	/* Set initial screen size */
	ScreenWidth = SCREENWIDTH;
	ScreenHeight = SCREENHEIGHT;

	/* Initialize the log file */
	InitLog();

	Log ("InitStuff: ORBIT Version %s", VERSION);

	/* Read the preferences file */
	ReadPrefs();

	/* Read stars data file */
	ReadStars();

	/* Initialize the message console */
	InitConsole();

    /* 	Initialize the message system */
	/* InitMessage(); */
	
	
	/* Initialize the object models */
	InitModels();

	/* Initialize the missiles */
	InitMissiles();

	/* Weapons */
	InitWeapons();

	/* Initialize explosions */
	InitBooms();

	/* Initialize lights */
	InitLights();

	/* Initialize targets */
	InitTargets();

	/* Initialize events */
	InitEvents();

	/* Self-promotion */
	Cprint ("Orbit %s, by Steve Belczyk <orbit@genesis.nred.ma.us>", VERSION);
	Cprint ("http://genesis.ne.mediaone.net/orbit");
}

double Theta (v)
double v[3];
/*
 *  Compute angle vector v points to
 */
{
	double th;

	th = (double) acos ((double) v[0]);
	th = th * 180.0 / 3.14159;
	if (v[1] < 0.0) th = (-th);

	return (th);
}

#ifdef WIN32MOUSE
Mouse()
/*
 *  Process the mouse
 */
{
	GetCursorPos (&point);
	mouse_x = point.x;
	mouse_y = point.y;

	if (mouse_x > last_mouse_x) player.move_right = 1.0;
	if (mouse_x < last_mouse_x) player.move_left = 1.0;
	if (mouse_y > last_mouse_y) player.move_up = 1.0;
	if (mouse_y < last_mouse_y) player.move_down = 1.0;

	last_mouse_x = last_mouse_y = 400;

	SetCursorPos (400, 400);
}
#endif

Gravity (deltav, pos)
double deltav[3], pos[3];
/*
 *  Compute change in velocity due to planet's gravity
 */
{
	double r, rr, dv, dp[3], v[3];
	int p;

	deltav[0] = deltav[1] = deltav[2] = 0.0;

	for (p=0; p<NPLANETS; p++)
	{
		/* Distance to planet */
		Vsub (dp, planet[p].pos, pos);

		rr = Mag2 (dp);

		/* Ignore planets too far away */
		if (rr > 200.0*200.0) continue;

		/* Don't stand so close to me */
		if (rr < RMIN) rr = RMIN;

		r = sqrt (rr);
		dv = G * planet[p].mass / rr;

		Vmul (v, dp, dv);
		Vdiv (v, v, r);

		Vadd (deltav, deltav, v);
	}
}

Junk()
/*
 *  Draw space junk to give visual cues of motion
 */
{
	double x, y, z;

	glDisable (GL_LIGHTING);
	glDisable (GL_CULL_FACE);
	glColor3d (0.5, 0.5, 0.5);

	x = (double) (int) player.pos[0];
	y = (double) (int) player.pos[1];
	z = (double) (int) player.pos[2];

	PieceOfJunk (x+0.5, y+0.5, z+0.5);
	PieceOfJunk (x-0.5, y+0.5, z+0.5);
	PieceOfJunk (x-0.5, y-0.5, z+0.5);
	PieceOfJunk (x+0.5, y-0.5, z+0.5);

	PieceOfJunk (x+0.5, y+0.5, z-0.5);
	PieceOfJunk (x-0.5, y+0.5, z-0.5);
	PieceOfJunk (x-0.5, y-0.5, z-0.5);
	PieceOfJunk (x+0.5, y-0.5, z-0.5);

	glEnable (GL_CULL_FACE);
	glEnable (GL_LIGHTING);
}

PieceOfJunk (x, y, z)
double x, y, z;
/*
 *  Draw a single piece of space junk
 */
{
	double r, v1[3], v2[3];

	glPushMatrix();

	/* Figure how much to spin junk */
	r = absT - ((double)(int)absT);
	r = 360.0 * r;

	/* Draw the junk */
	v1[0] = x; v1[1] = y; v1[2] = z;
	Vsub (v2, v1, player.pos);
	glTranslated (v2[0], v2[1], v2[2]);

	glRotated (r, 0.0, 1.0, 0.0);
	glBegin (GL_POLYGON);
	glNormal3d (0.0, 0.0, 1.0);
	glVertex3d (-.003, -.003, 0.0);
	glVertex3d (.003, -.003, 0.0);
	glVertex3d (0.0, .003, 0.0);
	glEnd();

	glPopMatrix();
}

ShowNames()
/*
 *  Show the names of things
 */
{
	glDisable (GL_DEPTH_TEST);
	glDisable (GL_LIGHTING);
	ShowPlanetNames();
	ShowTargetNames();
	glEnable (GL_LIGHTING);
	glEnable (GL_DEPTH_TEST);
}

LookAt (view, up)
double view[3], up[3];
/*
 *  Set up a transformation so a model will be drawn pointing the
 *  right direction (view) and upright (up)
 */
{
	double v1[3], v2[3], cos, th, thd, up2[3];

	/*
	 *  This thing is really ugly and inefficient.  It does sqrt's,
	 *  sin's, cos's, acos's, and asin's like there's no tomorrow.
	 *
	 *  In theory we should be able to build a little matrix like
	 *  gluLookAt() does, but I could never get that to work.  I
	 *  suspect what's needed is the inverse or transpose of the
	 *  gluLookAt() matrix, but I don't understand linear algebra
	 *  well enough to know.
	 *
	 *  If you can improve this thing please let me know.
	 */

	/* Make vector along positive X axis */
	v1[0] = 1.0;
	v1[1] = 0.0;
	v1[2] = 0.0;

	/* Cross-product of X-axis and view vector is vector to
	   rotate about */
	Crossp (v2, v1, view);

	/* Check for special case of null v2 (vectors are the same) */
	if (0.0 == Mag2 (v2))
	{
		v2[1] = 1.0;
	}

	/* Get cosine of angle between X axis and view vector */
	cos = Dotp (view, v1);

	/* Normalize the vector to rotate about */
	Normalize (v2);

	/* Compute angle to rotate */
	th = acos (cos);
	thd = th * 180.0 / 3.1415926535;

	/* Now we've got the model pointed in the right direction,
	   but the up vector is probably wrong.  This is the only
	   way I could think of to do this but boy it's ugly. */

	/* Figure out new up vector in up2 */
	v1[0] = v1[1] = 0.0;
	v1[2] = 1.0;
	RotateAbout (up2, v1, v2, -th);
	Normalize (up2);

	/* Rotate */
	glRotated (thd, v2[0], v2[1], v2[2]);

	/* Cross product of current up and desired up is vector
	   to rotate about */
	Crossp (v2, up2, up);

	/* If v2 is null, we're already facing the right way */
	if (0.0 == Mag2 (v2)) return;

	Normalize (v2);
	thd = acos (Dotp (up2, up)) * 180.0 / 3.1415926535;

	/* We rotate around the (rotated) view vector or negative view
	   vector depending which way v2 points */
	if (Dotp (v2, view) > 0.0)
		glRotated (thd, 1.0, 0.0, 0.0);
	else
		glRotated (thd, -1.0, 0.0, 0.0);
}

