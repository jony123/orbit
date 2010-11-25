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
 *  All sorts of stuff for network communication
 */

int endian;	/* Endian for transmitting network values */

InitNetwork()
/*
 *  Initialize network data structures
 */
{
	int c;

	Log ("InitNetwork: Initializing network");

	am_server = 0;
	am_client = 0;

	recv_bytes = xmit_bytes = 0;

	for (c=0; c<NCLIENTS; c++)
	{
		client[c].active = 0;
		client[c].is_me = 0;
		client[c].ping = 0.0;
		client[c].target = (-1);
		client[c].frags = 0;
		client[c].ip[0] = 0;

		client[c].timer.idle = 0.0;
		client[c].timer.ping = 0.0;

		client[c].state = NETSTATE_MAGIC;
	}
}

int OpenNetwork()
/*
 *  Really only need this for Winsock
 */
{
#ifdef WIN32
	WSADATA wsadata;
#endif

	Log ("OpenNetwork: Opening network");

#ifdef WIN32
	if (SOCKET_ERROR == WSAStartup (0x202, &wsadata))
	{
		Log ("OpenNetwork: WSAStartup failed with error %d",
			WSAGetLastError());
		WSACleanup();
		return 0;
	}
#endif

	return 1;
}

int CloseNetwork()
/*
 *  Close Winsock
 */
{
	Log ("CloseNetwork: Closing network");

#ifdef WIN32
	WSACleanup();
#endif
}

int BecomeServer()
/*
 *  Become an ORBIT server
 */
{
	struct sockaddr_in address;
	SOCKET listening_socket;
	int one = 1;
	int b, t, c;

	Cprint ("Becoming ORBIT server...");
	Log ("BecomeServer: Becoming ORBIT server...");

	/* Determine how to send floats */
	if (!FindEndian()) return 0;

	/* Open Winsock */
	if (!OpenNetwork()) return 0;

	/* Set address and port to listen on */
	memset ((char *) &address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons (server.port);
	address.sin_addr.s_addr = htonl (INADDR_ANY);

	/* Create the socket.  This shouldn't cause any network activity;
	   it's just creating a kernel data structure */
	Log ("BecomeServer: creating listening socket");
	listening_socket = socket (AF_INET, SOCK_STREAM, 0);
	if (listening_socket < 0)
	{
		Log ("BecomeServer: socket returned %d", listening_socket);
		CloseNetwork();
		return 0;
	}

	/* Remember listening socket so we can close it later */
	server.listening_socket = listening_socket;

	/* Set the listening socket option SO_REUSEADDR. */
	setsockopt (listening_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &one,
		sizeof(one));

	/* Bind the listening socket to the address information */
	b = bind (listening_socket, (struct sockaddr *) &address, sizeof(address));
	if (b < 0)
	{
		Log ("BecomeServer: bind() returned %d", b);
#ifndef WIN32
		close (listening_socket);
#else
		closesocket (listening_socket);
#endif
		CloseNetwork();
		return 0;
	}

	/* Set number of connections to queue */
	listen (listening_socket, 3);

	/* Put the socket in non-blocking mode so we can poll it
	   (This should really be done by getting the flags and setting
	   the one bit) */
#ifdef WIN32
	ioctlsocket (listening_socket, FIONBIO, &one);
#else
	fcntl (listening_socket, F_SETFL, O_NONBLOCK);
#endif

	/* Remember I'm a server */
	am_server = 1;

	/* Save IP address? */
	strcpy (server.ip, inet_ntoa(address.sin_addr));

	Cprint ("Now an ORBIT server, listening on port %d", server.port);
	Log ("BecomeServer: Successfully became server");

	/* Reset targets, events, weapons and planets, etc; */
	InitTargets();
	ResetEvents();
	InitWeapons();
	ResetPlanets();
	InitWaypoints();
	ResetModels();
	lock.target = (-1);
	compression = 1.0;

	/* Network players are always vulnerable */
	vulnerable = 1;

	/* Set up a client and target for me */
	c = FindClient();
	client[c].active = 1;
	client[c].is_me = 1;
	server.client = c;
	t = client[c].target = InitClientTarget();
	strcpy (target[t].name, player.name);
	target[t].hidden = target[t].invisible = 1;

	return 1;
}

int BecomeClient (s)
char *s;
/*
 *  Try to become ORBIT client
 */
{
/*	struct in_addr *addr;	*/
	SOCKET sock;
	int c, port;
	struct sockaddr_in address;
	char ip[128];

#ifdef WIN32
	unsigned long one = 1;
#else
	int one = 1;
#endif

	Log ("BecomeClient: Trying to become client");

	/* Determine how to send floats */
	if (!FindEndian()) return 0;

	/* Parse address as ip[ port] */
	port = (-1);
	sscanf (s, "%s %d", ip, &port);

	/* Init Winsock */
	if (!OpenNetwork()) return 0;

	/* Set up address, port of server */
	address.sin_family = AF_INET;
	address.sin_port = htons ((port < 0) ? ORBIT_PORT : port);
	address.sin_addr.s_addr = inet_addr (ip);

	/* Create the socket */
	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		Log ("BecomeClient: socket error: %d", errno);
		CloseNetwork();
		return 0;
	}

	/* Connect to the server */
	Log ("BecomeClient: Connecting to server %s on port %d...",
		ip, (port < 0 ? ORBIT_PORT : port));
	Cprint ("Connecting to server %s on port %d...",
		ip, (port < 0 ? ORBIT_PORT : port));
	c = connect (sock, (struct sockaddr *) &address, sizeof(address));

	/* Did it work? */
	if (c < 0)
	{
		/* No */
		Mprint ("Could not connect to server");
		Log ("BecomeClient: connect() error: %d", errno);
		CloseNetwork();
		return 0;
	}

	/* Yes!  We are connected */
	Mprint ("Connected to server");
	Log ("BecomeClient: Connected to server");

	/* Declare I am a client */
	am_client = 1;

	/* Remember this socket */
	clientme.socket = sock;

	/* Make the socket non-blocking */
#ifdef WIN32
	ioctlsocket (clientme.socket, FIONBIO, &one);
#else
	fcntl (clientme.socket, F_SETFL, O_NONBLOCK);
#endif

	/* Disable the Nagle algorithm */
	setsockopt (clientme.socket, IPPROTO_TCP, TCP_NODELAY, (char *) &one,
		sizeof(one));

	/* Init incoming packet buffer */
	clientme.pkt[0] = 0;
	clientme.ptr = 0;

	/* Init timers */
	clientme.timer.pos = 0.0;
	clientme.timer.server = 0.0;

	/* Init packet state */
	clientme.state = NETSTATE_MAGIC;

	/* Send our name and model */
	SendASCIIPacket (clientme.socket, "NAME %s\n", player.name);
	SendASCIIPacket (clientme.socket, "MODL %s\n", player.model);
	SendASCIIPacket (clientme.socket, "VERS %s\n", VERSION);

	/* Report our position */
	ReportPosition();

	/* Network players are always vulnerable */
	vulnerable = 1;

	/* Reset targets, events, weapons and planets, etc; */
	InitTargets();
	ResetEvents();
	InitWeapons();
	ResetPlanets();
	InitWaypoints();
	ResetModels();
	lock.target = (-1);
	compression = 1.0;

	return 1;
}

