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

#ifndef WIN32
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

/*
 *  All sorts of stuff a client needs
 */

int FindClient()
/*
 *  Find an unused client
 */
{
	int c;

	for (c=0; c<NCLIENTS; c++)
	{
		if (!client[c].active)
		{
			client[c].active = 1;
			return c;
		}
	}

	/* Ran out */
	return (-1);
}

DoClient()
/*
 *  Handle ORBIT client duties
 */
{
	char buf[1024];
	int r, e;

	/* Bump timers */
	clientme.timer.server += deltaT;
	clientme.timer.pos += deltaT;

	/* If server has been idle too long, goodbye */
	if (clientme.timer.server > MAXSERVERIDLE)
	{
		Mprint ("Server is not responding");
		Log ("DoClient: Dropping idle server");
#ifndef WIN32
		close (clientme.socket);
#else
		closesocket (clientme.socket);
#endif
		am_client = 0;
		return;
	}

	/* Time to report position? */
	if (clientme.timer.pos >= CLIENTPOSINTERVAL)
	{
		ReportPosition();
		clientme.timer.pos = 0.0;
	}

	/* Try to read from the server */
	r = recv (clientme.socket, buf, 1024, 0);

	/* If recv returns zero the other side has gone away */
	if (r == 0)
	{
		Mprint ("Dropped by server");
		Log ("DoClient: Dropped by server");
#ifndef WIN32
		close (clientme.socket);
#else
		closesocket (clientme.socket);
#endif
		am_client = 0;

		InitNetwork();
		InitTargets();

		return;
	}

	/* If we get an error it better be EWOULDBLOCK */
#ifndef WIN32
	if (r < 0)
	{
		if (errno == EWOULDBLOCK)
		{
			/* All is well, no data from server */
			return;
		}
		else
		{
			/* Uh oh */
			Log ("DoClient: recv() error: %d", errno);
			return;
		}
	}
#else
	if (r == SOCKET_ERROR)
	{
		e = WSAGetLastError();
		if (e == WSAEWOULDBLOCK)
		{
			/* All is well, no data from server */
			return;
		}
		else
		{
			/* Uh oh */
			Log ("DoClient: recv() error: %d", e);
			return;
		}
	}
#endif

	/* We really have data from the server! */
	clientme.timer.server = 0.0;

	/* Handle the data */
	buf[r] = 0;
	recv_bytes += r;
/*	Log ("DoClient: Server data: %s", buf);	*/

	ServerData (buf, r);
}

ServerData (buf, n)
int n;
char *buf;
/*
 *  Separate incoming data into packets
 */
{
	int i;
#ifndef BINARYPACKETS
	int ch, j;
#endif

#ifdef BINARYPACKETS
	for (i=0; i<n; i++) ServerByte (buf[i]);

#else
	j = clientme.ptr;

	for (i=0; i<n; i++)
	{
		ch = clientme.pkt[j] = buf[i];
		j = (j + 1) % 1024;

		/* Got a packet? */
		if (ch == 0)
		{
			ServerPacket (clientme.pkt);
			j = 0;
		}
	}

	clientme.ptr = j;
#endif
}

ServerByte (c)
char c;
/*
 *  Process one byte from server
 */
{
	switch (clientme.state)
	{
	case NETSTATE_MAGIC:
		if (c == NET_MAGIC)
		{
			clientme.state = NETSTATE_SIZE;
		}
		break;

	case NETSTATE_SIZE:
		clientme.remain = 0xff & c;
		clientme.ptr = 0;
		clientme.state = NETSTATE_PACKET;
		if (clientme.remain == 0) clientme.state = NETSTATE_MAGIC;
		break;

	case NETSTATE_PACKET:
		clientme.pkt[clientme.ptr++] = c;
		clientme.remain--;
		if (clientme.remain == 0)
		{
			clientme.pkt[clientme.ptr] = 0;
			ServerPacket (clientme.pkt);
			clientme.state = NETSTATE_MAGIC;
		}
		break;

	default:
		Log ("ServerByte: PANIC! No such state: %d", clientme.state);
		break;
	}
}

