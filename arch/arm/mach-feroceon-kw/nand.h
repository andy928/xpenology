/*******************************************************************************
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
*******************************************************************************/

#ifndef _NAND_H_
#define _NAND_H_

/* defines */
#define	MV_NAND_ECC_4BIT	4
#define MV_NAND_ECC_1BIT	1

static struct nand_ecclayout mv_nand_rs_oobinfo = {
	.eccbytes = 40,
	.eccpos = {
			24, 25, 26, 27, 28, 29, 30, 31,
			32, 33, 34, 35, 36, 37, 38, 39,
			40, 41, 42, 43, 44, 45, 46, 47,
			48, 49, 50, 51, 52, 53, 54, 55,
			56, 57, 58, 59, 60, 61, 62, 63
		  },

	.oobfree = {{6, 18}}
};

//typedef unsigned int dtype;
typedef u_short dtype;

void generate_gf(void); /* Generate Galois Field */
void gen_poly(void);    /* Generate generator polynomial */

/* Reed-Solomon encoding
 * data[] is the input block, parity symbols are placed in bb[]
 * bb[] may lie past the end of the data, e.g., for (255,223):
 *      encode_rs(&data[0],&data[223]);
 */
static inline char encode_rs(dtype data[], dtype bb[]);


/* Reed-Solomon errors decoding
 * The received block goes into data[]
 *
 * The decoder corrects the symbols in place, if possible and returns
 * the number of corrected symbols. If the codeword is illegal or
 * uncorrectible, the data array is unchanged and -1 is returned
 */
static inline int decode_rs(dtype data[]);

/**
 * nand_calculate_ecc - [NAND Interface] Calculate 3 byte ECC code for 256 byte block
 * @mtd:        MTD block structure
 * @dat:        raw data
 * @ecc_code:   buffer for ECC
 */
int mv_nand_calculate_ecc_rs(struct mtd_info *mtd, const u_char *data, u_char *ecc_code);

/**
 * nand_correct_data - [NAND Interface] Detect and correct bit error(s)
 * @mtd:        MTD block structure
 * @dat:        raw data read from the chip
 * @store_ecc:  ECC from the chip
 * @calc_ecc:   the ECC calculated from raw data
 *
 * Detect and correct a 1 bit error for 256 byte block
 */
int mv_nand_correct_data_rs(struct mtd_info *mtd, u_char *data, u_char *store_ecc, u_char *calc_ecc);

static void mv_nand_enable_hwecc(struct mtd_info *mtd, int mode);
#endif /* _NAND_H_ */

