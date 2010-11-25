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
#include <netinet/tcp.h>
#endif

#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>

/*
 *  All sorts of stuff the server does
 */

DoServer()
/*
 *  I am an ORBIT server
 */
{
	int c;

	/* Look for any new clients */
	CheckForClient();

	/* Loop through clients */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active) DoThisClient (c);
	}
}

DoThisClient (c)
int c;
/*
 *  Server handles client c
 */
{
	int i;

	/* Don't bother if this client is not active */
	if (!client[c].active) return;

	/* Don't bother if this client is us */
	if (client[c].is_me) return;

	/* Bump times */
	client[c].timer.idle += deltaT;
	client[c].timer.ping += deltaT;
	for (i=0; i<NCLIENTS; i++) client[c].timer.posn[i] += deltaT;
	client[c].timer.rollcall += deltaT;

	/* If client has been idle too long, goodbye */
	if (client[c].timer.idle > MAXCLIENTIDLE)
	{
		Cprint ("Dropping idle client");
		Log ("DoThisClient: Dropping idle client %d", c);
		DropClient (c);
		return;
	}

	/* See if this client has anything to say to us */
	ReadFromClient (c);

	/* Send anything we have to send to this client */
	SendToClient (c);

}

SendToClient (c)
int c;
/*
 *  Send anything we have to send to this client
 */
{
	int cc;

	if (!client[c].active) return;

	/* Time to send ping? */
	if (client[c].timer.ping >= PINGINTERVAL)
	{
		client[c].timer.ping = 0.0;
		SendASCIIPacket (client[c].socket,
			"PING %lf\n", absT);
	}

	/* Time to send position reports? */
	for (cc=0; cc<NCLIENTS; cc++)
	{
		/* Not if it's us and we're dead */
		if ( (cc == server.client) &&
		     ((state == STATE_DEAD1) || (state == STATE_DEAD2)) ) continue;

		if (client[cc].active && (cc != c))
		{
			if (client[c].timer.posn[cc] > client[c].posninterval)
			{
				/* Report client cc's position to client c */
				SendPosition (c, cc);
				client[c].timer.posn[cc] = 0.0;
			}
		}
	}

	/* Time to send roll call? */
	if (client[c].timer.rollcall >= ROLLCALLINTERVAL) SendRollCall (c);
/**
	if (client[c].timer.posn >= client[c].posninterval)
	{
		client[c].timer.posn = 0.0;
		SendPositions (c);
	}
**/
}

ReadFromClient (c)
int c;
/*
 *  See if this client has anything to say to us
 */
{
	char buf[1024];
	int r, e;

	/* Bail if not there */
	if (!client[c].active) return;

	/* Try to read from this client */
	r = recv (client[c].socket, buf, 1024, 0);

	/* If recv returns zero the other side has gone away */
	if (r == 0)
	{
		Cprint ("Client %d went away", c);
		Log ("ReadFromClient: Client %d went away", c);
		DropClient (c);

		return;
	}

	/* If we get an error it better be EWOULDBLOCK */
#ifndef WIN32
	if (r < 0)
	{
		if (errno == EWOULDBLOCK)
		{
			/* All is well */
			return;
		}
		else
		{
			/* Uh oh */
			Log ("ReadFromClient: recv error: %d", errno);
			return;
		}
	}
#else
	if (r == SOCKET_ERROR)
	{
		e = WSAGetLastError();
		if (e == WSAEWOULDBLOCK)
		{
			/* All is well */
			return;
		}
		else
		{
			/* Uh oh */
			Log ("ReadFromClient: recv error: %d", e);
			return;
		}
	}
#endif

	/* We really have data from the client! */
	client[c].timer.idle = 0.0;

	/* Handle the data */
	buf[r] = 0;
	recv_bytes += r;
/*
	Log ("ReadFromClient: Client %d data:", c);
	LogWrite (buf, r);
*/
	/* Handle packet from Client */
	ClientData (c, buf, r);
}

ClientData (c, buf, n)
int c, n;
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
	for (i=0; i<n; i++) ClientByte (c, buf[i]);
#else
	j = client[c].ptr;

	for (i=0; i<n; i++)
	{
		ch = client[c].pkt[j] = buf[i];
		j = (j + 1) % 1024;

		/* Got a packet? */
		if (ch == 0)
		{
			ClientPacket (c, client[c].pkt);
			j = 0;
		}
	}

	client[c].ptr = j;
