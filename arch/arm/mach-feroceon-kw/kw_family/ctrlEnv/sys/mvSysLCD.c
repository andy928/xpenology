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

#include "lcd/mvLcd.h"
#include "mvSysLCD.h"

/* defines  */
#ifdef MV_DEBUG
	#define DB(x)	x
#else
	#define DB(x)
#endif	


static MV_STATUS lcdWinOverlapDetect(MV_U32 winNum, MV_ADDR_WIN *pAddrWin);

MV_TARGET lcdAddrDecPrioTap[] = 
{
#if defined(MV_INCLUDE_DEVICE_CS0)
    DEVICE_CS0,
#endif
#if defined(MV_INCLUDE_PEX)
	PEX0_MEM,
	PEX1_MEM,
#endif
#if defined(MV_INCLUDE_SDRAM_CS0)
    SDRAM_CS0,
#endif
#if defined(MV_INCLUDE_SDRAM_CS1)
    SDRAM_CS1,
#endif
#if defined(MV_INCLUDE_SDRAM_CS2)
    SDRAM_CS2,
#endif
#if defined(MV_INCLUDE_SDRAM_CS3)
    SDRAM_CS3,
#endif
#if defined(MV_INCLUDE_DEVICE_CS1)
    DEVICE_CS1,
#endif
#if defined(MV_INCLUDE_CESA)
   CRYPT_ENG,
#endif 
	TBL_TERM
};
static MV_STATUS mvLcdInitWinsUnit (void)
{
	MV_U32		winNum;
	MV_LCD_DEC_WIN	addrDecWin;
	MV_CPU_DEC_WIN	cpuAddrDecWin;
	MV_U32          status;
	MV_U32		winPrioIndex=0;

	/* Initiate LCD address decode */

	/* First disable all address decode windows */
	for(winNum = 0; winNum < LCD_MAX_ADDR_DEC_WIN; winNum++)
	{
	    mvLcdTargetWinEnable(winNum, MV_FALSE);
	}
	
	/* Go through all windows in user table until table terminator			*/
	for (winNum = 0; ((lcdAddrDecPrioTap[winPrioIndex] != TBL_TERM) &&
					(winNum < LCD_MAX_ADDR_DEC_WIN));)
	{
		/* first get attributes from CPU If */
		status = mvCpuIfTargetWinGet(lcdAddrDecPrioTap[winPrioIndex], 
									 &cpuAddrDecWin);

        if(MV_NO_SUCH == status)
        {
            winPrioIndex++;
            continue;
        }
		if (MV_OK != status)
		{
            mvOsPrintf("%s: ERR. mvCpuIfTargetWinGet failed\n", __FUNCTION__);
			return MV_ERROR;
		}

	
        if (cpuAddrDecWin.enable == MV_TRUE)
		{

			addrDecWin.target           = lcdAddrDecPrioTap[winPrioIndex];
			addrDecWin.addrWin.baseLow  = cpuAddrDecWin.addrWin.baseLow; 
			addrDecWin.addrWin.baseHigh = cpuAddrDecWin.addrWin.baseHigh; 
			addrDecWin.addrWin.size     = cpuAddrDecWin.addrWin.size;    
			addrDecWin.enable           = MV_TRUE;
	
			if (MV_OK != mvLcdTargetWinSet(winNum, &addrDecWin))
			{
				DB(mvOsPrintf("mvLcdInit: ERR. mvDmaTargetWinSet failed\n"));
				return MV_ERROR;
			}
			winNum++;
		}
		winPrioIndex++;	
        
	}
    
	return MV_OK;
}


/*******************************************************************************
* mvLcdInit - Initialize Lcd engine
*
* DESCRIPTION:
*		This function initialize LCD unit. It set the default address decode
*		windows of the unit.
*		Note that if the address window is disabled in lcdAddrDecMap, the
*		window parameters will be set but the window will remain disabled.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_BAD_PARAM if parameters to function invalid, MV_OK otherwise.
*******************************************************************************/
MV_STATUS mvLcdInit (MV_VOID)
{
	if(mvCtrlModelGet() != MV_6282_DEV_ID)
	    return MV_OK;
	
	/* Initiate LCD address decode */
	mvLcdInitWinsUnit();

	mvLcdHalInit();	
    
	return MV_OK;
}

