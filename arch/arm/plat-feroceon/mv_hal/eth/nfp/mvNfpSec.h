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

/*******************************************************************************
* mvNfpSec.h - Header File for Marvell NFP IPSec (Routing only)
*
* DESCRIPTION:
*       This header file contains macros, typedefs and function declarations 
* 	specific to the Marvell Network Fast Processing with IPSec(Routing only).
*
* DEPENDENCIES:
*       None.
*
*******************************************************************************/

#ifndef __mvNfpSec_h__
#define __mvNfpSec_h__

#ifdef CONFIG_MV_ETH_NFP_SEC

#include "mvCommon.h"
#include "cesa/mvCesa.h"

/* IPSec defines */
#define MV_NFP_SEC_MAX_PACKET		1540
#define MV_NFP_SEC_ENC_BLOCK_SIZE	16

#define MV_NFP_SEC_ESP_OFFSET		34

/* IPSec Enumerators */
typedef enum {
	MV_NFP_SEC_TUNNEL = 0,
	MV_NFP_SEC_TRANSPORT,
} MV_NFP_SEC_PROT;

typedef enum {
	MV_NFP_SEC_ESP = 0,
	MV_NFP_SEC_AH,
} MV_NFP_SEC_ENCAP;

typedef enum {
	MV_NFP_SEC_RULE_DB_IN = 0,
	MV_NFP_SEC_RULE_DB_OUT,
} MV_NFP_SEC_RULE_DB_DIR;

typedef enum {
	MV_NFP_SEC_DROP = 0,
	MV_NFP_SEC_FWD,
	MV_NFP_SEC_SECURE
} MV_NFP_SEC_ACTION;

typedef enum {
	MV_NFP_SEC_ENCRYPT = 0,
	MV_NFP_SEC_DECRYPT,
} MV_NFP_SEC_OP;

typedef struct _mv_nfp_sa_stats {
	MV_U32 encrypt;
	MV_U32 decrypt;
	MV_U32 rejected; /* slow path */
	MV_U32 dropped;  /* packet drop */
	MV_U32 bytes;
} MV_NFP_SA_STATS;

/* IPSec Structures */
typedef struct _mv_nfp_sec_tunnel_hdr {
	MV_U32 sIp; // BE
	MV_U32 dIp; //BE
	/* dstMac should be 2 byte aligned */
	MV_U8  dstMac[MV_MAC_ADDR_SIZE]; //BE
	MV_U8  srcMac[MV_MAC_ADDR_SIZE]; //BE
	MV_U8  outIfIndex;
} MV_NFP_SEC_TUNNEL_HDR;

typedef struct _mv_nfp_sec_sa_entry {
	MV_U32 		      spi; //BE
	MV_NFP_SEC_PROT   tunProt;
	MV_NFP_SEC_ENCAP  encap; 
	MV_U16 		      sid; 	
	MV_U32		      seqNum; //LE 
	MV_NFP_SEC_TUNNEL_HDR tunnelHdr; 
	MV_U32 		      lifeTime;
	MV_U8		      ivSize; 
	MV_U8		      cipherBlockSize;
	MV_U8		      digestSize;
	MV_NFP_SEC_OP	  secOp;
	MV_NFP_SA_STATS   stats;
} MV_NFP_SEC_SA_ENTRY;

typedef struct _mv_nfp_sec_spd_rule {
	MV_U32 		     sIp; //BE
	MV_U32 		     dIp; //BE
	MV_U8 		     proto;
	MV_U16 		     dstPort; //BE
	MV_U16 		     srcPort; //BE
	MV_NFP_SEC_ACTION    actionType;
	MV_NFP_SEC_SA_ENTRY* pSAEntry;
} MV_NFP_SEC_SPD_RULE;

typedef struct _mv_nfp_sec_cesa_priv {
        MV_NFP_SEC_SA_ENTRY     *pSaEntry;
        MV_PKT_INFO             *pPktInfo;
	MV_U8			orgDigest[MV_CESA_MAX_DIGEST_SIZE];
	MV_CESA_COMMAND		*pCesaCmd;
} MV_NFP_SEC_CESA_PRIV;


MV_NFP_SEC_SPD_RULE* mvNfpSecSPDRuleFind(MV_U32 dstIp, MV_U32 srcIp,
	MV_U8 proto, MV_U16 dport, MV_U16 sport, MV_NFP_SEC_RULE_DB_DIR inOut);

MV_STATUS mvNfpSecOutCheck(MV_PKT_INFO *pPktInfo);

MV_STATUS mvNfpSecOutgoing(MV_PKT_INFO *pPktInfo, MV_NFP_SEC_SA_ENTRY* pSAEntry);

MV_STATUS mvNfpSecEspProcess(MV_PKT_INFO *pPktInfo, MV_NFP_SEC_SA_ENTRY* pSAEntry);

MV_NFP_SEC_SA_ENTRY* mvNfpSecSARuleFind(MV_U32 spiPkt);

MV_STATUS mvNfpSecIncoming(MV_PKT_INFO *pPktInfo, MV_NFP_SEC_SA_ENTRY* pSAEntry);

MV_NFP_SEC_SPD_RULE* mvNfpSecSPDRuleSet(MV_NFP_SEC_SPD_RULE* pSpdRule, MV_NFP_SEC_RULE_DB_DIR inOut);

MV_NFP_SEC_SA_ENTRY* mvNfpSecSAEntrySet(MV_NFP_SEC_SA_ENTRY* pSAEntry, MV_NFP_SEC_RULE_DB_DIR inOut);

MV_STATUS mvNfpSecInit(MV_U32 dbSize); // global init

MV_VOID mvNfpSecDbPrint(MV_VOID);

#endif /* CONFIG_MV_ETH_NFP_SEC */

#endif /* __mvNfpSec_h__ */

