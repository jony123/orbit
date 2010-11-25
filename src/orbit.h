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

#define VERSION "1.01"

/* At one point I needed these to keep VC++ from including it own
   GL headers */
/* #define __glu_h__	*/
/* #define __gl_h_	*/

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>

#ifndef WIN32
#include <stdlib.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include </mesa/mesa-3.0/include/GL/glut.h>
#include <winsock.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#ifdef ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

#ifdef WIN32
#pragma warning(disable:4013)	/* blah() undefined; assuming extern returning int */
#pragma warning(disable:4033)	/* blah() must return a value */
#pragma warning(disable:4716)	/* blah() must return a value */
#endif

/* THETA is rate of rotation in radians per second */
#define THETA (1.6)

/* Maximum change in velocity, per second, at full throttle */
#define DELTAV (0.2)

/* Coefficient of current velocity added to acceleration under warp speed */
#define WARP_COEFF (0.2)

/* Throttle limits for arcade mode, in kps */
#define MAX_THROTTLE (10000.0 / KM_TO_UNITS1)
#define MAX_WARP_THROTTLE (1000000.0 / KM_TO_UNITS1)

#define SCREENWIDTH (640)
#define SCREENHEIGHT (480)

void DrawScene(void);
void Reshape(int, int);
void MovePlayer();
double Dist2();
double rnd();
double Time();
double Theta();
double Dotp();
double Mag();
double Mag2();
void Log (char *fmt, ...);
void Mprint (char *msg, ...);
void Cprint (char *c, ...);
void DoConnect (void);
void DoChat (void);
void DoDrop (void);
void DoLoad (void);
void DoName (void);

EXTERN	int ScreenWidth, ScreenHeight;
EXTERN	double fov;

/* Stuff to remember about the player */
#define FLIGHT_NEWTONIAN (0)
#define FLIGHT_ARCADE (1)
EXTERN	struct
{
	char name[64];		/* Player name */
	char model[128];	/* Ship model (for multiplayer) */
	double pos[3];		/* Player position */
	double up[3];		/* Camera "up" vector */
	double view[3];		/* Viewing direction */
	double right[3];	/* Right-hand vector */

	double move_forward, move_backward,
		  move_up, move_down,
		  move_pitchleft, move_pitchright,
	      move_left, move_right; /* Motion */

	int flightmodel;	/* Newtonian or arcade */

	double vel[3];		/* Player's velocity */
	double throttle;	/* Throttle for arcade mode */
	int score;			/* Player's score */

	double shields;		/* Strenth of shields */
	double maxshields;	/* Maximum shields */
	int weapon;			/* Current weapon */
	double msl_idle;	/* Weapon idle time */
	int waypoint;		/* Selected waypoint */
	double dead_timer;	/* Time to die */
	int still;			/* Not changing orientation */
	int viewlock;		/* True if view locked on something */
} player;

/* Clipping planes */
EXTERN	double clipnear;
EXTERN	double clipfar;

#define DEAD_TIME	(5.0)

EXTERN	int vulnerable;	/* If player can be killed */

EXTERN	int joy_available;	/* True if we have a joystick */
/*
 *  Joystick values:
 *
 *  x,y - stick position
 *  r   - stick twist
 *  z   - throttle position
 */
EXTERN	double joy_x, joy_xmin, joy_xmax,
	joy_y, joy_ymin, joy_ymax,
	joy_r, joy_rmin, joy_rmax,
	joy_z, joy_zmin, joy_zmax;
EXTERN	int joy_buttons;		/* State of joystick buttons */
EXTERN	int joy_throttle;		/* True to use joystick throttle */
EXTERN	double deadzone;			/* Joystick dead zone */

#define DEADZONE (0.1)	/* Joystick dead zone */

EXTERN	int mouse_x, mouse_y;	/* Stuff for mouse */
EXTERN	int last_mouse_x, last_mouse_y;

/* Timer stuff */
#define MAXDELTAT (0.1)
EXTERN	long ticks_per_sec;
EXTERN	double deltaT;
EXTERN	double absT;
EXTERN	double fps;

