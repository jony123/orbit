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
 *  Handle the joystick
 */
#if defined WIN32

JOYINFOEX joyInfoEx;
JOYCAPS joyCaps;

void InitJoy(void)
/*
 *  See if there's a joystick
 */
{
	ZeroMemory (&joyInfoEx, sizeof(joyInfoEx));
	joyInfoEx.dwSize = sizeof(joyInfoEx);
	joy_available = (JOYERR_NOERROR == joyGetPosEx (JOYSTICKID1, &joyInfoEx));

	/* Get joystick min-max values */
	joyGetDevCaps (JOYSTICKID1, &joyCaps, sizeof(joyCaps));
	joy_xmin = (double) joyCaps.wXmin;
	joy_xmax = (double) joyCaps.wXmax;
	joy_ymin = (double) joyCaps.wYmin;
	joy_ymax = (double) joyCaps.wYmax;
	joy_rmin = (double) joyCaps.wRmin;
	joy_rmax = (double) joyCaps.wRmax;
	joy_zmin = (double) joyCaps.wZmin;
	joy_zmax = (double) joyCaps.wZmax;
}

void JoyStick(void)
/*
 *  Process joystick
 */
{
	joyInfoEx.dwSize = sizeof (joyInfoEx);
	joyInfoEx.dwFlags = JOY_RETURNALL;
	joyGetPosEx (JOYSTICKID1, &joyInfoEx);
	joy_x = (double) joyInfoEx.dwXpos;
	joy_y = (double) joyInfoEx.dwYpos;
	joy_r = (double) joyInfoEx.dwRpos;
	joy_z = (double) joyInfoEx.dwZpos;

	joy_buttons = joyInfoEx.dwButtonNumber;

	/* See if player fired a missile */
	if (1 & joy_buttons)
	{
		PlayerFires();
	}

	/* Ignore joystick motion if dead */
	if ( (state == STATE_DEAD1) || (state == STATE_DEAD2) ) return;

	/* Convert joy values to (-1.0,1.0) */
	joy_x = (-1.0) + 2.0 * (joy_x - joy_xmin) / (joy_xmax - joy_xmin);
	joy_y = (-1.0) + 2.0 * (joy_y - joy_ymin) / (joy_ymax - joy_ymin);
	joy_r = (-1.0) + 2.0 * (joy_r - joy_rmin) / (joy_rmax - joy_rmin);
	joy_z = (-1.0) + 2.0 * (joy_z - joy_zmin) / (joy_zmax - joy_zmin);

	/* See if joystick is outside deadzone */
	if (fabs(joy_x) < deadzone) joy_x = 0.0;
	if (fabs(joy_y) < deadzone) joy_y = 0.0;
	if (fabs(joy_r) < deadzone) joy_r = 0.0;
	if (player.flightmodel == FLIGHT_NEWTONIAN) if (fabs(joy_z) < deadzone) joy_z = 0.0;

	if (2 & joy_buttons)
	{
		/* Second joy button allows player to pitch with X-axis */
		if (joy_x > 0.0) player.move_pitchright = (joy_x - deadzone) / (1.0 - deadzone);
		if (joy_x < 0.0) player.move_pitchleft = (joy_x + deadzone) / (deadzone - 1.0);
	}
	else
	{
		if (joy_x > 0.0) player.move_right = (joy_x - deadzone) / (1.0 - deadzone);
		if (joy_x < 0.0) player.move_left = (joy_x + deadzone) / (deadzone - 1.0);
	}

	if (joy_y > 0.0) player.move_up = (joy_y - deadzone) / (1.0 - deadzone);
	if (joy_y < 0.0) player.move_down = (joy_y + deadzone) / (deadzone - 1.0);
	if (joy_r > 0.0) player.move_pitchright = (joy_r - deadzone) / (1.0 - deadzone);
	if (joy_r < 0.0) player.move_pitchleft = (joy_r + deadzone) / (deadzone - 1.0);

	if (joy_throttle)
	{
		if (player.flightmodel == FLIGHT_ARCADE)
		{
			joy_z = (-joy_z + 1.0) / 2.0;
			if (joy_z < deadzone)
			{
				player.throttle = 0.0;
			}
			else
			{
				joy_z = (joy_z - deadzone) / (1.0 - deadzone);
				if (joy_z < 0.5)
					player.throttle = 2.0 * joy_z * MAX_THROTTLE;
				else
					player.throttle = (2.0 * (joy_z - 0.5)) * MAX_WARP_THROTTLE;
			}
		}
		else
		{
			if (joy_z > 0.0) player.move_backward = (joy_z - deadzone) / (1.0 - deadzone);
			if (joy_z < 0.0) player.move_forward = (joy_z + deadzone) / (deadzone - 1.0);
		}
	}

}

#elif defined __linux__ /* if defines WIN32 */

/*
#include <linux/joystick.h>

int joy_fd = -1;
char *joy_dev = "/dev/js0";
*/
void InitJoy(void)
/*
 *  Sorry, no joystick in Unix yet
 *  /usr/src/linux-2.2.10/Documentation/joystick-api.txt
 *  man js
 */
{
	int number_of_axes;
	int number_of_buttons;

	joy_available = 0;
	/*
	joy_fd = open(joy_dev, O_RDONLY | O_NONBLOCK);
	if(joy_fd < 0)
	{
		joy_available = 0;
		return;
	}
	ioctl (fd, JSIOCGAXES, &number_of_axes);
	ioctl (fd, JSIOCGBUTTONS, &number_of_buttons);
	*/
}

void JoyStick(void)
{
/*
	struct js_event joy_e;
	read (joy_fd, &joy_e, sizeof(struct js_event));
*/
}

#else /* if defined __linux__ */

void InitJoy(void)
{}
void JoyStick(void)
{}

#endif
