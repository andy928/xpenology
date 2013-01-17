/* 
 *  SCSI Spinup specific attributes.
 *
 *  Copyright (c) 2009 Marvell,  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef SCSI_SPINUP_H
#define SCSI_SPINUP_H

/*
 * more defines of SCSI / ATA commands that are not defined by default
 */
#define STANDBY_IMMEDIATE     0xe0
#define SLEEP                 0xe6
#define STANDBY_TIMEOUT       0xe3
#define VERIFY_10             0x13
#define START_STOP_BIT        0x00000001

enum scsi_device_power_state {
	SDEV_PW_UNKNOWN = 0,		//0
	SDEV_PW_STANDBY,		//1
	SDEV_PW_STANDBY_TIMEOUT_WAIT,	//2
	SDEV_PW_STANDBY_TIMEOUT_PASSED,	//3
    SDEV_PW_SPINNING_UP,	//4
	SDEV_PW_WAIT_FOR_SPIN_UP,	//5
	SDEV_PW_ON,			//6
};

#endif /* SCSI_SPINUP_H */