#ifdef WIN32
EXTERN	_int64 last_ticks;
#else
EXTERN	int last_ticks;
#endif

/* Stuff for the stars */
#define NSTARS (2000)
EXTERN	struct
{
	double x,y,z;	/* Star coords */
	double mag;		/* Star magnitude */
	double bright;	/* Point brightness */
	double bright2;	/* Brightness for sparse starfield */
} star[NSTARS];

EXTERN	int star_list;		/* Starfield display lists */
EXTERN	int star_list_sparse;
EXTERN	int star_list_dense;
EXTERN	int starfield;	/* Display starfield? */

EXTERN	int paused;		/* True if game paused */

/* Whether or not to display the HUD */
EXTERN	int drawhud;

/* Whether or not to show the frame rate */
EXTERN	int showfps;

/* Stuff for the message console */
#define CONSLINES (10)
#define CONSBUF (128)
#define CONSAGE (3.0)
#define CONSHEIGHT (10)
EXTERN	struct
{
	double age[CONSLINES];	/* How long each line has been on screen */
	int next;		/* index of next line to use */
	char buf[CONSLINES][CONSBUF];	/* messages */
} console;

/* Gravity stuff */
EXTERN	int gravity;	/* Is gravity on or off? */
#define G (0.025)
#define RMIN (2.0)

/* Allow full stop? */
EXTERN	int fullstop;	/* True if full stop allowed */

/* Regular warp or super warp engines? */
EXTERN	int superwarp;

/* Stuff for missiles */
#define NMSLS (32)
#define MSL_EXPIRE (5.0)
#define MSL_IDLE (0.2)	/* Min seconds between missiles */
#define TARGET_MSL_IDLE (2.0)	/* Same for targets */
#define MSL_MIN_AGE (0.1)	/* Time before msl can hit something */
#define MSL_VEL (0.5)
EXTERN	struct
{
	double pos[3];	/* Missile position */
	double vel[3];	/* Missile velocity */
	double age;		/* Missile age (0 == not in use) */
	int friendly;	/* True if player launced missile */
	int weapon;		/* What type of weapon this is */
	int owner;		/* Who owns this missile (-1 if player) */
} msl[NMSLS];

/* Space junk */
EXTERN	int junk;			/* True to draw space junk */

/* Stuff for explosions */
#define NBOOMS (32)
#define BOOM_TIME (1.0)	/* Length of explosion in seconds */
EXTERN	int palette_flash;	/* True to flash whole screen */
EXTERN	struct
{
	double pos[3];	/* Explosion position */
	double age;		/* Explosion age (0 == not in use) */
	int light;		/* Index of corresponding light[] entry */
	double angle;	/* Angle to rotate boom */
	double size;	/* Size of this boom */
} boom[NBOOMS];

#define X .525731112119133606
#define Z .850650808352039932

#ifdef ALLOCATE
double icos_data[12][3] =
{
  {-X, 0, Z}, {X, 0, Z}, {-X, 0, -Z}, {X, 0, -Z}, {0, Z, X}, {0, Z, -X},
  {0, -Z, X}, {0, -Z, -X}, {Z, X, 0}, {-Z, X, 0}, {Z, -X, 0}, {-Z, -X, 0}
};
#else
extern double icos_data[12][3];
#endif

#ifdef ALLOCATE
int icos_index[20][3] =
{
  {0, 4, 1}, {0, 9, 4}, {9, 5, 4}, {4, 5, 8}, {4, 8, 1},
  {8, 10, 1}, {8, 3, 10}, {5, 3, 8}, {5, 2, 3}, {2, 7, 3},
  {7, 10, 3}, {7, 6, 10}, {7, 11, 6}, {11, 0, 6}, {0, 1, 6},
  {6, 1, 10}, {9, 0, 11}, {9, 11, 2}, {9, 2, 5}, {7, 2, 11}
};
#else
extern int icos_index[20][3];
#endif

EXTERN	double boom_data[12][3];

