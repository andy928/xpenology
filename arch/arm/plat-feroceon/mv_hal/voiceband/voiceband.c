/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File under the following licensing terms. 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer. 

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution. 

    *   Neither the name of Marvell nor the names of its contributors may be 
        used to endorse or promote products derived from this software without 
        specific prior written permission. 
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "voiceband/voiceband.h"
#include "voiceband/tdm/mvTdm.h"
#include "spi/mvSpi.h"

#define DAA_READ_CONTROL_BYTE	0x60	
#define DAA_WRITE_CONTROL_BYTE	0x20

MV_VOID mvVbSpiRead(MV_U16 lineId, MV_U8 address, mv_line_t lineType, MV_U8 *data)
{
	MV_U8 control = 0, spiMode;
	MV_U32 val1 = 0, val2 = 0, cmd = 0;

#ifdef MV_TDM_2CHANNELS
	spiMode = mvBoardTdmSpiModeGet();

	if(lineType == MV_LINE_FXO)
	{
		if(spiMode) /* daisy-chain */
		{
	   		control = DAA_READ_CONTROL_BYTE << 1; /* 2 channels */
		}
		else /* dedicated CS */
		{
			control = DAA_READ_CONTROL_BYTE;
		}

		val1 =  (MV_U32)((address<<8) | control);
		cmd = TRANSFER_BYTES(2) | ENDIANESS_MSB_MODE | RD_MODE | READ_1_BYTE | CLK_SPEED_LO_DIV;
	}
	else /* MV_LINE_FXS */
	{
		address |= 0x80;

		if(spiMode)
		{
			val1 = (MV_U32)((address<<8) | (1 << lineId));
			cmd = TRANSFER_BYTES(2) | ENDIANESS_MSB_MODE | RD_MODE | READ_1_BYTE | CLK_SPEED_LO_DIV;
		}
		else
		{
			val1 = address;
			cmd = TRANSFER_BYTES(1) | ENDIANESS_MSB_MODE | RD_MODE | READ_1_BYTE | CLK_SPEED_LO_DIV;
		}

	}

	mvTdmSpiRead(val1, val2, cmd, lineId, data);

#else /* MV_TDM_32CHANNELS */
	/* TBD - Fix dedicated SPI driver APIs  */

#endif /* MV_TDM_2CHANNELS */
}

MV_VOID mvVbSpiWrite(MV_U32 lineId, MV_U8 address, mv_line_t lineType, MV_U8 data)
{
	MV_U8 control = 0, spiMode;
	MV_U32 val1 = 0, val2 = 0, cmd = 0;

#ifdef MV_TDM_2CHANNELS
	spiMode = mvBoardTdmSpiModeGet();

	if(lineType == MV_LINE_FXO)
	{
		if(spiMode) /* daisy-chain */
		{
			control = DAA_WRITE_CONTROL_BYTE << 1; /* 2 channels */
		}
		else /* dedicated CS */
		{
			control = DAA_WRITE_CONTROL_BYTE;
		}

		val1 = (MV_U32)((address<<8) | control);
		val2 = (MV_U32)data;
		cmd = TRANSFER_BYTES(3) | ENDIANESS_MSB_MODE | WR_MODE | CLK_SPEED_LO_DIV;
	}
	else /* MV_LINE_FXS */
	{
		if(spiMode)
		{
			val1 = (MV_U32)((address<<8) | (1 << lineId));
			val2 = data;
			cmd = TRANSFER_BYTES(3) | ENDIANESS_MSB_MODE | WR_MODE | CLK_SPEED_LO_DIV;
		}
		else 
		{
			val1 = (data<<8) | address;
			cmd = TRANSFER_BYTES(2) | ENDIANESS_MSB_MODE | WR_MODE | CLK_SPEED_LO_DIV;
		}
	}

	mvTdmSpiWrite(val1, val2, cmd, lineId);

#else /* MV_TDM_32CHANNELS */
/* TBD - Fix dedicated SPI driver APIs  */

#endif /* MV_TDM_2CHANNELS */
}

