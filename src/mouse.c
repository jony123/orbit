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
 *  Handle the mouse
 */

int oldx, oldy;

static void Mouse (int x, int y)
/*
 *  Mouse motion callback
 */
{
	/* Don't bother if position hasn't changed */
	if ( (x == oldx) && (y == oldy) ) return;

	mouse.x = x;
	mouse.y = y;
}

static void MouseActive (int x, int y)
/*
 *  Mouse motion callback
 */
{
	PlayerFires();

	/* Don't bother if position hasn't changed */
	if ( (x == oldx) && (y == oldy) ) return;

	mouse.x = x;
	mouse.y = y;
}

InitMouse()
/*
 *  Initialize the mouse
 */
{
	glutPassiveMotionFunc (Mouse);
	glutMotionFunc (MouseActive);

	if (mouse_control)
	{
		glutSetCursor (GLUT_CURSOR_NONE);
		oldx = ScreenWidth / 2;
		oldy = ScreenHeight/ 2;
		glutWarpPointer (oldx, oldy);
		mouse.x = oldx;
		mouse.y = oldy;
	}
}

DoMouse()
/*
 *  Check for mouse movement
 */
{
	/* Not if we're dead */
	if ( (state == STATE_DEAD1) || (state == STATE_DEAD2) ) return;

	if (mouse.x > oldx) mouse.right = mouse.x - oldx;
	if (mouse.x < oldx) mouse.left  = oldx - mouse.x;
	if (mouse.y > oldy) mouse.up    = mouse.y - oldy;
	if (mouse.y < oldy) mouse.down  = oldy - mouse.y;

	if (mouse.flipx)
	{
		if (mouse.left ) player.move_right = 0.1 * (double) mouse.left;
		if (mouse.right) player.move_left  = 0.1 * (double) mouse.right;
	}
	else
	{
		if (mouse.left ) player.move_left  = 0.1 * (double) mouse.left;
		if (mouse.right) player.move_right = 0.1 * (double) mouse.right;
	}

	if (mouse.flipy)
	{
		if (mouse.up   ) player.move_down  = 0.1 * (double) mouse.up;
		if (mouse.down ) player.move_up    = 0.1 * (double) mouse.down;
	}
	else
	{
		if (mouse.up   ) player.move_up    = 0.1 * (double) mouse.up;
		if (mouse.down ) player.move_down  = 0.1 * (double) mouse.down;
	}

	mouse.left  = 0;
	mouse.right = 0;
	mouse.up    = 0;
	mouse.down  = 0;

	oldx = ScreenWidth / 2;
	oldy = ScreenHeight/ 2;
	glutWarpPointer (oldx, oldy);

	mouse.x = oldx;
	mouse.y = oldy;
}