#ifdef ALLOCATE
double boom_color[12][4] =
{
  {0.75, 0.75, 0.0, 1.0}, {0.75, 0.75, 0.0, 1.0}, {0.75, 0.75, 0.0, 1.0},
  {0.75, 0.75, 0.0, 1.0}, {0.75, 0.75, 0.0, 1.0}, {0.75, 0.75, 0.0, 1.0},
  {0.75, 0.75, 0.0, 1.0}, {0.75, 0.75, 0.0, 1.0}, {0.75, 0.75, 0.0, 1.0},
  {0.75, 0.75, 0.0, 1.0}, {0.75, 0.75, 0.0, 1.0}, {0.75, 0.75, 0.0, 1.0}
};
#else
extern double boom_color[12][4];
#endif

/* Sound stuff */
EXTERN	int sound;		/* True for sound effects */

/* Constants */
enum sounds {
    SOUND_FIRE,
    SOUND_BOOM,
    SOUND_COMM,
    NSOUNDS
};

/* Functions */
int InitSound (void);
int PlayAudio (enum sounds);
void FinishSound (void);

EXTERN	int show_names;	/* True to show names of things */

/* Lights stuff */
#define NLIGHTS GL_MAX_LIGHTS

EXTERN	struct
{
	float pos[4];	/* Light position */
	float color[3];	/* Light color */
	double age;		/* Age of light (0 == not in use) */
	int gl_num;		/* GL number of light */
} light[NLIGHTS];

/* Target stuff */
#define NTARGETS (32)
EXTERN	struct
{
	double pos[3];		/* Target position */
	double vel[3];		/* Target velocity */
	double view[3];		/* Viewing direction */
	double up[3];		/* Up vector */
	double right[3];	/* Right vector */
	double age;			/* Target age (0 == unused) */
	double range2;		/* Distance from player */
	double move_forward, move_backward,
		  move_up, move_down,
		  move_pitchleft, move_pitchright,
	      move_left, move_right; /* Motion */
	double msl_idle;	/* How long laucher has been idle */
	char name[32];		/* Target name */
	int list;			/* Target display list */
	int score;			/* Points for destroying this target */
	int model;			/* Index of target's model */
	int strategy;		/* This target's strategy for ThinkTarget() */
	int hidden;			/* True if hidden */
	int invisible;			/* True if invisible */
	int friendly;		/* True if on our side */
	double shields;		/* Shield strength */
	double maxshields;	/* Maximum shields */
	double shieldregen;	/* How fast shields regenerate */
	double turnrate;	/* How fast it can turn */
	double maxvel;		/* How fast it can accelerate */
	int weapon;			/* Current weapon */
} target[NTARGETS];

#define TARGDIST (0.02)
#define TARGDIST2 (TARGDIST * TARGDIST)
#define MINFIREDIST (1.0 / KM_TO_UNITS1)
#define MINFIREDIST2 (MINFIREDIST * MINFIREDIST)
#define MAXFIREDIST (3000.0 / KM_TO_UNITS1)
#define MAXFIREDIST2 (MAXFIREDIST * MAXFIREDIST)
#define TARG_MAXRANGE (50000.0 / KM_TO_UNITS1)
#define TARG_MAXRANGE2 (TARG_MAXRANGE * TARG_MAXRANGE)

/* Radar stuff */
EXTERN	double radarR, radarCOS, radarSIN;
EXTERN	struct
{
	int center[2];		/* Center of radar on screen, in pixels */
	double fcenter[2];	/* Floating version */
	int radius;			/* Radius of screen, in pixels */
	double fradius;		/* Floating version */
	int list;			/* Display list */
} radar;

/* Other HUD stuff */
EXTERN	struct
{
	double throt_min[2];	/* Coords of throttle display */
	double throt_mid[2];
	double throt_max[2];
	double targ_name[2];	/* Coords of target name */
	double targ_range[2];	/* Target range */
	double vel[2];			/* Player velocity */
	double shields_min[2];	/* Player shields */
	double shields_max[2];
	double weapon[2];		/* Weapon name */
	double targshields_min[2]; /* Target shields */
	double targshields_max[2];
	double waypoint[2];		/* Current waypoint */
} hud;