DoNetwork()
/*
 *  Process network activity
 */
{
	/* Are we client or server (or neither?) */
	if (am_server) DoServer();
	if (am_client) DoClient();

	return;
}

void SendASCIIPacket (SOCKET socket, char *fmt, ...)
/*
 *  Write an ASCII packet to given network socket
 */
{
	va_list ap;
	char buf[1024];
	int len;
#ifndef BINARYPACKETS
	int e;
#endif

	/* Start up varargs stuff */
	va_start (ap, fmt);

	/* Get packet into string */
	vsprintf (buf, fmt, ap);

	/* Wrap up varargs */
	va_end (ap);

	/* Add trailing null */
	buf[strlen(buf)] = 0;

	/* Send it */
	len = 1 + strlen (buf);
#ifdef BINARYPACKETS
	XmitBinaryPacket (socket, buf, len);
#else
	e = send (socket, buf, len, 0);
#endif
	xmit_bytes += len;
}

XmitBinaryPacket (socket, buf, len)
SOCKET socket;
char *buf;
int len;
{
	char pkt[512];
	int i;

	/* Length check */
	if (len > 255)
	{
		Cprint ("PANIC: Packet too large: %d", len);
		return;
	}

	/* Put in magic byte */
	pkt[0] = NET_MAGIC;

	/* Put in size */
	pkt[1] = len;

	/* Put in data */
	for (i=0; i<len; i++) pkt[2+i] = buf[i];

	/* Send it! */
	send (socket, pkt, len+2, 0);
}