ServerPacket (pkt)
char *pkt;
/*
 *  Handle a packet from the server
 */
{
	char cmd[128];

	/* If high bit of first byte is set, this is a binary packet */
	if (0x80 & pkt[0])
	{
		ServerBinaryPacket (pkt);
		return;
	}

	/* Sanity check on packet */
	if ( (strlen(pkt) == 0) ||
	     (pkt[0] == ' ') ||
	     (pkt[0] == '\r') ||
	     (pkt[0] == '\n') ||
	     (pkt[0] == '\t') )
	{
		Log ("ServerPacket: Insane packet: %s", pkt);
		return;
	}

	/* Extract command part */
	sscanf (pkt, "%s", cmd);

	/* Dispatch */
	if (!strcasecmp (cmd, "poss"))
	{
		ServerPositionShort (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "posl"))
	{
		ServerPositionLong (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "ping"))
	{
		ServerPing (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "fire"))
	{
		ServerFire (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "mhit"))
	{
		ServerMHit (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "mdie"))
	{
		ServerMDie (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "mesg"))
	{
		ServerMessage (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "name"))
	{
		ServerName (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "vcnt"))
	{
		ServerVacant (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "modl"))
	{
		ServerModel (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "flag"))
	{
		ServerFlag (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "crat"))
	{
		ServerCrater (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "cmsg"))
	{
		ServerCMSG (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "helo"))
	{
		ServerHelo (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "gbye"))
	{
		ServerGbye (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "welc"))
	{
		ServerWelcome (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "plan"))
	{
		ServerPlanet (pkt);
		return;
	}
	else if (!strcasecmp (cmd, "rpln"))
	{
		ServerResetPlanets (pkt);
		return;
	}
	else
	{
		Log ("ServerPacket: Unrecongized command: %s", cmd);
		return;
	}
}

ServerPing (pkt)
char *pkt;
/*
 *  Handle ping packet from server
 */
{
	/* Just send it right back */
	SendASCIIPacket (clientme.socket, pkt);
}

ServerBinaryPing (pkt)
char *pkt;
/*
 *  Handle binary ping packet
 */
{
	double t;

	/* Decode */
	DecodeBinaryPacket (pkt, "F", &t);

	/* Echo */
	SendBinaryPacket (clientme.socket, "cF", PKT_PING, t);
}

ServerMessage (pkt)
char *pkt;
/*
 *  Text message from server
 */
{
	/* Print it to the screen */
	if ( (pkt[4] != 0) && (pkt[5] != 0) )
	{
		Mprint (&pkt[5]);
	}
}

ServerWelcome (pkt)
char *pkt;
/*
 *  Server says hello and gives us our client number
 */
{
	int i, t;

	/* Extract client number */
	if (1 != sscanf (pkt, "%*s %d", &i)) return;

	/* Set up our client */
	Log ("ServerWelcome: I am client %d", i);
	clientme.client = i;

	/* Set up client and target */
	client[i].active = 1;
	client[i].is_me = 1;
	client[i].frags = 0;
	t = client[i].target = InitClientTarget();
	strcpy (target[t].name, player.name);
	target[t].hidden = target[t].invisible = 1;
}

ReportPosition()
/*
 *  Report my position
 */
{
#ifdef BINARYPACKETS
	int c;
#else
	char cmd[5];
#endif

	/* Not if we're dead */
	if ( (state == STATE_DEAD1) || (state == STATE_DEAD2) ) return;

	/* Urgent? */
	if (clientme.urgent)
	{
#ifdef BINARYPACKETS
		c = PKT_POSU;
#else
		strcpy (cmd, "POSU");
#endif
		clientme.urgent = 0;
	}
	else
	{
#ifdef BINARYPACKETS
		c = PKT_POSN;
#else
		strcpy (cmd, "POSN");
#endif
	}

#ifdef BINARYPACKETS
	SendBinaryPacket (clientme.socket, "cVVvvffffff", c,
		player.pos, player.vel, player.view, player.up,
		player.move_up, player.move_down,
		player.move_right, player.move_left,
		player.move_pitchright, player.move_pitchleft);
#else
	SendASCIIPacket (clientme.socket,
	    "POSN %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n",
	    player.pos[0], player.pos[1], player.pos[2],
	    player.vel[0], player.vel[1], player.vel[2],
	    player.view[0], player.view[1], player.view[2],
	    player.up[0], player.up[1], player.up[2],
		player.move_up, player.move_down,
		player.move_right, player.move_left,
		player.move_pitchright, player.move_pitchleft);
#endif
}

