/*******************************************************************************
Copyright (C) Marvell Interfdbional Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
Interfdbional Ltd. and/or its affiliates ("Marvell") under the following
alterfdbive licensing terms.  Once you have made an election to distribute the
File under one of the following license alterfdbives, please (i) delete this
introductory statement regarding license alterfdbives, (ii) delete the two
license alterfdbives that you have not elected to use and (iii) preserve the
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
* mvFastPath.c - Marvell Fast Route Routing and Bridge
*
* DESCRIPTION:
*       
*       Supported Features:
*       - OS independent. 
*
*******************************************************************************/

/* includes */
#include "mvOs.h"
#include "mvDebug.h"
#include "eth/nfp/mvNfp.h"
#include "eth/mvEth.h"


struct fdbRuleHashBucket    *fdbRuleDb;
MV_U32                      fdbRuleDbSize;
MV_U32			    fdbMember[ETH_FP_IFINDEX_MAX];
MV_U32                      fdbRuleUpdateCount = 0;
MV_U32                      fdbRuleSetCount = 0;
MV_U32                      fdbRuleDeleteCount = 0;
MV_U32                      fdbHashMaxDepth;


MV_STATUS mvFpFdbInit(MV_U32 dbSize)
{
	fdbRuleDb = (struct fdbRuleHashBucket *)mvOsMalloc(sizeof(struct fdbRuleHashBucket)*dbSize);
	if (fdbRuleDb == NULL) {
		mvOsPrintf("NFP (fdb): not enough memory\n");
		return MV_NO_RESOURCE;
	}
	fdbRuleDbSize = dbSize;
	memset(fdbRuleDb, 0, sizeof(struct fdbRuleHashBucket)*fdbRuleDbSize);
	memset(fdbMember, 0, sizeof(fdbMember));

	mvOsPrintf("NFP (fdb) init %d entries, %d bytes\n", 
	fdbRuleDbSize, sizeof(struct fdbRuleHashBucket)*fdbRuleDbSize);
	return MV_OK;
}

/* Clear Fast Route Bridge Rule Database (SNAT + DNAT table) */
MV_STATUS mvFpFdbClear(void)
{
	MV_U32 i = fdbRuleDbSize;
	MV_FP_FDB_RULE *rule, *tmp;

	if (fdbRuleDb != NULL)
		return MV_NOT_INITIALIZED;

	while (i--) {
		rule = fdbRuleDb[i].ruleChain;
		while (rule) {
			tmp = rule;
			rule = rule->next;
			mvOsFree(tmp);
		}
        	fdbRuleDb[i].ruleChain = NULL;
	}
	return MV_OK;
}

void mvFpFdbDestroy(void)
{
	if (fdbRuleDb != NULL)
		mvOsFree(fdbRuleDb);
}

static INLINE int mvFpFdbRuleCmp(MV_FP_RULE_FDB_INFO *rule1, MV_FP_RULE_FDB_INFO *rule2)
{
	return !((rule1->bridge == rule2->bridge) && 
			 (rule1->ifIndex == rule2->ifIndex) &&
			 !memcmp(rule1->mac, rule2->mac, 6));
}

static INLINE MV_U32 mvFpFdbRuleHash(MV_FP_FDB_RULE *rule)
{
	return mv_jhash_3words(	rule->fdbInfo.bridge, 
				0,*(MV_U32*)(rule->fdbInfo.mac+2), fp_ip_jhash_iv);
}

MV_STATUS mvFpFdbRuleSet(MV_FP_FDB_RULE *newrule)
{
    int             depth = 0;
    MV_U32          hash;
    MV_FP_FDB_RULE* rule;

	/* ignore foreign ifindex */
	if (newrule->fdbInfo.ifIndex >= ETH_FP_IFINDEX_MAX)
		return MV_OUT_OF_RANGE;

	hash = mvFpFdbRuleHash(newrule);
    hash &= (fdbRuleDbSize - 1);

    rule = fdbRuleDb[hash].ruleChain;    
    while (rule) {
		if (!mvFpFdbRuleCmp(&rule->fdbInfo, &newrule->fdbInfo)) 
        {
            fdbRuleUpdateCount++;
			goto out;
        }
        depth++;
        rule = rule->next;
    }
    fdbRuleSetCount++;
    if(depth > fdbHashMaxDepth)
        fdbHashMaxDepth = depth;

	rule = mvOsMalloc(sizeof(MV_FP_FDB_RULE));

    if (!rule) {
        mvOsPrintf("NFP (fdb): can't allocate new rule\n");
        return MV_FAIL;
    }

	/* FIXME: No spinlocks */
	rule->next = fdbRuleDb[hash].ruleChain;
	fdbRuleDb[hash].ruleChain = rule;
out:
	mvOsMemcpy(rule, newrule, sizeof(MV_FP_FDB_RULE));

	if (rule->fdbInfo.flags & MV_FP_FDB_IS_LOCAL) {
		fdbMember[rule->fdbInfo.ifIndex] = rule->fdbInfo.bridge;
		fdbMember[rule->fdbInfo.bridge] = rule->fdbInfo.bridge;
	}

	MV_NFP_DBG("NFP (fdb): new bridge=%d ifIndex=%d %02X:%02X:%02X:%02X:%02X:%02X flags=%x\n",
			rule->fdbInfo.bridge, rule->fdbInfo.ifIndex,
			rule->fdbInfo.mac[0], rule->fdbInfo.mac[1], rule->fdbInfo.mac[2], 
			rule->fdbInfo.mac[3], rule->fdbInfo.mac[4], rule->fdbInfo.mac[5], 
			rule->fdbInfo.flags);

    return MV_OK;
}