ShutdownClient()
/*
 *  Undo being a client
 */
{
	if (!am_client) return;

	Log ("ShutdownClient: Shutting down network client");

	/* Close the socket to the server */
#ifndef WIN32
	close (clientme.socket);
#else
	closesocket (clientme.socket);
#endif

	/* Shut down Winsock */
	CloseNetwork();

	/* Show no longer client */
	am_client = 0;

	/* Tell Sparky */
	Mprint ("Disconnected from server");

	/* Reset network data structures */
	InitNetwork();
	InitTargets();
}

ShutdownServer()
/*
 *  Stop being a server
 */
{
	int c;

	if (!am_server) return;

	Log ("ShutdownServer: Shutting down network server");

	/* Close each active client connection */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active && (c != server.client))
		{
			Log ("ShutdownServer: Closing client %d", c);
			DropClient (c);
		}
	}

	/* Stop listening for clients */
#ifndef WIN32
	close (server.listening_socket);
#else
	closesocket (server.listening_socket);
#endif

	/* Shut down Winsock */
	CloseNetwork();

	/* No longer server */
	am_server = 0;

	/* Hey Sparky */
	Mprint ("No longer a server");

	/* Reset network data structures */
	InitNetwork();
}

ShutdownNetwork()
/*
 *  Shut down all network connections and services
 */
{
	Log ("ShutdownNetwork: Shutting down network");

	if (am_server) ShutdownServer();
	if (am_client) ShutdownClient();
}

DropClient (c)
int c;
/*
 *  Hang up on client c
 */
{
	int cc;
#ifndef WIN32
	close (client[c].socket);
#else
	closesocket (client[c].socket);
#endif

	client[c].active = 0;

	/* Notify us */
	Cprint ("%s is gone", target[client[c].target].name);

	/* Notify everyone */
	for (cc=0; cc<NCLIENTS; cc++)
	{
		if (client[cc].active && (cc != server.client) && (c != cc) )
		{
			SendASCIIPacket (client[cc].socket, "GBYE %d %s",
				c, target[client[c].target].name);
		}
	}

	/* Destroy associated target */
	DestroyTarget (client[c].target);
}

QueuePositionReport()
/*
 *  Force our position to be sent
 */
{
	int c;

	if (am_client)
	{
		clientme.timer.pos = CLIENTPOSINTERVAL;
	}
	else if (am_server)
	{
		for (c=0; c<NCLIENTS; c++)
		{
			client[c].timer.posn[server.client] = client[c].posninterval;
		}
	}
}

int InitClientTarget()
/*
 *  Set up a target for a client
 */
{
	int m, t;

	t = FindTarget();
	target[t].age = 0.1;
	strcpy (target[t].name, "Sparky");
	target[t].invisible = 0;
	target[t].hidden = 0;
	target[t].model = m = LoadModel ("light2.tri");
	target[t].list = model[m].list;
	target[t].shieldregen = SHIELD_REGEN;

	return t;
}

NetHitTarget (t, m)
int t, m;
/*
 *  Target t was hit by missile m
 */
{
	int c;

	if (!am_server) return;

	/* Find the client that belongs to this target */
	c = FindClientByTarget (t);

	/* Don't bother if it's me */
	if (c == server.client) return;

	/* Send them the packet */
	if (client[c].active)
	{
		SendASCIIPacket (client[c].socket,
			"MHIT %lf", weapon[msl[m].weapon].yield);
	}
}

NetDestroyTarget (t, m)
int t, m;
/*
 *  Target t was destroyed by missile m
 */
{
	int cvictim, ckiller;

	if (!am_server) return;

	/* Find the client that belongs to the victim */
	cvictim = FindClientByTarget (t);

	/* Find the client that belongs to the killer */
	ckiller = FindClientByTarget (msl[m].owner);

	/* Tell everyone */
	NetDestroyClient (cvictim, ckiller);
}

NetDestroyClient (cv, ck)
int cv, ck;
/*
 *  Client cv was killed by client ck
 */
{
	int c;

	/* Adjust frags */
	client[ck].frags++;

	/* Tell everyone */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active && (c != server.client) )
		{
			SendASCIIPacket (client[c].socket, "MDIE %d %d", cv, ck);
		}
	}

	/* Tell me */
	Cprint ("%s was killed by %s",
		target[client[cv].target].name,
		target[client[ck].target].name);

	/* Make client's target hidden */
	if (cv != server.client) target[client[cv].target].hidden = 1;

	CheckLock();
}

NetTargetCratered (t, p)
int t, p;
/*
 *  Target t cratered on planet p
 */
{
	int c, cc;

	/* Get client */
	cc = FindClientByTarget (t);

	/* Adjust frags */
	client[cc].frags--;

	/* Tell everyone */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active && (c != server.client) )
		{
			SendASCIIPacket (client[c].socket, "CRAT %d %d", cc, p);
		}
	}

	/* Tell me */
	Cprint ("%s cratered on %s", target[t].name, planet[p].name);

	/* Hide target */
	if (cc != server.client) target[t].hidden = 1;

	CheckLock();
}	