/* Shield stuff */
#define SHIELD_REGEN (5.0)

/* Texture stuff */
EXTERN	GLubyte planet_tex[256][256][3];	/* Planet texture */
EXTERN	int planet_list;

/* Screen shot stuff */
EXTERN	int screen_shot_num;

/* Planet stuff */
#define NPLANETS (34)
EXTERN	struct
{
	int hidden;			/* Don't draw */
	double dist;		/* Distance from primary */
	double pos[3];		/* Planet position */
	double theta;		/* Solar angle */
	double radius;		/* Planet size */
	double oblicity;	/* Oblicity (inclination from orbital plane) */
	double radius2;		/* Radius squared */
	double range2;		/* Distance^2 from player */
	double absrange2;	/* Absolute range (ignoring radius) */
	double mass;		/* Mass, for gravity calculation */
	float color[3];		/* Average color */
	int texid;			/* Texture id */
	GLubyte tex[256][256][3];	/* Texture */
	char texfname[32];	/* Name of texture file */
	char name[32];		/* Planet name */
	int is_moon;		/* True if this is a moon */
	int primary;		/* Index of primary if moon */
	int list20;			/* Display lists */
	int list80;
	int list320;
	int orbitlist;		/* Display list for orbit */
	double angvel;		/* Angular velocity in degrees per second */
	int custom;			/* True if customized */
} planet[NPLANETS];

#define ORBIT_SECTORS (360)

/* Planet detail levels */
EXTERN	int slices0, stacks0,
			slices1, stacks1,
			slices2, stacks2;

EXTERN	int textures;	/* True to draw textures */

EXTERN	int realdistances;	/* True for correct planet distances */

EXTERN	int draw_orbits;	/* True to draw orbits */
EXTERN	int orbit;			/* True to make planets orbit */
EXTERN	double compression;	/* Time compression */

EXTERN	int first_vertex;		/* For texture fixing */
EXTERN	double maxtdiff;

/* Unit conversions */
#define KM_TO_UNITS1	(6000.0)	/* Radii */
#define KM_TO_UNITS2	(6000.0 / 1000000.0)	/* Distances */

/* Model stuff */
#define NMODELS (32)
EXTERN	struct
{
	int in_use;			/* True if in use */
	char name[32];		/* Model name */
	int list;			/* Display list */
	double lobound[3];
	double hibound[3];	/* Coords of bounding box */
	double radius;
	double radius2;		/* Radius of bounding sphere */
} model[NMODELS];

/* Mission stuff */
EXTERN	struct
{
	double cursor[3];
	double player[3];
	char briefing[4096];
	char fn[128];
	int verbose;
} mission;

/* Strategies */
#define STRAT_DONOTHING	(0)
#define STRAT_SIT1 (1)
#define STRAT_SIT2 (2)
#define STRAT_SIT3 (3)
#define STRAT_SIT4 (4)
#define STRAT_HUNT1 (5)
#define STRAT_HUNT2 (6)
#define STRAT_HUNT3 (7)
#define STRAT_HUNT4 (8)

/* Message stuff (messages in center of screen) */
EXTERN	struct
{
	char text[4096];	/* Message text */
	int len;			/* Length in pixels */
	double age;			/* Age in seconds */
} message;

/* Max time to keep message on screen */
#define MSG_MAXAGE (30)

/* Rings stuff */
#define NRINGS (4)
#define MAX_RING_RANGE (200.0 * 200.0)
#define RING_SECTORS (32)
EXTERN	struct
{
	int primary;	/* Which planet */
	int list;		/* Display list */
	int texid;		/* Texture id */
	GLubyte tex[256][8][4]; /* Texture */
	double r1, r2;	/* Inner, outer radii */
	char fn[64];	/* Texture file name */
} ring[NRINGS];

EXTERN	int rings;		/* True to draw rings */
EXTERN	int ring_sectors; /* Sectors per ring */

