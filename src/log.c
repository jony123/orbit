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

/*
 *  Stuff to maintain the log file
 */

FILE *logfd;

InitLog()
/*
 *  Open the log file
 */
{
	if (NULL == (logfd = fopen ("orbit.log", "wt"))) return;

	Log ("InitLog()");

	/* That's all! */
	return;
}

CloseLog()
/*
 *  Close up the log file
 */
{
	Log ("CloseLog()");
	fclose (logfd);
	return;
}

void Log (char *fmt, ...)
/*
 *  Write a log message
 */
{
	va_list ap;
	int tm;
	char buf[64];

	tm = time (NULL);

	va_start (ap, fmt);
	strcpy (buf, ctime((const time_t *)&tm));
	buf[strlen(buf)-1] = 0;
	fprintf (logfd, "%s: ", buf);
	vfprintf (logfd, fmt, ap);
	fprintf (logfd, "\n");
	fflush (logfd);
	va_end (ap);
}

LogWrite (buf, n)
char *buf;
int n;
/*
 *  Write raw data to logfile
 */
{
	fwrite (buf, 1, n, logfd);
	fprintf (logfd, "\n");
}