ServerPositionShort (pkt)
char *pkt;
/*
 *  The server is sending a client's short position report
 */
{
	int c;
	double pos[3], vel[3];

	/* Parse packet */
	if (7 != sscanf (pkt, "%*s %d %lf %lf %lf %lf %lf %lf", &c,
		&player.pos[0], &player.pos[1], &player.pos[2],
		&player.vel[0], &player.vel[1], &player.vel[2]))
	{
		Log ("ServerPositionShort: Malformed POSS packet: %s", pkt);
		return;
	}

	PositionShort (c, pos, vel);
}

PositionShort (c, pos, vel)
int c;
double pos[3], vel[3];
/*
 *  Process short position report
 */
{
	int t;

	/* Sanity */
	if ((c<0) || (c>=NCLIENTS)) return;

	/* Mark client active */
	client[c].active = 1;

	/* Is it for me? */
	if (client[c].is_me)
	{
		Vset (player.pos, pos);
		Vset (player.vel, vel);
	}
	else
	{
		/* Not for me, see if we have target for this client */
		if ((-1) == (t = client[c].target))
		{
			t = client[c].target = InitClientTarget();
		}

		/* Make target unhidden */
		target[t].hidden = 0;

		/* Set position */
		Vset (target[t].pos, pos);
		Vset (target[t].vel, vel);
	}
}

ServerPositionLong (pkt)
char *pkt;
/*
 *  The server is sending a client's long position report
 */
{
	int c;
	double pos[3], vel[3], view[3], up[3], move_up, move_down, move_left;
	double move_right, move_pitchright, move_pitchleft;

	/* Parse packet */
	if (19 != sscanf (pkt, "%*s %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
		&c,
		&pos[0], &pos[1], &pos[2], &vel[0], &vel[1], &vel[2],
		&view[0], &view[1], &view[2], &up[0], &up[1], &up[2],
		&move_up, &move_down, &move_right, &move_left,
		&move_pitchright, &move_pitchleft))
	{
		Log ("ServerPositionLong: Malformed POSL packet: %s", pkt);
		return;
	}

	PositionLong (c, pos, vel, view, up, move_up, move_down, move_right,
		move_left, move_pitchright, move_pitchleft);
}

PositionLong (c, pos, vel, view, up, move_up, move_down, move_right, move_left,
	move_pitchright, move_pitchleft)
int c;
double pos[3], vel[3], view[3], up[3];
double move_up, move_down, move_right, move_left;
double move_pitchright, move_pitchleft;
/*
 *  Process a long position report
 */
{
	int t;

	/* Sanity */
	if ((c<0) || (c>=NCLIENTS)) return;

	/* Mark client active */
	client[c].active = 1;

	/* Is it for me? */
	if (client[c].is_me)
	{
		Vset (player.pos, pos);
		Vset (player.vel, vel);
		Vset (player.view, view);
		Vset (player.up, up);
		player.move_up = move_up;
		player.move_down = move_down;
		player.move_left = move_left;
		player.move_right = move_right;
		player.move_pitchright = move_pitchright;
		player.move_pitchleft = move_pitchleft;
		Crossp (player.right, player.up, player.view);
	}
	else
	{
		/* Not for me, see if we have target for this client */
		if ((-1) == (t = client[c].target))
		{
			t = client[c].target = InitClientTarget();
		}

		/* Make target unhidden */
		target[t].hidden = 0;

		/* Set position */
		Vset (target[t].pos, pos);
		Vset (target[t].vel, vel);
		Vset (target[t].view, view);
		Vset (target[t].up, up);
		target[t].move_up = move_up;
		target[t].move_down = move_down;
		target[t].move_left = move_left;
		target[t].move_right = move_right;
		target[t].move_pitchright = move_pitchright;
		target[t].move_pitchleft = move_pitchleft;

		Normalize (target[t].up);
		Normalize (target[t].view);

		Crossp (target[t].right, target[t].up, target[t].view);
	}
}