/* Event stuff */
#define NEVENTS (32)
#define ACTIONS_PER_EVENT (64)
EXTERN	struct
{
	char name[32];	/* Event name */
	int pending;	/* True if hasn't happened yet */
	int enabled;	/* True if enabled */
	int trigger;	/* Event type */
	int ivalue;		/* Event values */
	double fvalue;
	char *cvalue;
	double pos[3];	/* Event position */

	/* Actions */
	struct
	{
		int active;
		int action;		/* Event actions */
		int ivalue;
		double fvalue;
		char *cvalue;
	} action[ACTIONS_PER_EVENT];
} event[NEVENTS];

/* Event triggers */
#define EVENT_NULL (0)
#define EVENT_APPROACH (1)
#define EVENT_DESTROY (2)
#define EVENT_SCORE (3)
#define EVENT_ALARM (4)
#define EVENT_DEPART (5)
#define EVENT_TRUE (6)
#define EVENT_STOPNEAR (7)
#define EVENT_SHIELDS (8)

/* Event actions */
#define EVENT_MESSAGE (10)
#define EVENT_HIDE (11)
#define EVENT_UNHIDE (12)
#define EVENT_ENABLE (13)
#define EVENT_DISABLE (14)
#define EVENT_LOADMISSION (15)
#define EVENT_STOP (16)
#define EVENT_BOOM (17)
#define EVENT_FLASH (18)
#define EVENT_MOVEOBJECT (19)
#define EVENT_MOVEPLAYER (20)
#define EVENT_MOVEPLANET (21)
#define EVENT_HIDEPLANET (22)
#define EVENT_UNHIDEPLANET (23)
#define EVENT_BETRAY (24)

/* Target lock stuff */
#define LOCK_ENEMY (0)
#define LOCK_FRIENDLY (1)
#define LOCK_PLANET (2)
EXTERN	struct
{
	int target;		/* Locked target, -1 if none */
	int type;		/* Target type */
} lock;

/* Weapons stuff */
#define NWEAPONS (10)
EXTERN	struct
{
	char name[32];	/* Name of weapon */
	double yield;	/* How much damage it does */
	double speed;	/* How fast it goes */
	double idle;	/* Time between shots, in seconds */
	double expire;	/* How long till it dissipates */
	int renderer;	/* How to draw */
	float color[3];	/* Color */
	double range2;	/* Range of weapon squared */
} weapon[NWEAPONS];
#define NPLAYER_WEAPONS (4)

EXTERN	int warpspeed;	/* True if using warp engines */

/* Mouse stuff */
EXTERN	struct
{
	int left, right, up, down;
	int x, y;
	int flipx;
	int flipy;
} mouse;
EXTERN	int mouse_control;

/* Video mode stuff */
EXTERN	int fullscreen;		/* To use glutFullScreen() */
EXTERN	char gamemode[64];	/* To use glutGameMode() */

/* Game state stuff */
#define STATE_INIT (1)
#define STATE_NORMAL (2)
#define STATE_DEAD1 (3)
#define STATE_DEAD2 (4)
#define STATE_LOADGAME (5)
#define STATE_GETTEXT (6)
EXTERN	int state;		/* Game state */

/* Stuff for loads and saves */
#define NSAVES (10)
EXTERN	struct
{
	char	fn[128];	/* Mission file name */
	int	time;	/* Time stamp */
} save[NSAVES];
EXTERN	int nsaves;

/* Waypoints */
#define NWAYPOINTS (32)
EXTERN	struct
{
	double pos[3];		/* Waypoint location */
} waypoint[NWAYPOINTS];
EXTERN	int	nwaypoints;	/* Number of active waypoints */

/* 
 *  Network stuff
 */

#ifndef WIN32
#define SOCKET int
#endif

/* Undef this to use ASCII packets only.  Good if the two machines
   don't use IEEE floats */
#define BINARYPACKETS

#define	ORBIT_PORT	(2061)