#endif
}

ClientByte (c, b)
int c;
char b;
/*
 *  Process one byte from server
 */
{
	switch (client[c].state)
	{
	case NETSTATE_MAGIC:
		if (b == NET_MAGIC)
		{
			client[c].state = NETSTATE_SIZE;
		}
		break;

	case NETSTATE_SIZE:
		client[c].remain = 0xff & b;
		client[c].ptr = 0;
		client[c].state = NETSTATE_PACKET;
		if (client[c].remain == 0) client[c].state = NETSTATE_MAGIC;
		break;

	case NETSTATE_PACKET:
		client[c].pkt[client[c].ptr++] = b;
		client[c].remain--;
		if (client[c].remain == 0)
		{
			client[c].pkt[client[c].ptr] = 0;
			ClientPacket (c, client[c].pkt);
			client[c].state = NETSTATE_MAGIC;
		}
		break;

	default:
		Log ("ClientByte: PANIC! No such state: %d", client[c].state);
		break;
	}
}

CheckForClient()
/*
 *  Look for a new network client
 */
{
	int c, e, i, len;
	SOCKET a;
	int one = 1;
	struct sockaddr_in sin;
#ifdef WIN32
	unsigned long longone = 1;
#endif

	len = sizeof (sin);

	/* Ask for any outstanding connections */
	a = accept (server.listening_socket, (struct sockaddr *) &sin, &len);

#ifndef WIN32
	if (a < 0)
	{
		/* If the accept failed, it better be because the socket
		   is non-blocking or something went wrong */
		if (errno == EWOULDBLOCK)
		{
			/* No problem, just no new connection */
			return;
		}
		else
		{
			/* Uh oh */
			Log ("CheckForClient: accept() error: %d", errno);
			return;
		}
	}
#else
	if (a == SOCKET_ERROR)
	{
		e = WSAGetLastError();
		if (e == WSAEWOULDBLOCK)
		{
			/* All is well, no new client */
			return;
		}
		else
		{
			/* Uh oh */
			Log ("CheckForClient: accept() error: %d", e);
			return;
		}
	}
#endif
	else
	{
		/* Got one! */
		c = FindClient();
		if (c < 0)
		{
			Cprint ("Too many clients!");
			Log ("CheckForClient: Too many clients");
#ifndef WIN32
			close (a);
#else
			closesocket (a);
#endif
		}
		else
		{
			/* Make this socket non-blocking too */
#ifdef WIN32
			ioctlsocket (a, FIONBIO, &longone);
#else
			fcntl (a, F_SETFL, O_NONBLOCK);
#endif

			/* Save client IP address */
			strcpy (client[c].ip, inet_ntoa(sin.sin_addr));

			Log ("CheckForClient: Client %d [%s] connected", c, client[c].ip);
			Cprint ("Client %d [%s] connected", c, client[c].ip);

			/* Disable the Nagle algorithm */
			setsockopt (a, IPPROTO_TCP, TCP_NODELAY, (char *) &one,
				sizeof(one));

			/* Set up new client */
			client[c].active = 1;
			client[c].socket = a;
			client[c].is_me = 0;
			client[c].ping = 0.0;
			client[c].posninterval = POSNINTERVALMEDIUM;
			client[c].frags = 0;

			client[c].target = InitClientTarget();

			client[c].timer.idle = 0.0;
			client[c].timer.ping = PINGINTERVAL;
			for (i=0; i<NCLIENTS; i++) client[c].timer.posn[i] = 0.0;
			client[c].timer.rollcall = 0.0;

			client[c].pkt[0] = 0;
			client[c].ptr = 0;

			/* Send welcome message to new client */
			SendASCIIPacket (client[c].socket, "WELC %d\n", c);
			SendASCIIPacket (client[c].socket,
			    "MESG Welcome, client %d, to the ORBIT server.\n", c);

			/* Send current roll call */
			SendRollCall (c);
		}
	}
}