MV_STATUS mvFpFdbRuleDel(MV_FP_FDB_RULE *oldrule)
{
	MV_U32 hash;
	MV_FP_FDB_RULE* rule, *prev;

	/* ignore foreign ifindex */
	if (oldrule->fdbInfo.ifIndex >= ETH_FP_IFINDEX_MAX)
		return MV_OUT_OF_RANGE;

	if (oldrule->fdbInfo.flags & MV_FP_FDB_IS_LOCAL) {
		fdbMember[oldrule->fdbInfo.ifIndex] = 0;
		fdbMember[oldrule->fdbInfo.bridge] = 0;
		MV_NFP_DBG("NFP (fdb): del member bridge=%d ifIndex=%d\n", 
                oldrule->fdbInfo.bridge, oldrule->fdbInfo.ifIndex);
	}

	hash = mvFpFdbRuleHash(oldrule);
	hash &= (fdbRuleDbSize - 1);

	rule = fdbRuleDb[hash].ruleChain;    
	prev = NULL;

	while (rule) {
		if (!mvFpFdbRuleCmp(&rule->fdbInfo, &oldrule->fdbInfo)) {

			if (prev)
				prev->next = rule->next;
			else
				fdbRuleDb[hash].ruleChain = rule->next;

            fdbRuleDeleteCount++;
			MV_NFP_DBG("NFP (fdb): del bridge=%d ifIndex=%d %02X:%02X:%02X:%02X:%02X:%02X flags=%x count=%d\n",
					rule->fdbInfo.bridge, rule->fdbInfo.ifIndex,
					rule->fdbInfo.mac[0], rule->fdbInfo.mac[1], rule->fdbInfo.mac[2], 
					rule->fdbInfo.mac[3], rule->fdbInfo.mac[4], rule->fdbInfo.mac[5], 
					rule->fdbInfo.flags, rule->mgmtInfo.new_count);

			mvOsFree(rule);	
			return MV_OK;
		}

		prev = rule;
		rule = rule->next;
	}

	return MV_NOT_FOUND;
}

MV_U32 mvFpFdbRuleAge(MV_FP_FDB_RULE *oldrule)
{
	MV_U32 hash, age;
	MV_FP_FDB_RULE* rule, *prev;

	/* ignore foreign ifindex */
	if (oldrule->fdbInfo.ifIndex >= ETH_FP_IFINDEX_MAX)
		return 0;

	hash = mvFpFdbRuleHash(oldrule);
	hash &= (fdbRuleDbSize - 1);

	rule = fdbRuleDb[hash].ruleChain;    
	prev = NULL;

	while (rule) {
		if (!mvFpFdbRuleCmp(&rule->fdbInfo, &oldrule->fdbInfo)) {

			MV_NFP_DBG("NFP (fdb): age bridge=%d ifIndex=%d %02X:%02X:%02X:%02X:%02X:%02X flags=%x age=%d\n",
					rule->fdbInfo.bridge, rule->fdbInfo.ifIndex,
					rule->fdbInfo.mac[0], rule->fdbInfo.mac[1], rule->fdbInfo.mac[2], 
					rule->fdbInfo.mac[3], rule->fdbInfo.mac[4], rule->fdbInfo.mac[5], 
					rule->fdbInfo.flags, rule->mgmtInfo.new_count);

			age = rule->mgmtInfo.new_count;
			rule->mgmtInfo.new_count = 0;
			return age;

		}

		prev = rule;
		rule = rule->next;
	}

	return 0;
}

static void mvFpFdbRulePrint(MV_FP_FDB_RULE *rule, MV_U32 hash)
{
	mvOsPrintf("NFP (fdb): 0x%x bridge=%d ifIndex=%d %02X:%02X:%02X:%02X:%02X:%02X flags=%x count=%d\n",
			mvFpFdbRuleHash((MV_FP_FDB_RULE*)rule),
			rule->fdbInfo.bridge, rule->fdbInfo.ifIndex,
			rule->fdbInfo.mac[0], rule->fdbInfo.mac[1], rule->fdbInfo.mac[2], 
			rule->fdbInfo.mac[3], rule->fdbInfo.mac[4], rule->fdbInfo.mac[5], 
			rule->fdbInfo.flags, rule->mgmtInfo.new_count);
}

MV_STATUS mvFpFdbPrint(void)
{
	MV_U32 i = fdbRuleDbSize;
	MV_FP_FDB_RULE *rule;

	for (i=0; i<ETH_FP_IFINDEX_MAX; i++) {
		if (fdbMember[i])
			mvOsPrintf("NFP (fdb): bridge=%d ifIndex=%d\n",fdbMember[i],i);
	}

	for (i=0; i<fdbRuleDbSize; i++) {
		rule = fdbRuleDb[i].ruleChain;

       		while (rule) {
			mvFpFdbRulePrint(rule, i);
			rule = rule->next;
       		}
    	}
    	return MV_OK;
}