/* Timing stuff */
#define MAXCLIENTIDLE	(60.0)
#define MAXSERVERIDLE	(120.0)
#define PINGINTERVAL	(20.0)
#define CLIENTPOSINTERVAL	(0.33)
#define ROLLCALLINTERVAL	(30.0)

#define POSNINTERVALSMALL	(0.33)
#define POSNINTERVALMEDIUM	(0.66)
#define POSNINTERVALLARGE	(1.0)
#define PINGFAST	(100.0)
#define PINGSLOW	(400.0)

EXTERN	int	am_server;	/* True if I'm a server */
EXTERN	int	am_client;	/* True if I'm a client */

void SendASCIIPacket (SOCKET socket, char *fmt, ...);
void SendBinaryPacket (SOCKET socket, char *fmt, ...);

/* Stuff to remember about each client */
#define NCLIENTS (16)
EXTERN struct
{
	int active;	/* True if the client is active */
	int is_me;	/* True if this client is me */
	int target;	/* Index of target structure for this client */
	SOCKET socket;	/* This client's socket */
	double ping;	/* Ping time for this client */
	double posninterval;	/* How often to send position reports to this client */
	struct
	{
		double idle;	/* How long since we heard from this client */
		double ping;	/* How long since we pinged this client */
		double posn[NCLIENTS];	/* How long since we reported this client's
								position to each other client */
		double rollcall;	/* Time since we sent a roll call to this client */
	} timer;
	char pkt[1024];	/* Packet currently being received */
	int ptr;	/* Index of next byte in pkt */
	int state;	/* Binary packet state */
	int remain;	/* Bytes remaining in this binary packet */
	int frags;	/* Number of kills */
	char ip[32];	/* IP address of client */
} client[NCLIENTS];

/* Some server stuff */
EXTERN struct
{
	SOCKET listening_socket;	/* Socket server listens on */
	int client;			/* Number of server's client */
	int port;			/* Port to listen on */
	char ip[32];		/* My IP address */
} server;

/* Network game flags */
#define FLAG_GRAVITY (0x01)
#define FLAG_FLIGHTMODEL (0x02)
#define FLAG_FULLSTOP (0x04)
#define FLAG_REALDISTANCES (0x08)
#define FLAG_ORBIT (0x10)

EXTERN struct
{
	SOCKET socket;		/* Socket client uses to talk to server */
	char pkt[1024];		/* Packet being received */
	int ptr;		/* Next byte in pkt */
	int state;		/* Binary packet state */
	int remain;		/* Bytes remaining in this binary packet */
	int client;		/* Our client number */
	int urgent;		/* We have an urgent position report */
	struct
	{
		double server;	/* Time since we heard from server */
		double pos;	/* Time since we last reported position */
	} timer;
} clientme;

/* States for binary packets */
#define NETSTATE_MAGIC (1)	/* Waiting for Magic byte */
#define NETSTATE_SIZE (2)	/* Waiting for size byte */
#define NETSTATE_PACKET (3)	/* Reading packet */

#define NET_MAGIC (0x56)	/* Magic start-of-packet byte */

/* Binary packet types.  HIGH BIT MUST BE SET! */
#define PKT_POSS (0x80)		/* Short position */
#define PKT_POSL (0x81)		/* Long position */
#define PKT_POSU (0x82)		/* Urgent position */
#define PKT_POSN (0x83)		/* Regular position */
#define PKT_PLAN (0x84)		/* Planet position */
#define PKT_VCNT (0x85)		/* Vacancy report */
#define PKT_PING (0x86)		/* Ping to get latency */

EXTERN	double recv_bps, xmit_bps;	/* Receive, transmit rates */
EXTERN	int recv_bytes, xmit_bytes;	/* Bytes received/sent */

/* Stuff for reading in text */
#define TEXTSIZE (128)
EXTERN struct
{
	int yes;			/* True if typing */
	char buf[TEXTSIZE];	/* Buffer */
	int index;			/* Index into buffer */
	char prompt[32];	/* Prompt */
	void (*func)(void);		/* Function to call when done */
} text;