ServerMHit (pkt)
char *pkt;
/*
 *  We were hit by a missile
 */
{
	double yield;

	/* Parse packet */
	if (1 != sscanf (pkt, "%*s %lf", &yield))
	{
		Log ("ServerMHit: Malformed MHIT packet: %s", pkt);
		return;
	}

	/* Flash screen red */
	palette_flash = 2;

	/* Damage shields */
	player.shields -= yield;
	if (player.shields < 0.0) player.shields = 0.0;
}

ServerMDie (pkt)
char *pkt;
/*
 *  Someone was killed.  Hope it wasn't me!
 */
{
	int ckiller, cvictim;
	double v[3], d;

	/* Parse packet */
	if (2 != sscanf (pkt, "%*s %d %d", &cvictim, &ckiller))
	{
		Log ("ServerMDie: Malformed MDIE packet: %s", pkt);
		return;
	}

	/* Adjust frags */
	client[ckiller].frags++;

	/* Tell me about it */
	Cprint ("%s was killed by %s",
		target[client[cvictim].target].name,
		target[client[ckiller].target].name);

	/* Was it me? */
	if (cvictim == clientme.client)
	{
		Cprint ("YOU were killed!");
		NetPlayerDies();
	}
	else
	{
		/* Make target hidden */
		target[client[cvictim].target].hidden = 1;

		/* Make a boom if close enough */
		Vsub (v, player.pos, target[client[cvictim].target].pos);
		d = Mag2 (v);
		if (d <= TARG_MAXRANGE2) Boom (target[client[cvictim].target].pos, 1.0);
	}

	CheckLock();
}

ServerCrater (pkt)
char *pkt;
/*
 *  Someone hit a planet.  Hope it wasn't me!
 */
{
	int c, p;
	double v[3], d;

	/* Parse packet */
	if (2 != sscanf (pkt, "%*s %d %d", &c, &p))
	{
		Log ("ServerMDie: Malformed CRAT packet: %s", pkt);
		return;
	}

	/* Adjust frags */
	client[c].frags--;

	/* Tell me about it */
	Cprint ("%s cratered on %s",
		target[client[c].target].name, planet[p].name);

	/* Was it me? */
	if (c == clientme.client)
	{
		NetPlayerDies();
	}
	else
	{
		/* Make target hidden */
		target[client[c].target].hidden = 1;

		/* Make a boom if close enough */
		Vsub (v, player.pos, target[client[c].target].pos);
		d = Mag2 (v);
		if (d <= TARG_MAXRANGE2) Boom (target[client[c].target].pos, 1.0);
	}

	CheckLock();
}

ServerFire (pkt)
char *pkt;
/*
 *  Someone nearby has fired a missile
 */
{
	int c, t, w;

	/* Parse packet */
	if (2 != sscanf (pkt, "%*s %d %d", &c, &w))
	{
		Log ("ServerFire: Malformed FIRE packet: %s", pkt);
		return;
	}

	/* Get target for this client */
	t = client[c].target;

	/* Fire the missile! */
	FireMissile (target[t].pos, target[t].vel, target[t].view,
		0, w, t);
}

ServerPlanet (pkt)
char *pkt;
/*
 *  Server sends the position of a planet
 */
{
	int p;
	double theta;

	/* Parse packet */
	if (2 != sscanf (pkt, "%*s %d %lf", &p, &theta))
	{
		Log ("ServerPlanet: Malformed PLAN packet: %s", pkt);
		return;
	}

	if ((p >= 0) && (p < NPLANETS)) planet[p].theta = theta;
}

ServerBinaryPlanet (pkt)
char *pkt;
/*
 *  Server sends the position of a planet in binary
 */
{
	int p;
	double theta;

	/* Parse packet */
	DecodeBinaryPacket (pkt, "cF", &p, &theta);

	/* Set planet angle */
	if ((p >= 0) && (p < NPLANETS)) planet[p].theta = theta;
}