ClientPacket (c, pkt)
int c;
char *pkt;
/*
 *  Handle packet from this client
 */
{
	char cmd[128];

/*	Log ("ClientPacket: %s", pkt);	*/

	/* High bit of first byte means this is a binary packet */
	if (0x80 & pkt[0])
	{
		ClientBinaryPacket (c, pkt);
		return;
	}

	/* Sanity check on packet */
	if ( (strlen(pkt) == 0) ||
	     (pkt[0] == ' ') ||
	     (pkt[0] == '\r') ||
	     (pkt[0] == '\n') ||
	     (pkt[0] == '\t') )
	{
		Log ("ClientPacket: Insane packet: %s", pkt);
		return;
	}

	/* Extract command part */
	sscanf (pkt, "%s", cmd);

	/* Dispatch */
	if (!strcasecmp (cmd, "posn"))
	{
		ClientPosition (c, pkt, 0);
		return;
	}
	else if (!strcasecmp (cmd, "posu"))
	{
		ClientPosition (c, pkt, 1);
		return;
	}
	else if (!strcasecmp (cmd, "fire"))
	{
		ClientFire (c, pkt);
		return;
	}
	else if (!strcasecmp (cmd, "ping"))
	{
		ClientPing (c, pkt);
		return;
	}
	else if (!strcasecmp (cmd, "chat"))
	{
		ClientChat (c, pkt);
		return;
	}
	else if (!strcasecmp (cmd, "name"))
	{
		ClientName (c, pkt);
		return;
	}
	else if (!strcasecmp (cmd, "modl"))
	{
		ClientModel (c, pkt);
		return;
	}
	else if (!strcasecmp (cmd, "vers"))
	{
		ClientVersion (c, pkt);
		return;
	}
	else
	{
		Log ("ClientPacket: Unrecongized command: %s", cmd);
		return;
	}
}

ClientBinaryPacket (c, pkt)
int c;
unsigned char *pkt;
/*
 *  Binary packet from client c
 */
{
	/* Dispatch */
	switch (pkt[0])
	{
	case PKT_POSN:
		ClientBinaryPosition (c, &pkt[1], 0);
		break;

	case PKT_POSU:
		ClientBinaryPosition (c, &pkt[1], 1);
		break;

	case PKT_PING:
		ClientBinaryPing (c, &pkt[1]);
		break;

	default:
		Log ("ClientBinaryPacket: Unrecognized packet type: 0x%x", pkt[0]);
		break;
	}
}

ClientPing (c, pkt)
int c;
char *pkt;
/*
 *  Handle ping packet from client
 */
{
	double t;

	/* Extract time from packet */
	if (1 == sscanf (pkt, "%*s %lf", &t))
	{
		/* Compute ping round-trip time in milliseconds */
		client[c].ping = 1000.0 * (absT - t);

		/* Use ping time to determine how fast we send position
		   reports to this client */
		if (client[c].ping <= PINGFAST)
		{
			client[c].posninterval = POSNINTERVALSMALL;
		}
		else if (client[c].ping <= PINGSLOW)
		{
			client[c].posninterval = POSNINTERVALMEDIUM;
		}
		else
		{
			client[c].posninterval = POSNINTERVALLARGE;
		}
	}
	else
	{
		Log ("ClientPing: Malformed ping packet: %s", pkt);
	}
}

ClientBinaryPing (c, pkt)
int c;
char *pkt;
/*
 *  Handle binary ping packet from client
 */
{
	double t;

	/* Extract time from packet */
	DecodeBinaryPacket (pkt, "F", &t);

	/* Compute ping round-trip time in milliseconds */
	client[c].ping = 1000.0 * (absT - t);

	/* Use ping time to determine how fast we send position
	   reports to this client */
	if (client[c].ping <= PINGFAST)
	{
		client[c].posninterval = POSNINTERVALSMALL;
	}
	else if (client[c].ping <= PINGSLOW)
	{
		client[c].posninterval = POSNINTERVALMEDIUM;
	}
	else
	{
		client[c].posninterval = POSNINTERVALLARGE;
	}
}