/*******************************************************************************
* mvLcdTargetWinSet - Set LCDtarget address window
*
* DESCRIPTION:
*       This function sets a peripheral target (e.g. SDRAM bank0, PCI_MEM0) 
*       address window. After setting this target window, the LCD will be 
*       able to access the target within the address window. 
*
* INPUT:
*	winNum - One of the possible LCD memory decode windows.
*       target - Peripheral target enumerator.
*       base   - Window base address.
*       size   - Window size.
*       enable - Window enable/disable.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_BAD_PARAM if parameters to function invalid, MV_OK otherwise.
*
*******************************************************************************/
MV_STATUS mvLcdTargetWinSet(MV_U32 winNum, MV_LCD_DEC_WIN *pAddrDecWin)
{
    MV_DEC_REGS lcdDecRegs;
    MV_TARGET_ATTRIB targetAttribs;
    
    /* Parameter checking */
    if (winNum >= LCD_MAX_ADDR_DEC_WIN)
    {
	DB(mvOsPrintf("%s: ERR. Invalid win num %d\n",__FUNCTION__, winNum));
        return MV_BAD_PARAM;
    }
    if (pAddrDecWin == NULL)
    {
        DB(mvOsPrintf("%s: ERR. pAddrDecWin is NULL pointer\n", __FUNCTION__ ));
        return MV_BAD_PTR;
    }                                         
    /* Check if the requested window overlaps with current windows */
    if (MV_TRUE == lcdWinOverlapDetect(winNum, &pAddrDecWin->addrWin))
    {
	DB(mvOsPrintf("%s: ERR. Window %d overlap\n",__FUNCTION__,winNum));
	return MV_ERROR;
    }                              

    lcdDecRegs.baseReg = MV_REG_READ(LCD_BASE_ADDR_REG(winNum));
    lcdDecRegs.sizeReg = MV_REG_READ(LCD_WINDOW_CTRL_REG(winNum));

    /* Get Base Address and size registers values */
    if(MV_OK != mvCtrlAddrDecToReg(&pAddrDecWin->addrWin, &lcdDecRegs))
    {
	DB(mvOsPrintf("%s: ERR. Invalid addr dec window\n",__FUNCTION__));
        return MV_BAD_PARAM;
	}
    

	mvCtrlAttribGet(pAddrDecWin->target,&targetAttribs);

	/* set attributes */
	lcdDecRegs.sizeReg &= ~LCDWCR_ATTR_MASK;
	lcdDecRegs.sizeReg |= targetAttribs.attrib << LCDWCR_ATTR_OFFS;
	/* set target ID */
	lcdDecRegs.sizeReg &= ~LCDWCR_TARGET_MASK;
	lcdDecRegs.sizeReg |= targetAttribs.targetId << LCDWCR_TARGET_OFFS;


    /* Write to address decode Base Address Register */
	MV_REG_WRITE(LCD_BASE_ADDR_REG(winNum), lcdDecRegs.baseReg);
    
    /* Write to Size Register */
    MV_REG_WRITE(LCD_WINDOW_CTRL_REG(winNum), lcdDecRegs.sizeReg);
    
    if (pAddrDecWin->enable)
    {
        MV_REG_BIT_SET(LCD_WINDOW_CTRL_REG(winNum), LCDWCR_WIN_EN_MASK);
    }
    else
    {
        MV_REG_BIT_RESET(LCD_WINDOW_CTRL_REG(winNum), LCDWCR_WIN_EN_MASK);
    }

    return MV_OK;
}

