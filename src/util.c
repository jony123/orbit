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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void RotateAbout (vp, v, n, theta)
double vp[3], v[3], n[3], theta;
/*
 *  Rotate the vector v about the vector n by theta radians, leaving
 *  the rotated vector in vp.
 */
{
	double X, Y, Z, x, y, z, sintheta, costheta, t1, t2, t3, t4, t5;
	double t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17;
	double t18, t19, a, b, c, d, e, f, g, h, i;
	
	X = v[0];
	Y = v[1];
	Z = v[2];

	x = n[0];
	y = n[1];
	z = n[2];

	sintheta = sin (theta);
	costheta = cos (theta);

	t1 = x * x;
	t2 = y * y;
	t3 = z * z;

	t4 = 1.0 - t1;
	t5 = 1.0 - t2;
	t6 = 1.0 - t3;

	t7 = 1.0 - costheta;

	t8 = x * y;
	t9 = x * z;
	t10 = y * z;

	t11 = x * sintheta;
	t12 = y * sintheta;
	t13 = z * sintheta;

	t14 = t4 * costheta;
	t15 = t5 * costheta;
	t16 = t6 * costheta;

	t17 = t8 * t7;
	t18 = t9 * t7;
	t19 = t10 * t7;

	a = t1 + t14;
	b = t17 + t13;
	c = t18 - t12;

	d = t17 - t13;
	e = t2 + t15;
	f = t19 + t11;

	g = t18 + t12;
	h = t19 - t11;
	i = t3 + t16;

	vp[0] = X * a + Y * b + Z * c;
	vp[1] = X * d + Y * e + Z * f;
	vp[2] = X * g + Y * h + Z * i;
}

Normalize (v)
double v[3];
{
	double Mag(), vt[3];
	Vdiv (vt, v, Mag(v));
	Vset (v, vt);
}

double Dotp (a, b)
double a[3], b[3];
{
	return (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]);
}

double Mag (a)
double a[3];
{
	return ((double) sqrt ((double)(a[0]*a[0] + a[1]*a[1] + a[2]*a[2])));
}

double Mag2 (a)
double a[3];
{
	return (a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
}

Vadd (a, b, c)
double a[3], b[3], c[3];
{
	a[0] = b[0] + c[0];
	a[1] = b[1] + c[1];
	a[2] = b[2] + c[2];
}

Vsub (a, b, c)
double a[3], b[3], c[3];
{
	a[0] = b[0] - c[0];
	a[1] = b[1] - c[1];
	a[2] = b[2] - c[2];
}

Crossp (c, a, b)
double a[3], b[3], c[3];
{
	c[0] = a[1]*b[2] - a[2]*b[1];
	c[1] = a[2]*b[0] - a[0]*b[2];
	c[2] = a[0]*b[1] - a[1]*b[0];
}

Vdiv (a, b, s)
double a[3], b[3], s;
{
	a[0] = b[0] / s;
	a[1] = b[1] / s;
	a[2] = b[2] / s;
}

Vmul (a, b, s)
double a[3], b[3], s;
{
	a[0] = b[0] * s;
	a[1] = b[1] * s;
	a[2] = b[2] * s;
}

Vset (a, b)
double a[3], b[3];
{
	a[0] = b[0];
	a[1] = b[1];
	a[2] = b[2];
}

double Dist2 (x1, y1, z1, x2, y2, z2)
double x1, y1, z1, x2, y2, z2;
/*
 *  Squared distance between two points
 */
{
	double dx, dy, dz;

	dx = x1 - x2;
	dy = y1 - y2;
	dz = z1 - z2;

	return (dx*dx + dy*dy + dz*dz);
}

double rnd (r)
double r;
/*
 *  Random double on [0..r]
 */
{
	int i;
	double f;

	i = (rand() & 0xffff);
	f = r * (((double)(i)) / 65536.0);

	return (f);
}

OutOfMemory()
{
	Log ("OutOfMemory: Out of memory!  Panicking!!");
	FinishSound();
	CloseLog();
	exit (0);
}

Perp (v1, v)
double v1[3], v[3];
/*
 *  Find a vector v1 perpendicular to non-zero vector v
 *
 *  Based on the idea that the dot product of perpendicular vectors is zero
 */
{
	if ( (v[0] != 0.0) && (v[1] != 0.0) )
	{
		v1[0] = -1.0 / v[0];
		v1[1] =  1.0 / v[1];
		v1[2] = 0.0;
	}
	else if ( (v[1] != 0.0) && (v[2] != 0.0) )
	{
		v1[0] = 0.0;
		v1[1] = -1.0 / v[1];
		v1[2] =  1.0 / v[2];
	}
	else if ( (v[0] != 0.0) && (v[2] != 0.0) )
	{
		v1[0] = -1.0 / v[0];
		v1[1] = 0.0;
		v1[2] =  1.0 / v[2];
	}
	else if ( (v[0] == 0.0) && (v[1] == 0.0) && (v[2] != 0.0) )
	{
		v1[0] = 1.0;
		v1[1] = 0.0;
		v1[2] = 0.0;
	}
	else if ( (v[0] != 0.0) && (v[1] == 0.0) && (v[2] == 0.0) )
	{
		v1[0] = 0.0;
		v1[1] = 1.0;
		v1[2] = 0.0;
	}
	else if ( (v[0] == 0.0) && (v[1] != 0.0) && (v[2] == 0.0) )
	{
		v1[0] = 1.0;
		v1[1] = 0.0;
		v1[2] = 0.0;
	}
	else
	{
		/* Should never happen */
		v1[0] = 1.0;
		v1[1] = 0.0;
		v1[2] = 0.0;
	}
}