FindClientByTarget (t)
int t;
/*
 *  Figure out what client belongs to this target
 */
{
	int c;

	/* Is it me? */
	if (t == (-1)) return server.client;

	/* Loop through clients */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active && (t==client[c].target) ) return c;
	}

	/* When in doubt, make it me */
	return server.client;
}

NetClientFires (clnt, wep)
int clnt, wep;
/*
 *  Client clnt has fired weapon wep
 */
{
	double v[3], d;
	int c, t1, t2;

	if (!am_server) return;

	t1 = client[clnt].target;

	/* Loop through clients */
	for (c=0; c<NCLIENTS; c++)
	{
		if (client[c].active && (c != clnt) && (c != server.client))
		{
			t2 = client[c].target;

			/* Find distance between clients */
			Vsub (v, target[t1].pos, target[t2].pos);
			d = Mag2 (v);

			/* Don't bother if too far */
			if (d > TARG_MAXRANGE2) continue;

			/* Send packet */
			SendASCIIPacket (client[c].socket,
				"FIRE %d %d", clnt, wep);
		}
	}
}

NetPlayerDies()
/*
 *  We died in a network game
 */
{
	double v[3];
	int i;

	/* Change game state */
	state = STATE_DEAD1;

	/* Set up timer */
	player.dead_timer = DEAD_TIME;

	/* Stop Sparky */
	player.vel[0] = player.vel[1] = player.vel[2] = 0.0;

	/* Assign new position */
	do
	{
		for (i=0; i<3; i++) v[i] = player.pos[i] +
			rnd(1000000.0/KM_TO_UNITS1) - 500000.0/KM_TO_UNITS1;
	} while (!RespawnOkay(v));
	Vset (player.pos, v);

	/* Give Zorkian message */
	Mprint ("*** You have died ***");
}

int RespawnOkay (v)
double v[3];
/*
 *  Check if respawn location is okay
 */
{
	int p;
	double v1[3], d;

	for (p=0; p<NPLANETS; p++)
	{
		if (!planet[p].hidden)
		{
			/* Compute range to planet */
			Vsub (v1, planet[p].pos, v);
			d = Mag (v1);

			/* Too close? */
			if (d < 5.0 * planet[p].radius) return 0;
		}
	}

	/* Okay if we got here */
	return 1;
}

FindEndian()
/*
 *  Try to figure out the endianness of this machine, especially with
 *  regard to floats.  Kinda kludgy but it's been working great.
 */
{
	union
	{
		unsigned char c[4];
		float f;
	} u;

	/* Assume one way */
	endian = 0;

	/* Better have 4-byte floats or we're sunk */
	if (4 != sizeof(float))
	{
		Mprint ("PANIC! This machine doesn't use 4-byte floats");
		Log ("FindEndian: PANIC! This machine doesn't use 4-byte floats");
		return 0;
	}

	/* Set a float and pick apart the bytes to see if we
	   recognize the order */
	u.f = 3141.59;

	/* Test one way */
	if ( (u.c[0] == 0x45) && (u.c[1] == 0x44) &&
	     (u.c[2] == 0x59) && (u.c[3] == 0x71) )
	{
		/* Yay! It's MIPS, SPARC, RS/6000, HP or something
		   like that */
		endian = 0;
		return 1;
	}

	/* Hmm, test other way */
	if ( (u.c[0] == 0x71) && (u.c[1] == 0x59) &&
	     (u.c[2] == 0x44) && (u.c[3] == 0x45) )
	{
		/* Yay! Intel or DEC or something like that */
		endian = 1;
		return 1;
	}

	/* Bad news, Sparky! */
	Mprint ("Unrecognized floating point format! No network for you!");
	Log ("FindEndian: Unrecognized floating point format");
	
	return 0;
}

EncFloat (d, c)
double d;
unsigned char *c;
/*
 *  Encode a double
 */
{
	union
	{
		unsigned char c[4];
		float f;
	} u;

	/* Convert to float */
	u.f = (float) d;

	/* Write bytes */
	if (endian)
	{
		c[0] = u.c[0];
		c[1] = u.c[1];
		c[2] = u.c[2];
		c[3] = u.c[3];
	}
	else
	{
		c[0] = u.c[3];
		c[1] = u.c[2];
		c[2] = u.c[1];
		c[3] = u.c[0];
	}
}