/*******************************************************************************
* mvLcdTargetWinGet - Get LCD peripheral target address window.
*
* DESCRIPTION:
*		Get LCD peripheral target address window.
*
* INPUT:
*	  winNum - One of the possible LCD memory decode windows.
*
* OUTPUT:
*       base   - Window base address.
*       size   - Window size.
*       enable - window enable/disable.
*
* RETURN:
*       MV_BAD_PARAM if parameters to function invalid, MV_OK otherwise.
*
*******************************************************************************/
MV_STATUS mvLcdTargetWinGet(MV_U32 winNum, MV_LCD_DEC_WIN *pAddrDecWin)
{
    MV_DEC_REGS lcdDecRegs;
    MV_TARGET_ATTRIB targetAttrib;
    MV_U32      winEn;
    
    /* Parameter checking */
    if (winNum >= LCD_MAX_ADDR_DEC_WIN)
    {
		DB(mvOsPrintf("%s: ERR. Invalid win num %d\n",__FUNCTION__ , winNum));
        return MV_ERROR;
    }

    if (NULL == pAddrDecWin)
    {
        DB(mvOsPrintf("%s: ERR. pAddrDecWin is NULL pointer\n", __FUNCTION__ ));
        return MV_BAD_PTR;
    }

    winEn = MV_REG_READ(LCD_WINDOW_CTRL_REG(winNum)) & LCDWCR_WIN_EN_MASK;
    
    /* Check if enable bit is equal for all channels */
    if ((MV_REG_READ(LCD_WINDOW_CTRL_REG(winNum)) & 
             LCDWCR_WIN_EN_MASK) != winEn)
    {
            mvOsPrintf("%s: ERR. Window enable field must be equal in "
                              "all channels(chan=%d)\n",__FUNCTION__, chan);
            return MV_ERROR;
    }



	lcdDecRegs.baseReg  = MV_REG_READ(LCD_BASE_ADDR_REG(winNum));
	lcdDecRegs.sizeReg  = MV_REG_READ(LCD_WINDOW_CTRL_REG(winNum));

	if (MV_OK != mvCtrlRegToAddrDec(&lcdDecRegs, &pAddrDecWin->addrWin))
	{
		mvOsPrintf("%s: ERR. mvCtrlRegToAddrDec failed\n", __FUNCTION__);
		return MV_ERROR;
	}

	/* attrib and targetId */
	targetAttrib.attrib = 
		(lcdDecRegs.sizeReg & LCDWCR_ATTR_MASK) >> LCDWCR_ATTR_OFFS;
	targetAttrib.targetId = 
		(lcdDecRegs.sizeReg & LCDWCR_TARGET_MASK) >> LCDWCR_TARGET_OFFS;


	pAddrDecWin->target = mvCtrlTargetGet(&targetAttrib);

	if(winEn)
	{
		pAddrDecWin->enable = MV_TRUE;
	}
	else pAddrDecWin->enable = MV_FALSE;
	
    return MV_OK;
}

/*******************************************************************************
* mvLcdTargetWinEnable - Enable/disable a LCD address decode window
*
* DESCRIPTION:
*       This function enable/disable a LCD address decode window.
*       if parameter 'enable' == MV_TRUE the routine will enable the 
*       window, thus enabling LCD accesses (before enabling the window it is 
*       tested for overlapping). Otherwise, the window will be disabled.
*
* INPUT:
*       winNum - Decode window number.
*       enable - Enable/disable parameter.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_BAD_PARAM if parameters to function invalid, MV_OK otherwise.
*
*******************************************************************************/
MV_STATUS mvLcdTargetWinEnable(MV_U32 winNum, MV_BOOL enable)
{
    MV_LCD_DEC_WIN  addrDecWin;
    
	/* Parameter checking   */               
    if (winNum >= LCD_MAX_ADDR_DEC_WIN)
    {
        DB(mvOsPrintf("%s: ERR. Invalid winNum%d\n", __FUNCTION__, winNum));
        return MV_ERROR;
    }

	if (enable == MV_TRUE) 
	{
		/* Get current window */
	    if (MV_OK != mvLcdTargetWinGet(winNum, &addrDecWin))
	    {
			DB(mvOsPrintf("%s: ERR. targetWinGet fail\n", __FUNCTION__));
			return MV_ERROR;
	    }

		/* Check for overlapping */
	    if (MV_TRUE == lcdWinOverlapDetect(winNum, &(addrDecWin.addrWin)))
	    {
			/* Overlap detected	*/
			DB(mvOsPrintf("%s: ERR. Overlap detected\n", __FUNCTION__));
			return MV_ERROR;
	    }

		/* No Overlap. Enable address decode target window */
		MV_REG_BIT_SET(LCD_WINDOW_CTRL_REG(winNum),LCDWCR_WIN_EN_MASK);
	}
	else
	{
		/* Disable address decode target window */
		MV_REG_BIT_RESET(LCD_WINDOW_CTRL_REG(winNum),LCDWCR_WIN_EN_MASK);
	}

	return MV_OK;                  
}