ShowClients ()
/*
 *  Give list of clients
 */
{
	int c;
	char buf[4096], buf2[256];

	if (!am_server && !am_client) return;

	/* Init message string */
	strcpy (buf, "Active clients:\\\\");

	/* Add info for each active client */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active)
		{
			if (am_server)
			{
				if (c == server.client)
				{
					sprintf (buf2, "%d) %s, idle %.0lf, ping %.0lf, frags %d\\",
						c, target[client[c].target].name,
						client[c].timer.idle, client[c].ping,
						client[c].frags);
				}
				else
				{
					sprintf (buf2, "%d) %s [%s], idle %.0lf, ping %.0lf, frags %d\\",
						c, target[client[c].target].name,
						client[c].ip, client[c].timer.idle, client[c].ping,
						client[c].frags);
				}
			}
			else if (am_client)
			{
				sprintf (buf2, "%d) %s, frags %d\\",
					c, target[client[c].target].name,
					client[c].frags);
			}

			strcat (buf, buf2);
		}
	}

	/* Give message */
	Mprint (buf);
}

ClientName (c, pkt)
int c;
char *pkt;
/*
 *  Set client name
 */
{
	int cc;

	if ( (pkt[4] == 0) || (pkt[5] == 0) ) return;

	strncpy (target[client[c].target].name, &pkt[5], 16);
	target[client[c].target].name[16] = 0;

	/* Tell us */
	Cprint ("%s is here", target[client[c].target].name);

	/* Tell everyone else */
	for (cc=0; cc<NCLIENTS; cc++)
	{
		if (client[cc].active && (cc != server.client) && (c != cc) )
		{
			SendASCIIPacket (client[cc].socket, "HELO %d %s",
				c, target[client[c].target].name);
		}
	}
}

ClientModel (c, pkt)
int c;
char *pkt;
/*
 *  Set client model
 */
{
	char buf[128];
	int m;

	Log ("ClientModel: %s", pkt);

	/* Parse out model name */
	if (1 != sscanf (pkt, "%*s %s", buf))
	{
		Log ("ClientModel: Malformed MODL packet: %s", pkt);
		return;
	}

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
		/* Use default model */
		m = LoadModel ("light2.tri");
		target[client[c].target].model = m;
		target[client[c].target].list = model[m].list;
	}
}

ClientPosition (c, pkt, urgent)
int c, urgent;
char *pkt;
/*
 *  Client c is telling us where it is
 */
{
	int t, cc;

	/* Get index of client's target */
	t = client[c].target;

	/* Make unhidden */
	target[t].hidden = 0;

	/* Parse packet */
	if (18 != sscanf (pkt, "%*s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
		&target[t].pos[0], &target[t].pos[1], &target[t].pos[2],
		&target[t].vel[0], &target[t].vel[1], &target[t].vel[2],
		&target[t].view[0], &target[t].view[1], &target[t].view[2],
		&target[t].up[0], &target[t].up[1], &target[t].up[2],
		&target[t].move_up, &target[t].move_down,
		&target[t].move_right, &target[t].move_left,
		&target[t].move_pitchright, &target[t].move_pitchleft))
	{
		Log ("ClientPosition: Malformed POSN packet: %s", pkt);
	}

	/* Set target's right vector */
	Crossp (target[t].right, target[t].up, target[t].view);

	/* If it's urgent, we must tell other clients */
	if (urgent)
	{
		for (cc=0; cc<NCLIENTS; cc++)
		{
			if (client[cc].active &&
			    (cc != c) &&
			    (cc != server.client) )
			{
				client[cc].timer.posn[c] = 
					client[cc].posninterval;
			}
		}
	}
}

ClientBinaryPosition (c, pkt, urgent)
int c, urgent;
unsigned char *pkt;
/*
 *  Client c is telling us where it is
 */
{
	int t, cc;

	/* Get index of client's target */
	t = client[c].target;

	/* Make unhidden */
	target[t].hidden = 0;

	/* Parse packet */
	DecodeBinaryPacket (pkt, "VVvvffffff", target[t].pos, target[t].vel,
		target[t].view, target[t].up,
		&target[t].move_up, &target[t].move_down,
		&target[t].move_right, &target[t].move_left,
		&target[t].move_pitchright, &target[t].move_pitchleft);

	Normalize (target[t].up);
	Normalize (target[t].view);

	/* Set target's right vector */
	Crossp (target[t].right, target[t].up, target[t].view);

	/* If it's urgent, we must tell other clients */
	if (urgent)
	{
		for (cc=0; cc<NCLIENTS; cc++)
		{
			if (client[cc].active &&
			    (cc != c) &&
			    (cc != server.client) )
			{
				client[cc].timer.posn[c] = 
					client[cc].posninterval;
			}
		}
	}
}