double DecFloat (c)
unsigned char *c;
/*
 *  Decode a float
 */
{
	union
	{
		unsigned char c[4];
		float f;
	} u;

	/* Move into union */
	if (endian)
	{
		u.c[0] = c[0];
		u.c[1] = c[1];
		u.c[2] = c[2];
		u.c[3] = c[3];
	}
	else
	{
		u.c[3] = c[0];
		u.c[2] = c[1];
		u.c[1] = c[2];
		u.c[0] = c[3];
	}

	return (double) u.f;
}

EncVector (double v[3], char *c)
/*
 *  Encode a whole vector
 */
{
	EncFloat (v[0], &c[0]);
	EncFloat (v[1], &c[4]);
	EncFloat (v[2], &c[8]);
}

EncUnitVector (double v[3], char *c)
/*
 *  Encode vector of unit floats
 */
{
	c[0] = EncUnitFloat (v[0]);
	c[1] = EncUnitFloat (v[1]);
	c[2] = EncUnitFloat (v[2]);
}

DecVector (v, c)
double v[3];
unsigned char *c;
/*
 *  Decode a whole vector
 */
{
	v[0] = DecFloat (&c[0]);
	v[1] = DecFloat (&c[4]);
	v[2] = DecFloat (&c[8]);
}

int EncUnitFloat (d)
double d;
/*
 *  Convert double on [-1,1] to integer on [0,255]
 */
{
	int i;

	if (d < -1.0) d = -1.0;
	if (d >  1.0) d =  1.0;

	i = (int) ((d + 1.0) * 127.5);

	return i;
}

double DecUnitFloat (c)
unsigned char c;
/*
 *  Convert integer on [0,255] to double on [-1,1]
 */
{
	double d;
	int i;

	i = 0xff & c;
	d = (((double) i) / 127.5) - 1.0;

	return d;
}

DecUnitVector (v, c)
double v[3];
unsigned char *c;
{
	v[0] = DecUnitFloat (c[0]);
	v[1] = DecUnitFloat (c[1]);
	v[2] = DecUnitFloat (c[2]);
}

void SendBinaryPacket (SOCKET socket, char *fmt, ...)
/*
 *  Send a binary packet
 */
{
	int i, p, len;
	unsigned char buf[256];
	va_list ap;

	/* Start up varargs */
	va_start (ap, fmt);

	p = 0;
	len = strlen (fmt);

	/* Loop through format */
	for (i=0; i<len; i++)
	{
		switch (fmt[i])
		{
		case 'c':	/* Single character */
			buf[p++] = va_arg (ap, int);
			break;

		case 'F':	/* Normal float */
			EncFloat (va_arg (ap, double), &buf[p]);
			p += 4;
			break;

		case 'f':	/* Unit float */
			buf[p++] = EncUnitFloat (va_arg (ap, double));
			break;

		case 'V':	/* Normal vector */
			EncVector (va_arg (ap, double *), &buf[p]);
			p += 12;
			break;

		case 'v':	/* Unit vector */
			EncUnitVector (va_arg (ap, double *), &buf[p]);
			p += 3;
			break;

		default:
			Log ("SendBinaryPacket: Bad format character: %c", fmt[i]);
			break;
		}
	}

	va_end (ap);

	/* Send it off! */
	XmitBinaryPacket (socket, buf, p);
	xmit_bytes += 2 + p;
}

void DecodeBinaryPacket (char *pkt, char *fmt, ...)
/*
 *  Decode a binary packet
 */
{
	int i, p, len;
	va_list ap;
	double *d;

	/* Start up varargs */
	va_start (ap, fmt);

	len = strlen (fmt);
	p = 0;

	/* Loop through format */
	for (i=0; i<len; i++)
	{
		switch (fmt[i])
		{
		case 'c':	/* Single character */
			*(va_arg (ap, int *)) = pkt[p];
			p++;
			break;

		case 'F':	/* Normal float */
			*(va_arg (ap, double *)) = DecFloat (&pkt[p]);
			p += 4;
			break;

		case 'f':	/* Unit float */
			*(va_arg (ap, double *)) = DecUnitFloat (pkt[p]);
			p++;
			break;

		case 'V':	/* Normal vector */
			d = va_arg (ap, double *);
			DecVector (d, &pkt[p]);
			p += 12;
			break;

		case 'v':	/* Unit vector */
			d = va_arg (ap, double *);
			DecUnitVector (d, &pkt[p]);
			p += 3;
			break;

		default:
			Log ("DecBinaryPacket: Bad format character: %c", fmt[i]);
			break;
		}
	}

	va_end (ap);
}