/*******************************************************************************
* mvXorPciRemap - Set XOR remap register for PCI address windows.
*
* DESCRIPTION:
*       only Windows 0-3 can be remapped.
*
* INPUT:
*       winNum      - window number
*       pAddrDecWin  - pointer to address space window structure
* OUTPUT:
*       None.
*
* RETURN:
*       MV_BAD_PARAM if parameters to function invalid, MV_OK otherwise.
*
*******************************************************************************/
MV_STATUS mvLcdPciRemap(MV_U32 winNum, MV_U32 addrHigh)
{
    /* Parameter checking   */
    if (winNum >= LCD_MAX_REMAP_WIN)
    {
	DB(mvOsPrintf("%s: ERR. Invalid win num %d\n", __FUNCTION__, winNum));
        return MV_BAD_PARAM;
    }

    MV_REG_WRITE(LCD_ADDR_REMAP_REG(winNum), addrHigh);
    
	return MV_OK;
}

/*******************************************************************************
* lcdWinOverlapDetect - Detect LCD address windows overlaping
*
* DESCRIPTION:
*       An unpredicted behaviour is expected in case LCD address decode 
*       windows overlaps.
*       This function detects LCD address decode windows overlaping of a 
*       specified window. The function does not check the window itself for 
*       overlaping. The function also skipps disabled address decode windows.
*
* INPUT:
*       winNum      - address decode window number.
*       pAddrDecWin - An address decode window struct.
*
* OUTPUT:
*       None.
*
* RETURN:
*       MV_TRUE if the given address window overlap current address
*       decode map, MV_FALSE otherwise, MV_ERROR if reading invalid data 
*       from registers.
*
*******************************************************************************/
static MV_STATUS lcdWinOverlapDetect(MV_U32 winNum, MV_ADDR_WIN *pAddrWin)
{
	MV_U32 	        baseAddrEnableReg;
	MV_U32          winNumIndex;
	MV_LCD_DEC_WIN  addrDecWin;

	if (pAddrWin == NULL)
	{
		DB(mvOsPrintf("%s: ERR. pAddrWin is NULL pointer\n", __FUNCTION__ ));
		return MV_BAD_PTR;
	}
    

		for (winNumIndex = 0; winNumIndex < LCD_MAX_ADDR_DEC_WIN; winNumIndex++)
		{
			/* Do not check window itself */
			if (winNumIndex == winNum)
			{
				continue;
			}
    
			/* Read base address enable register. Do not check disabled windows	*/
			baseAddrEnableReg = MV_REG_READ(LCD_WINDOW_CTRL_REG(winNumIndex));

			/* Do not check disabled windows */
			if ((baseAddrEnableReg & LCDWCR_WIN_EN_MASK) == 0)
			{
				continue;
			}
    
			/* Get window parameters */
			if (MV_OK != mvLcdTargetWinGet(winNumIndex, &addrDecWin))
			{
				DB(mvOsPrintf("%s: ERR. TargetWinGet failed\n", __FUNCTION__ ));
				return MV_ERROR;
			}
            
			if (MV_TRUE == ctrlWinOverlapTest(pAddrWin, &(addrDecWin.addrWin)))
			{
				return MV_TRUE;
			}
		}
	
	return MV_FALSE;
}

static MV_VOID mvLcdAddrDecShowUnit(void)
{
	MV_LCD_DEC_WIN win;
	int            i;

	mvOsOutput( "\n" );
	mvOsOutput( "LCD:\n");
	mvOsOutput( "----\n" );

	for( i = 0; i < LCD_MAX_ADDR_DEC_WIN; i++ )
	{
		memset( &win, 0, sizeof(MV_LCD_DEC_WIN) );

		mvOsOutput( "win%d - ", i );

		if( mvLcdTargetWinGet(i, &win ) == MV_OK )
		{
			if( win.enable )
			{
				mvOsOutput( "%s base %x, ",
				mvCtrlTargetNameGet(win.target), win.addrWin.baseLow );

				mvSizePrint( win.addrWin.size );
				
				mvOsOutput( "\n" );
			}
			else
				mvOsOutput( "disable\n" );
		}
	}
}

/*******************************************************************************
* mvLcdAddrDecShow - Print the LCD address decode map.
*
* DESCRIPTION:
*		This function print the LCD address decode map.
*
* INPUT:
*       None.
*
* OUTPUT:
*       None.
*
* RETURN:
*       None.
*
*******************************************************************************/
MV_VOID mvLcdAddrDecShow(MV_VOID)
{
	if(mvCtrlModelGet() != MV_6282_DEV_ID)
	    return;

	mvLcdAddrDecShowUnit();
}
