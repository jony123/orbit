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

/*
 *  Stuff for weapons
 */

InitWeapons()
/*
 *  Set them up
 */
{
	int i;

	strcpy (weapon[0].name, "Laser");
	weapon[0].speed = 4000.0 / KM_TO_UNITS1;
	weapon[0].yield = 20.0;
	weapon[0].idle = 0.2;
	weapon[0].expire = 1.0;
	weapon[0].renderer = 1;
	weapon[0].color[0] = 1.0;
	weapon[0].color[1] = 1.0;
	weapon[0].color[2] = 0.0;

	strcpy (weapon[1].name, "PhotonRay");
	weapon[1].speed = 5000.0 / KM_TO_UNITS1;
	weapon[1].yield = 30.0;
	weapon[1].idle = 0.3;
	weapon[1].expire = 0.75;
	weapon[1].renderer = 2;
	weapon[1].color[0] = 0.0;
	weapon[1].color[1] = 0.0;
	weapon[1].color[2] = 1.0;

	strcpy (weapon[2].name, "IonGun");
	weapon[2].speed = 4000.0 / KM_TO_UNITS1;
	weapon[2].yield = 30.0;
	weapon[2].idle = 0.5;
	weapon[2].expire = 1.5;
	weapon[2].renderer = 3;
	weapon[2].color[0] = 0.0;
	weapon[2].color[1] = 1.0;
	weapon[2].color[2] = 0.0;

	strcpy (weapon[3].name, "Disruptor");
	weapon[3].speed = 3000.0 / KM_TO_UNITS1;
	weapon[3].yield = 60.0;
	weapon[3].idle = 1.0;
	weapon[3].expire = 1.5;
	weapon[3].renderer = 4;
	weapon[3].color[0] = 1.0;
	weapon[3].color[1] = 1.0;
	weapon[3].color[2] = 0.0;

	/* Spare weapons for enemies */
	for (i=NPLAYER_WEAPONS; i<NWEAPONS; i++)
	{
		strcpy (weapon[i].name, "Spare");
		weapon[i].speed = 6000.0 / KM_TO_UNITS1;
		weapon[i].yield = 15.0;
		weapon[i].idle = 2.0;
		weapon[i].expire = 1.0;
		weapon[i].renderer = 0;
		weapon[i].color[0] = 1.0;
		weapon[i].color[1] = 0.0;
		weapon[i].color[2] = 0.0;
	}

	/* Compute weapon ranges */
	WeaponRanges();
}

WeaponRanges()
/*
 *  Compute weapon range for all weapons
 */
{
	int w;

	for (w=0; w<NWEAPONS; w++)
	{
		weapon[w].range2 = weapon[w].speed * weapon[w].expire;
		weapon[w].range2 = weapon[w].range2 * weapon[w].range2;
	}
}