ServerResetPlanets (pkt)
char *pkt;
/*
 *  Server done sending planet positions
 */
{
	PositionPlanets();
}

ServerName (pkt)
char *pkt;
/*
 *  Server sends us the name of an active client
 */
{
	int c, t, f;
	char buf[256];

	/* Parse packet */
	if (3 != sscanf (pkt, "%*s %d %s %d", &c, buf, &f))
	{
		Log ("ServerName: Malformed NAME packet: %s", pkt);
		return;
	}

	/* Sanity check */
	if ((c<0) || (c>=NCLIENTS)) return;

/*	Log ("ServerName: NAME %d %s %d", c, buf, f);	*/

	/* Don't bother if it's us */
	if (c == clientme.client)
	{
		client[c].frags = f;
		return;
	}

	/* Show client is active */
	client[c].active = 1;

	/* Set up a target if we didn't know about this one */
	if ((-1) == (t = client[c].target))
	{
		t = InitClientTarget();
		client[c].target = t;
	}

	/* Set name and frags */
	strcpy (target[t].name, buf);
	client[c].frags = f;
}

ServerHelo (pkt)
char *pkt;
/*
 *  A new client has joined the game
 */
{
	int c, t;
	char buf[256];

	/* Parse packet */
	if (2 != sscanf (pkt, "%*s %d %s", &c, buf))
	{
		Log ("ServerName: Malformed HELO packet: %s", pkt);
		return;
	}

	/* Sanity check */
	if ((c<0) || (c>=NCLIENTS)) return;

	/* Tell us */
	Cprint ("%s is here", buf);
	Log ("ServerHelo: HELO %d %s", c, buf);

	/* Don't bother if it's us */
	if (c == clientme.client) return;

	/* Set up a target if we didn't know about this one */
	if ((-1) == (t = client[c].target))
	{
		t = InitClientTarget();
		client[c].target = t;
	}

	/* Set name */
	strcpy (target[t].name, buf);
}

ServerVacant (pkt)
char *pkt;
/*
 *  Server tells us about an unused client slot
 */
{
	int c;

	/* Parse packet */
	if (1 != sscanf (pkt, "%*s %d", &c))
	{
		Log ("ServerVacant: Malformed VCNT packet: %s", pkt);
		return;
	}

	if ((c<=0) || (c>=NCLIENTS)) return;

	Log ("ServerVacant: VCNT %d", c);

	/* Don't bother if it's us (this should never happen!) */
	if (c == clientme.client)
	{
		Log ("ServerVacant: PANIC! Server says I am vacant!");
		return;
	}

	/* Destroy any associated target */
	if ((-1) != client[c].target) DestroyTarget (client[c].target);
	client[c].active = 0;
	client[c].target = (-1);
}

ServerGbye (pkt)
char *pkt;
/*
 *  Someone has left the game
 */
{
	int c;
	char buf[256];

	/* Parse packet */
	if (2 != sscanf (pkt, "%*s %d %s", &c, buf))
	{
		Log ("ServerVacant: Malformed GBYE packet: %s", pkt);
		return;
	}

	if ((c<=0) || (c>=NCLIENTS)) return;

	Log ("ServerGbye: GBYE %d", c);

	/* Don't bother if it's us (this should never happen!) */
	if (c == clientme.client)
	{
		Log ("ServerGbye: PANIC! Server says I am gone!");
		return;
	}

	/* Tell us */
	Cprint ("%s is gone", buf);

	/* Destroy any associated target */
	if ((-1) != client[c].target) DestroyTarget (client[c].target);
	client[c].active = 0;
	client[c].target = (-1);
}

ServerCMSG (pkt)
char *pkt;
/*
 *  Server has a console message for us
 */
{
	/* Sanity */
	if ( (pkt[4] == 0) || (pkt[5] == 0) ) return;

	/* Show it */
	Cprint ("%s", &pkt[5]);

	/* Give the sound */
	if (sound) PlayAudio (SOUND_COMM);
}