ClientVersion (c, pkt)
int c;
char *pkt;
/*
 *  Client c is telling us what version it is running
 */
{
	char buf[64];

	/* Extract version from packet */
	if (1 != sscanf (pkt, "%*s %s", buf))
	{
		Log ("ClientVersion: Malformed VERS packet: %s", pkt);
		return;
	}

	/* Check version number */
	if (strcmp (buf, VERSION))
	{
		/* Not the same, notify client */
		SendASCIIPacket (client[c].socket,
			"MESG Server is version %s, client is version %s",
			VERSION, buf);
		Log ("ClientVersion: Incompatible client version: %s", buf);
		Cprint ("WARNING:  Client %d is using incompatible version: %s",
			c, buf);
	}
}

ClientFire (c, pkt)
int c;
char *pkt;
/*
 *  Client c has sent a fire packet
 */
{
	int t, w;

	/* Get client's target */
	t = client[c].target;

	/* Parse packet */
	if (1 != sscanf (pkt, "%*s %d", &w))
	{
		Log ("ClientFire: Malformed FIRE packet: %s", pkt);
		return;
	}

	/* Fire the missile */
	FireMissile (target[t].pos, target[t].vel, target[t].view,
		0, w, t);

	/* Tell everyone about it */
	NetClientFires (c, w);
}

SendPositions (clnt)
int clnt;
/*
 *  THIS IS NOT CALLED!
 *
 *  Send a position report to client c
 */
{
	int c, t1, t2;
	double v[3], d;

	t1 = client[clnt].target;

	/* Loop through clients */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active)
		{
			/* Don't tell client about itself */
			if (c == clnt) continue;
			if (c == server.client) continue;

			t2 = client[c].target;

			/* Figure out how far away these clients are */
			Vsub (v, target[t2].pos, target[t1].pos);
			d = Mag2 (v);

			/* If far away, send short position packet */
			if (d > TARG_MAXRANGE2)
			{
				SendPositionShort (clnt, c);
			}
			else	/* Send long packet */
			{
				SendPositionLong (clnt, c);
			}
		}
	}
}

SendPosition (clnt, c)
int clnt, c;
/*
 *  Report c's position to clnt
 */
{
	int t1, t2;
	double v[3], d;

	if ((!client[clnt].active) || (!client[c].active)) return;

	t1 = client[clnt].target;
	t2 = client[c].target;

	/* Determine distance */
	Vsub (v, target[t2].pos, target[t1].pos);
	d = Mag2 (v);

	if (d > TARG_MAXRANGE2)
	{
		SendPositionShort (clnt, c);
	}
	else
	{
		SendPositionLong (clnt, c);
	}
}

SendPositionShort (clnt, c)
int clnt, c;
/*
 *  Send short position of client c to client clnt
 */
{
	int t;

	t = client[c].target;

#ifdef BINARYPACKETS
	SendBinaryPacket (client[clnt].socket, "ccVV", PKT_POSS, c,
		target[t].pos, target[t].vel);
#else
	SendASCIIPacket (client[clnt].socket,
		"POSS %d %lf %lf %lf %lf %lf %lf", c,
		target[t].pos[0], target[t].pos[1], target[t].pos[2],
		target[t].vel[0], target[t].vel[1], target[t].vel[2]);
#endif
}

SendPositionLong (clnt, c)
int clnt, c;
/*
 *  Send long position of client c to client clnt
 */
{
	int t;

	t = client[c].target;

#ifdef BINARYPACKETS
	SendBinaryPacket (client[clnt].socket, "ccVVvvffffff", PKT_POSL, c,
		target[t].pos, target[t].vel, target[t].view, target[t].up,
		target[t].move_up, target[t].move_down,
		target[t].move_right, target[t].move_left,
		target[t].move_pitchright, target[t].move_pitchleft);
#else
	SendASCIIPacket (client[clnt].socket,
		"POSL %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf", c,
		target[t].pos[0], target[t].pos[1], target[t].pos[2],
		target[t].vel[0], target[t].vel[1], target[t].vel[2],
		target[t].view[0], target[t].view[1], target[t].view[2],
		target[t].up[0], target[t].up[1], target[t].up[2],
		target[t].move_up, target[t].move_down,
		target[t].move_right, target[t].move_left,
		target[t].move_pitchright, target[t].move_pitchleft);
#endif
}

