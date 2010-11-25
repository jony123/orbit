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
#include <stdarg.h>

InitConsole()
/*
 *  Initialize the message console
 */
{
	int i;

	console.next = 0;

	for (i=0; i<CONSLINES; i++)
	{
		console.buf[i][0] = 0;
		console.age[i] = 0.0;
	}
}

void Cprint (char *c, ...)
/*
 *  Print a message to the console
 */
{
	int i;
	va_list ap;
	char buf[256];

	/* Don't bother if NULL */
	if (c == NULL) return;

	va_start (ap, c);
	vsprintf (buf, c, ap);
	va_end (ap);	

	/* Scroll messages if at end */
	if (console.next == CONSLINES)
	{
		for (i=0; i<CONSLINES-1; i++)
		{
			strcpy (console.buf[i], console.buf[i+1]);
			console.age[i] = console.age[i+1];
		}
		console.next--;
	}

	/* Add this line to buffer */
	strcpy (console.buf[console.next], buf);
	console.age[console.next] = 0.0;
	console.next++;
}

DisplayConsole()
/*
 *  Display the console messages
 */
{
	int i, y;

	/* Bump age */
	for (i=0; i<console.next; i++) console.age[i] += deltaT;

	/* Else display each line */
	y = ScreenHeight - CONSHEIGHT;
	for (i=0; i<console.next; i++)
	{
		if (console.age[i] < CONSAGE)
		{
			glColor3d (0.0, 0.8, 0.2);
			glRasterPos2i (1, y);
			Print (GLUT_BITMAP_HELVETICA_10, console.buf[i]);
			y -= CONSHEIGHT;
		}
	}
}

InitMessage()
/*
 *  Initialize the message system
 */
{
	strcpy (message.text, "No message");
	message.len = glutBitmapLength (GLUT_BITMAP_HELVETICA_10,
					message.text);
	message.age = MSG_MAXAGE + 1.0;
}

DrawMessage()
/*
 *  Draw the message on the screen
 */
{
	int rows, x, y, wrap, pixels_per_row;
	char *p;

	/* Bump age */
	if (!text.yes) message.age += deltaT;

	/* Never mind if too old */
	if (message.age > MSG_MAXAGE) return;

	wrap = 0;
	pixels_per_row = 3 * ScreenWidth / 4;

	/* Guess out how many rows this message will take */
	rows = message.len / pixels_per_row;

	/* Figure out where to start */
	y = (ScreenHeight / 2) + (CONSHEIGHT / 2) * (rows + 1);
	if (rows == 0)
	{
		/* Center if short */
		x = (ScreenWidth - message.len) / 2;
	}
	else
	{
		/* Start at left margin */
		x = ScreenWidth / 8;
	}

	/* Display the message */
	glColor3f (1.0, 0.5, 0.0);
	glRasterPos2i (x, y);
	p = message.text;
	while (*p)
	{
		/* Look for newline */
		if (*p == '\\')
		{
			wrap = 0;
			x = ScreenWidth / 8;
			y -= CONSHEIGHT;
			glRasterPos2i (x, y);
			p++;
			continue;
		}

		/* Break at space */
		if (wrap && (*p == ' '))
		{
			x = ScreenWidth / 8;
			y -= CONSHEIGHT;
			glRasterPos2i (x, y);
			wrap = 0;
		}
		glutBitmapCharacter (GLUT_BITMAP_HELVETICA_10, *p);
		x += glutBitmapWidth (GLUT_BITMAP_HELVETICA_10, *p);
		p++;
		if (x >= (pixels_per_row + ScreenWidth/8)) wrap = 1;
	}
}

void Mprint (char *msg, ...)
/*
 *  Print a message in the center of the screen
 */
{
	char buf[4096];
	int pixels_per_row;
	unsigned int i;
	va_list ap;

	if (msg == NULL) return;

	va_start (ap, msg);
	vsprintf (buf, msg, ap);
	va_end (ap);	

	pixels_per_row = 3 * ScreenWidth / 4;
	
	message.age = 0.0;
	strcpy (message.text, buf);
	message.len = glutBitmapLength (GLUT_BITMAP_HELVETICA_10,
					message.text);

	/* Fudge length upward for each newline */
	for (i=0; i<strlen(buf); i++)
	{
		/* Guess that each newline will add half a row to length */
		if (msg[i] == '\\') message.len += pixels_per_row / 2;
	}

	/* Give the sound */
	if (sound && !text.yes) PlayAudio (SOUND_COMM);
}

void DoChat (void)
/*
 *  Process chat buffer
 */
{
	int c;

	/* Show message to us */
	Mprint (" ");
	Cprint ("%s: %s", player.name, text.buf);
	Log ("DoChat: %s: %s", player.name, text.buf);

	if (am_client)
	{
		/* Send to server */
		SendASCIIPacket (clientme.socket, "CHAT %s", text.buf);
	}
	else if (am_server)
	{
		/* Send to each active client */
		for (c=0; c<NCLIENTS; c++)
		{
			if (client[c].active && (c != server.client))
			{
				SendASCIIPacket (client[c].socket, "CMSG %s: %s",
					player.name, text.buf);
			}
		}
	}
}

GetText (prompt, func)
char *prompt;
void (*func)(void);
/*
 *  Start getting text from player
 */
{
	text.yes = 1;
	text.index = 0;
	text.buf[0] = 0;
	strcpy (text.prompt, prompt);
	text.func = func;
	Mprint ("%s", text.prompt);
}