ServerFlag (pkt)
char *pkt;
/*
 *  Game flags from server
 */
{
	int f;

	/* Parse packet */
	if (1 != sscanf (pkt, "%*s %d", &f))
	{
		Log ("ServerFlag: Malformed FLAG packet: %s", pkt);
		return;
	}

	/* Set flags */
	gravity = (f & FLAG_GRAVITY) ? 1 : 0;
	player.flightmodel = (f & FLAG_FLIGHTMODEL) ? FLIGHT_ARCADE : FLIGHT_NEWTONIAN;
	fullstop = (f & FLAG_FULLSTOP) ? 1 : 0;
	realdistances = (f & FLAG_REALDISTANCES) ? 1 : 0;
	orbit = (f & FLAG_ORBIT) ? 1 : 0;
}

void DoConnect (void)
/*
 *  Sparky has finished typing the server address
 */
{
	BecomeClient (text.buf);
}

ServerModel (pkt)
char *pkt;
/*
 *  Set client model
 */
{
	char buf[128];
	int m, c;

	/* Parse packet */
	if (2 != sscanf (pkt, "%*s %d %s", &c, buf))
	{
		Log ("ServerModel: Malformed MODL packet: %s", pkt);
		return;
	}

	/* Sanity */
	if ( (c < 0) || (c >= NCLIENTS) || (!client[c].active) || (client[c].is_me) ) return;

	/* Try to load model */
	m = LoadModel (buf);

	/* Success? */
	if (m != (-1))
	{
		target[client[c].target].model = m;
		target[client[c].target].list = model[m].list;
	}
	else
	{
		Log ("ServerModel: LoadModel (%s) failed", buf);
		/* Use default model */
		m = LoadModel ("light2.tri");
		target[client[c].target].model = m;
		target[client[c].target].list = model[m].list;
	}
}

ServerBinaryPacket (pkt)
unsigned char *pkt;
/*
 *  Handle binary packet from server
 */
{
	/* Dispatch */
	switch (pkt[0])
	{
	case PKT_POSS:
		BinaryPositionShort (&pkt[1]);
		break;

	case PKT_POSL:
		BinaryPositionLong (&pkt[1]);
		break;

	case PKT_PLAN:
		ServerBinaryPlanet (&pkt[1]);
		break;

	case PKT_VCNT:
		ServerBinaryVacant (&pkt[1]);
		break;

	case PKT_PING:
		ServerBinaryPing (&pkt[1]);
		break;

	default:
		Log ("ServerBinaryPacket: Unrecognized packet type: 0x%x",
			pkt[0]);
		break;
	}
}

BinaryPositionShort (pkt)
unsigned char *pkt;
/*
 *  Binary short position report
 */
{
	int c;
	double pos[3], vel[3];

	DecodeBinaryPacket (pkt, "cVV", &c, pos, vel);
	PositionShort (c, pos, vel);
}

BinaryPositionLong (pkt)
unsigned char *pkt;
/*
 *  Binary long position report
 */
{
	int c;
	double pos[3], vel[3], view[3], up[3];
	double move_up, move_down;
	double move_right, move_left, move_pitchright, move_pitchleft;

	DecodeBinaryPacket (pkt, "cVVvvffffff", &c, pos, vel, view, up,
		&move_up, &move_down, &move_right, &move_left,
		&move_pitchright, &move_pitchleft);

	PositionLong (c, pos, vel, view, up,
		move_up, move_down, move_right, move_left,
		move_pitchright, move_pitchleft);
}

ServerBinaryVacant (pkt)
char *pkt;
/*
 *  Server tells us which clients are in use
 */
{
	int c;
	unsigned int w, bit, i1, i2;

	/* Parse packet */
	DecodeBinaryPacket (pkt, "cc", &i1, &i2);
	w = i1 + 256*i2;

	/* See which bits are set */
	bit = 1;

	for (c=0; c<NCLIENTS; c++)
	{
		if (!(w & bit))
		{
			/* Don't bother if it's us (this should never happen!) */
			if (c == clientme.client)
			{
				Log ("ServerVacant: PANIC! Server says I am vacant!");
				return;
			}

			/* Destroy any associated target */
			if ((-1) != client[c].target) DestroyTarget (client[c].target);
			client[c].active = 0;
			client[c].target = (-1);
		}

		bit = bit * 2;
	}
}