SendPlanets (c)
int c;
/*
 *  Send planet positions to client
 */
{
	int p;

	for (p=0; p<NPLANETS; p++)
	{
#ifdef BINARYPACKETS
		SendBinaryPacket (client[c].socket, "ccF", PKT_PLAN, p, planet[p].theta);
#else
		SendASCIIPacket (client[c].socket, "PLAN %d %lf", p, planet[p].theta);
#endif
	}

	SendASCIIPacket (client[c].socket, "RPLN");
}

SendRollCall (c)
/*
 *  Send roll call to client c
 */
{
	int cc, t;

	/* Reset timer */
	client[c].timer.rollcall = 0.0;

	for (cc=0; cc<NCLIENTS; cc++)
	{
		/* Send name if client active */
		if (client[cc].active)
		{
			t = client[cc].target;

			if (t >= 0)
			{
				/* Send name */
				SendASCIIPacket (client[c].socket,
					"NAME %d %s %d", cc, target[t].name, client[cc].frags);

				/* Send model */
				if (cc != server.client)
				{
					SendASCIIPacket (client[c].socket,
						"MODL %d %s", cc, model[target[t].model].name);
				}
				else
				{
					SendASCIIPacket (client[c].socket,
						"MODL %d %s", cc, player.model);
				}
			}
		}
		else
		{
#ifndef BINARYPACKETS
			/* Show unused */
			SendASCIIPacket (client[c].socket,
				"VCNT %d", cc);
#endif
		}
	}

#ifdef BINARYPACKETS
	/* Send vacant slots to client */
	SendBinaryVacant (c);
#endif

	/* Send flags while we're here */
	SendFlags (c);

	/* Oh and what the heck, let's send planet positions too */
	SendPlanets (c);
}

SendBinaryVacant (c)
/*
 *  Send binary vacancy report to client c
 */
{
	int cc;
	unsigned int w, bit, i1, i2;

	w = 0;
	bit = 1;

	/* Make bitmask */
	for (cc=0; cc<NCLIENTS; cc++)
	{
		if (client[cc].active) w += bit;
		bit = bit * 2;
	}

	/* Split into bytes */
	i1 = w % 256;
	i2 = w / 256;

	/* Send the packet */
	SendBinaryPacket (client[c].socket, "ccc", PKT_VCNT, i1, i2);
}

ClientChat (c, pkt)
int c;
char *pkt;
/*
 *  Client sent chat message
 */
{
	int cc;

	/* Sanity */
	if ( (pkt[4] == 0) || (pkt[5] == 0) || (strlen(pkt) > TEXTSIZE) ) return;

	/* Show the message to us */
	Cprint ("%s: %s", target[client[c].target].name, &pkt[5]);
	if (sound) PlayAudio (SOUND_COMM);

	/* Send to other clients */
	for (cc=0; cc<NCLIENTS; cc++)
	{
		if (client[cc].active && (cc != server.client) && (cc != c) )
		{
			SendASCIIPacket (client[cc].socket, "CMSG %s: %s",
				target[client[c].target].name, &pkt[5]);
		}
	}
}

SendFlags()
/*
 *  Send gravity, etc., flags to clients
 */
{
	int c, f;

	/* Set flags */
	f = 0;
	if (gravity) f |= FLAG_GRAVITY;
	if (player.flightmodel == FLIGHT_ARCADE) f |= FLAG_FLIGHTMODEL;
	if (fullstop) f |= FLAG_FULLSTOP;
	if (realdistances) f |= FLAG_REALDISTANCES;
	if (orbit) f |= FLAG_ORBIT;

	/* Send em */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active && (c != server.client) )
		{
			SendASCIIPacket (client[c].socket, "FLAG %d", f);
		}
	}
}

void DoDrop (void)
/*
 *  Server admin wants to drop a client
 */
{
	int c;

	if (!am_server) return;

	c = atoi (text.buf);

	/* Check and drop if okay */
	if ( (c < 0) || (c >= NCLIENTS) || (!client[c].active) ||
		client[c].is_me)
	{
		Mprint ("No such client: %d", c);
		return;
	}

	DropClient (c);
	Cprint ("Dropped client %d", c);
}
