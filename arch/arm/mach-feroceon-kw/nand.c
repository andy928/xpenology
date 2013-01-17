
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <asm/io.h>
#include "ctrlEnv/sys/mvCpuIf.h"

#ifdef	CONFIG_NAND_RS_ECC_SUPPORT
#include "nand.h"
#endif

#undef DEBUG
#ifdef DEBUG
#define DBG(fmt, arg...)	mvOsPrintf(KERN_INFO fmt, ##arg) 
#else 
#define DBG(fmt, arg...)
#endif

extern MV_U32    boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devType);

static struct mtd_info *mv_mtd;
static unsigned long baseaddr;

unsigned int mv_nand_ecc;

#define MV_NUM_OF_NAND_PARTS 3
static struct mtd_partition parts_info[] = {
	{ .name = "u-boot",
	  .offset = 0,
	  .size = 1 * 1024 * 1024 },
	{ .name = "uImage",
	  .offset = MTDPART_OFS_NXTBLK,
	  .size = 2 * 1024 * 1024 },
	{ .name = "root",
	  .offset = MTDPART_OFS_NXTBLK,
	  .size = MTDPART_SIZ_FULL },
};
static const char *part_probes[] __initdata = { "cmdlinepart", NULL };

static void board_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	if (ctrl & NAND_CTRL_CHANGE) {        
		this->IO_ADDR_W = (void __iomem *)((unsigned long)this->IO_ADDR_W & ~3);
		ctrl &= ~NAND_CTRL_CHANGE;
		switch(ctrl) {
			case NAND_CTRL_CLE: 
				this->IO_ADDR_W = (void __iomem *)((unsigned long)this->IO_ADDR_W | 1); //x8=>1, x16=>2
				break;
			case NAND_CTRL_ALE:
				this->IO_ADDR_W = (void __iomem *)((unsigned long)this->IO_ADDR_W | 2); //x8=>2, x16=>4
				break;
		}				
	}
	if (cmd != NAND_CMD_NONE) 
	{
		writeb(cmd, this->IO_ADDR_W);		
	}	
}

static void mv_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	void __iomem *io_base = chip->IO_ADDR_R;
	uint64_t *buf64;
	int i = 0;

	while (len && (unsigned long)buf & 7)
	{
		*buf++ = readb(io_base);
		len--;
	}

	buf64 = (uint64_t *)buf;
	while (i < len/8)
	{
		uint64_t x;
		__asm__ __volatile__ ("ldrd\t%0, [%1]" : "=r" (x) : "r" (io_base));
		buf64[i++] = x;
	}

	i *= 8;
	while (i < len)
	{
		buf[i++] = readb(io_base);
	}
}



int __init mv_nand_init(void)
{
	struct nand_chip *this;
	int err = 0;
	int num_of_parts = 0;
	const char *part_type = 0;
	struct mtd_partition *mtd_parts = 0;
	u32 physaddr;
	int nand_dev_num;
	MV_CPU_DEC_WIN addr_win;

	nand_dev_num =boardGetDevCSNum(0, BOARD_DEV_NAND_FLASH);
	if(-1 == nand_dev_num) {
		printk("NAND init: NAND device not found on board\n");
		err = -ENODEV;
		goto out;
	}

	if( MV_OK != mvCpuIfTargetWinGet((DEVICE_CS0 + nand_dev_num), &addr_win) ) {
                printk("Failed to init NAND MTD (boot-CS window %d err).\n", (DEVICE_CS0 + nand_dev_num));
		err = -ENODEV;
		goto out;
	}

	if(!addr_win.enable) {
                printk("Failed to init NAND MTD (boot-CS window disabled).\n" );
		err = -ENODEV;
		goto out;
	}
	physaddr = addr_win.addrWin.baseLow;

	mv_mtd = (struct mtd_info *)kmalloc(sizeof(struct mtd_info)+sizeof(struct nand_chip), GFP_KERNEL);
	if(!mv_mtd){
		printk("Failed to allocate NAND MTD structure\n");
		err = -ENOMEM;
		goto out;
	}

	memset((char*)mv_mtd,0,sizeof(struct mtd_info)+sizeof(struct nand_chip));

	baseaddr = (unsigned long)ioremap(physaddr, 1024);
	if(!baseaddr) {
		printk("Failed to remap NAND MTD\n");
		err = -EIO;
		goto out_mtd;
	}

	this = (struct nand_chip *)((char *)mv_mtd+sizeof(struct mtd_info));
	mv_mtd->priv = this;
	this->IO_ADDR_R = this->IO_ADDR_W = (void __iomem *)baseaddr;
	this->cmd_ctrl = board_hwcontrol;
#ifdef	CONFIG_MV_NAND_8BYTE_READ
	this->read_buf = mv_nand_read_buf;
#endif
#ifdef	CONFIG_NAND_RS_ECC_SUPPORT
	printk("Using %s ECC for NAND device\n",(mv_nand_ecc == MV_NAND_ECC_4BIT ? 
		"Reed-Solomon 4-bit" : "Hamming 1-bit"));
	if(mv_nand_ecc == MV_NAND_ECC_4BIT) {
	this->ecc.hwctl		= mv_nand_enable_hwecc; 
	this->ecc.calculate	= mv_nand_calculate_ecc_rs;
	this->ecc.correct	= mv_nand_correct_data_rs;
	this->ecc.size		= 512;
	this->ecc.bytes		= 10;
	this->ecc.layout = &mv_nand_rs_oobinfo;
	this->ecc.mode = NAND_ECC_HW;
	}
	else
#endif
	this->ecc.mode = NAND_ECC_SOFT;
	this->chip_delay = 30;
	if(nand_scan(mv_mtd,1)) {
		err = -ENXIO;
		goto out_ior;
	}

        mv_mtd->name = "nand_mtd";
        num_of_parts = parse_mtd_partitions(mv_mtd,part_probes,&mtd_parts,0);
        if(num_of_parts > 0)
                part_type = "command line";
        else
                num_of_parts = 0;
        if(num_of_parts == 0) {
                mtd_parts = parts_info;
                num_of_parts = MV_NUM_OF_NAND_PARTS;
                part_type = "static";
        }

	printk("Using %s partition definition\n", part_type);
	add_mtd_partitions(mv_mtd, mtd_parts, num_of_parts);
	goto out;

out_ior:
	iounmap((void *)baseaddr);
out_mtd:
	kfree(mv_mtd);
out:
	return err;
}

module_init(mv_nand_init);

#ifdef MODULE
static void __exit board_cleanup(void)
{
	nand_release(mv_mtd);
	iounmap((void*)baseaddr);
	kfree(mv_mtd);
}
module_exit(board_cleanup);
#endif

#ifdef CONFIG_NAND_RS_ECC_SUPPORT

#define mm 10	  /* RS code over GF(2**mm) - the size in bits of a symbol*/
#define	nn 1023   /* nn=2^mm -1   length of codeword */
#define tt 4      /* number of errors that can be corrected */
#define kk 1015   /* kk = number of information symbols  kk = nn-2*tt  */


static char rs_initialized = 0;

//typedef unsigned int gf;
typedef u_short tgf;  /* data type of Galois Functions */

/* Primitive polynomials -  irriducibile polynomial  [ 1+x^3+x^10 ]*/
short pp[mm+1] = { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 };


/* index->polynomial form conversion table */
tgf alpha_to[nn + 1];

/* Polynomial->index form conversion table */
tgf index_of[nn + 1];

/* Generator polynomial g(x) = 2*tt with roots @, @^2, .. ,@^(2*tt) */
tgf Gg[nn - kk + 1];


#define	minimum(a,b)	((a) < (b) ? (a) : (b))

#define	BLANK(a,n) {\
	short ci;\
	for(ci=0; ci<(n); ci++)\
		(a)[ci] = 0;\
	}

#define	COPY(a,b,n) {\
	short ci;\
	for(ci=(n)-1;ci >=0;ci--)\
		(a)[ci] = (b)[ci];\
	}
#define	COPYDOWN(a,b,n) {\
	short ci;\
	for(ci=(n)-1;ci >=0;ci--)\
		(a)[ci] = (b)[ci];\
	}


/* generate GF(2^m) from the irreducible polynomial p(X) in p[0]..p[mm]
   lookup tables:  index->polynomial form   alpha_to[] contains j=alpha^i;
                   polynomial form -> index form  index_of[j=alpha^i] = i
   alpha=2 is the primitive element of GF(2^m) 
*/

void generate_gf(void)
{
	register int i, mask;

	mask = 1;
	alpha_to[mm] = 0;
	for (i = 0; i < mm; i++) {
		alpha_to[i] = mask;
		index_of[alpha_to[i]] = i;
		if (pp[i] != 0)
			alpha_to[mm] ^= mask;	
		mask <<= 1;	
	}
	index_of[alpha_to[mm]] = mm;
	
	mask >>= 1;
	for (i = mm + 1; i < nn; i++) {
		if (alpha_to[i - 1] >= mask)
			alpha_to[i] = alpha_to[mm] ^ ((alpha_to[i - 1] ^ mask) << 1);
		else
			alpha_to[i] = alpha_to[i - 1] << 1;
		index_of[alpha_to[i]] = i;
	}
	index_of[0] = nn;  
	alpha_to[nn] = 0;
}


/*
 * Obtain the generator polynomial of the tt-error correcting, 
 * length nn = (2^mm -1) 
 * Reed Solomon code from the product of (X + @^i), i=1..2*tt
*/
void gen_poly(void)
{
	register int i, j;

	Gg[0] = alpha_to[1]; /* primitive element*/ 
	Gg[1] = 1;		     /* g(x) = (X+@^1) initially */
	for (i = 2; i <= nn - kk; i++) {
		Gg[i] = 1;
		/*
		 * Below multiply (Gg[0]+Gg[1]*x + ... +Gg[i]x^i) by
		 * (@^i + x)
		 */
		for (j = i - 1; j > 0; j--)
			if (Gg[j] != 0)
				Gg[j] = Gg[j - 1] ^ alpha_to[((index_of[Gg[j]]) + i)%nn];
			else
				Gg[j] = Gg[j - 1];
		Gg[0] = alpha_to[((index_of[Gg[0]]) + i) % nn];
	}
	/* convert Gg[] to index form for quicker encoding */
	for (i = 0; i <= nn - kk; i++)
		Gg[i] = index_of[Gg[i]];
}


/*
 * take the string of symbols in data[i], i=0..(k-1) and encode
 * systematically to produce nn-kk parity symbols in bb[0]..bb[nn-kk-1] data[]
 * is input and bb[] is output in polynomial form. Encoding is done by using
 * a feedback shift register with appropriate connections specified by the
 * elements of Gg[], which was generated above. Codeword is   c(X) =
 * data(X)*X**(nn-kk)+ b(X)
 */
static inline char encode_rs(dtype data[kk], dtype bb[nn-kk])
{
	register int i, j;
	tgf feedback;

	BLANK(bb,nn-kk);
	for (i = kk - 1; i >= 0; i--) {
		if(data[i] > nn)
			return -1;	/* Illegal symbol */
		feedback = index_of[data[i] ^ bb[nn - kk - 1]];
		if (feedback != nn) {	/* feedback term is non-zero */
			for (j = nn - kk - 1; j > 0; j--)
				if (Gg[j] != nn)					
					bb[j] = bb[j - 1] ^ alpha_to[(Gg[j] + feedback)%nn];
				else
					bb[j] = bb[j - 1];			
			bb[0] = alpha_to[(Gg[0] + feedback)%nn];
		} else {	
			for (j = nn - kk - 1; j > 0; j--)
				bb[j] = bb[j - 1];
			bb[0] = 0;
		}
	}
	return 0;
}




/* assume we have received bits grouped into mm-bit symbols in data[i],
   i=0..(nn-1), We first compute the 2*tt syndromes, then we use the 
   Berlekamp iteration to find the error location polynomial  elp[i].   
   If the degree of the elp is >tt, we cannot correct all the errors
   and hence just put out the information symbols uncorrected. If the 
   degree of elp is <=tt, we  get the roots, hence the inverse roots, 
   the error location numbers. If the number of errors located does not 
   equal the degree of the elp, we have more than tt errors and cannot 
   correct them.  Otherwise, we then solve for the error value at the 
   error location and correct the error.The procedure is that found in
   Lin and Costello.*/

static inline int decode_rs(dtype data[nn])
{
	int deg_lambda, el, deg_omega;
	int i, j, r;
	tgf q,tmp,num1,num2,den,discr_r;
	tgf recd[nn];
	tgf lambda[nn-kk + 1], s[nn-kk + 1];	/* Err+Eras Locator poly
						                 * and syndrome poly  */
	tgf b[nn-kk + 1], t[nn-kk + 1], omega[nn-kk + 1];
	tgf root[nn-kk], reg[nn-kk + 1], loc[nn-kk];
	int syn_error, count;

	

	/* data[] is in polynomial form, copy and convert to index form */
	for (i = nn-1; i >= 0; i--){

	  if(data[i] > nn)
		return -1;	/* Illegal symbol */

	  recd[i] = index_of[data[i]];
	}

	/* first form the syndromes; i.e., evaluate recd(x) at roots of g(x)
	 * namely @**(1+i), i = 0, ... ,(nn-kk-1)
	 */

	syn_error = 0;

	for (i = 1; i <= nn-kk; i++) {
		tmp = 0;
		
		for (j = 0; j < nn; j++)
			if (recd[j] != nn)	/* recd[j] in index form */
								tmp ^= alpha_to[(recd[j] + (1+i-1)*j)%nn];
		
			syn_error |= tmp;	/* set flag if non-zero syndrome =>
					 * error */
	/* store syndrome in index form  */
		s[i] = index_of[tmp];
	}

	if (!syn_error) {
		/*
		 * if syndrome is zero, data[] is a codeword and there are no
		 * errors to correct. So return data[] unmodified
		 */
		return 0;
	}

	BLANK(&lambda[1],nn-kk);
	
	lambda[0] = 1;
	
	for(i=0;i<nn-kk+1;i++)
		b[i] = index_of[lambda[i]];

	/*
	 * Begin Berlekamp-Massey algorithm to determine error
	 * locator polynomial
	 */
	r = 0; 
	el = 0; 
	while (++r <= nn-kk) {	/* r is the step number */
		/* Compute discrepancy at the r-th step in poly-form */
		discr_r = 0;
		
		for (i = 0; i < r; i++){
			if ((lambda[i] != 0) && (s[r - i] != nn)) {				
				discr_r ^= alpha_to[(index_of[lambda[i]] + s[r - i])%nn];
			}
		}
		

		discr_r = index_of[discr_r];	/* Index form */
		if (discr_r == nn) {
			/* 2 lines below: B(x) <-- x*B(x) */
			COPYDOWN(&b[1],b,nn-kk);
			b[0] = nn;
		} else {
			/* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
			t[0] = lambda[0];
			for (i = 0 ; i < nn-kk; i++) {
				if(b[i] != nn)
					//t[i+1] = lambda[i+1] ^ alpha_to[modnn(discr_r + b[i])];
					t[i+1] = lambda[i+1] ^ alpha_to[(discr_r + b[i])%nn];
				else
					t[i+1] = lambda[i+1];
			}
			if (2 * el <= r - 1) {
				el = r - el;
				/*
				 * 2 lines below: B(x) <-- inv(discr_r) *
				 * lambda(x)
				 */
				for (i = 0; i <= nn-kk; i++)
					//b[i] = (lambda[i] == 0) ? nn : modnn(index_of[lambda[i]] - discr_r + nn);
					b[i] = (lambda[i] == 0) ? nn : ((index_of[lambda[i]] - discr_r + nn)%nn);
			} else {
				/* 2 lines below: B(x) <-- x*B(x) */
				COPYDOWN(&b[1],b,nn-kk);
				b[0] = nn;
			}
			COPY(lambda,t,nn-kk+1);
		}
	}

	/* Convert lambda to index form and compute deg(lambda(x)) */
	deg_lambda = 0;
	for(i=0;i<nn-kk+1;i++){
		lambda[i] = index_of[lambda[i]];
		if(lambda[i] != nn)
			deg_lambda = i;
	}
	/*
	 * Find roots of the error locator polynomial. By Chien
	 * Search
	 */
	COPY(&reg[1],&lambda[1],nn-kk);
	count = 0;		/* Number of roots of lambda(x) */
	for (i = 1; i <= nn; i++) {
		q = 1;
		for (j = deg_lambda; j > 0; j--)
			if (reg[j] != nn) {
				//reg[j] = modnn(reg[j] + j);
				reg[j] = (reg[j] + j)%nn;
				q ^= alpha_to[reg[j]];
			}
		if (!q) {
			/* store root (index-form) and error location number */
			root[count] = i;
			loc[count] = nn - i;
			count++;
		}
	}

#ifdef DEBUG
/*
	printk("\n Final error positions:\t");
	for (i = 0; i < count; i++)
		printk("%d ", loc[i]);
	printk("\n");
*/
#endif
	
	
	
	if (deg_lambda != count) {
		/*
		 * deg(lambda) unequal to number of roots => uncorrectable
		 * error detected
		 */
		return -1;
	}
	/*
	 * Compute err evaluator poly omega(x) = s(x)*lambda(x) (modulo
	 * x**(nn-kk)). in index form. Also find deg(omega).
	 */
	
	deg_omega = 0;
	for (i = 0; i < nn-kk;i++){
		tmp = 0;
		j = (deg_lambda < i) ? deg_lambda : i;
		for(;j >= 0; j--){
			if ((s[i + 1 - j] != nn) && (lambda[j] != nn))
				//tmp ^= alpha_to[modnn(s[i + 1 - j] + lambda[j])];
				tmp ^= alpha_to[(s[i + 1 - j] + lambda[j])%nn];
		}
		if(tmp != 0)
			deg_omega = i;
		omega[i] = index_of[tmp];
	}
	omega[nn-kk] = nn;




	/*
	 * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
	 * inv(X(l))**(1-1) and den = lambda_pr(inv(X(l))) all in poly-form
	 */
	for (j = count-1; j >=0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != nn)
				//num1  ^= alpha_to[modnn(omega[i] + i * root[j])];
				num1  ^= alpha_to[(omega[i] + i * root[j])%nn];
		}
		//num2 = alpha_to[modnn(root[j] * (1 - 1) + nn)];
		num2 = alpha_to[(root[j] * (1 - 1) + nn)%nn];
		den = 0;

		/* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
		for (i = minimum(deg_lambda,nn-kk-1) & ~1; i >= 0; i -=2) {
			if(lambda[i+1] != nn)
				//den ^= alpha_to[modnn(lambda[i+1] + i * root[j])];
				den ^= alpha_to[(lambda[i+1] + i * root[j])%nn];
		}
		if (den == 0) {
#ifdef DEBUG
			printk("\n ERROR: denominator = 0\n");
#endif
			return -1;
		}
		/* Apply error to data */
		if (num1 != 0) {
			//data[loc[j]] ^= alpha_to[modnn(index_of[num1] + index_of[num2] + nn - index_of[den])];
			data[loc[j]] ^= alpha_to[(index_of[num1] + index_of[num2] + nn - index_of[den])%nn];
		}
	}
	return count;
}

/**
 * mv_nand_calculate_ecc_rs - [NAND Interface] Calculate 4 symbol ECC code for 512 byte block
 * @mtd:	MTD block structure
 * @dat:	raw data
 * @ecc_code:	buffer for ECC
 */
int mv_nand_calculate_ecc_rs(struct mtd_info *mtd, const u_char *data, u_char *ecc_code)
{
    	int i;	
	u_short rsdata[nn];
	
	/* Generate Tables in first run */
	if (!rs_initialized) {
		generate_gf();
		gen_poly();		
		rs_initialized = 1;
	}

	for(i=512; i<nn; i++)
		rsdata[i] = 0;
	
		for(i=0; i<512; i++)
			rsdata[i] = (u_short) data[i];	
		if ((encode_rs(rsdata,&(rsdata[kk]))) != 0) 
			return -1;
		*(ecc_code) 	= (unsigned char) rsdata[kk];
		*(ecc_code+1) 	= ((rsdata[0x3F7])   >> 8) | ((rsdata[0x3F7+1]) << 2);
		*(ecc_code+2) 	= ((rsdata[0x3F7+1]) >> 6) | ((rsdata[0x3F7+2]) << 4);  
		*(ecc_code+3) 	= ((rsdata[0x3F7+2]) >> 4) | ((rsdata[0x3F7+3]) << 6);
		*(ecc_code+4) 	= ((rsdata[0x3F7+3]) >> 2);
		*(ecc_code+5) 	= (unsigned char) rsdata[kk+4];
		*(ecc_code+6)	= ((rsdata[0x3F7+4])   >> 8) | ((rsdata[0x3F7+1+4]) << 2);
		*(ecc_code+7) 	= ((rsdata[0x3F7+1+4]) >> 6) | ((rsdata[0x3F7+2+4]) << 4);
		*(ecc_code+8) 	= ((rsdata[0x3F7+2+4]) >> 4) | ((rsdata[0x3F7+3+4]) << 6);
		*(ecc_code+9) 	= ((rsdata[0x3F7+3+4]) >> 2);

	return 0;
}

/**
 * mv_nand_correct_data - [NAND Interface] Detect and correct bit error(s)
 * @mtd:	MTD block structure
 * @dat:	raw data read from the chip
 * @store_ecc:	ECC from the chip
 * @calc_ecc:	the ECC calculated from raw data
 *
 * Detect and correct a 1 bit error for 256 byte block
 */
int mv_nand_correct_data_rs(struct mtd_info *mtd, u_char *data, u_char *store_ecc, u_char *calc_ecc)
{
	int ret,i=0;
	u_short rsdata[nn];
	
	/* Generate Tables in first run */
	if (!rs_initialized) {
		generate_gf();
		gen_poly();		
		rs_initialized = 1;
	}

	/* is decode needed ? */
	if((*(u32*)store_ecc == *(u32*)calc_ecc) &&
	   (*(u32*)(store_ecc + 4) == *(u32*)(calc_ecc + 4)) &&
	   (*(u16*)(store_ecc + 8) == *(u16*)(calc_ecc + 8)))
		return 0;

	/* did we read an erased page ? */
	for(i = 0; i < 512 ;i += 4) {
		if(*(u32*)(data+i) != 0xFFFFFFFF) {
			DBG("%s: trying to correct data\n",__FUNCTION__);
			goto correct;
		}
	}
	/* page was erased, return gracefully */
	return 0;
	
correct:

	for(i=512; i<nn; i++) 
	rsdata[i] = 0;

    	/* errors*/
	//data[20] = 0xDD;
	//data[30] = 0xDD;
	//data[40] = 0xDD;
	//data[50] = 0xDD;
	//data[60] = 0xDD;

	/* Ecc is calculated on chunks of 512B */
		for(i=0; i<512; i++)
			rsdata[i] = (u_short) data[i];

		rsdata[kk]   = ( (*(store_ecc+1) & 0x03) <<8) | (*(store_ecc));
		rsdata[kk+1] = ( (*(store_ecc+2) & 0x0F) <<6) | (*(store_ecc+1)>>2);
		rsdata[kk+2] = ( (*(store_ecc+3) & 0x3F) <<4) | (*(store_ecc+2)>>4);
		rsdata[kk+3] = (*(store_ecc+4) <<2) | (*(store_ecc+3)>>6);

		rsdata[kk+4] = ( (*(store_ecc+1+5) & 0x03) <<8) | (*(store_ecc+5));
		rsdata[kk+5] = ( (*(store_ecc+2+5) & 0x0F) <<6) | (*(store_ecc+1+5)>>2);
		rsdata[kk+6] = ( (*(store_ecc+3+5) & 0x3F) <<4) | (*(store_ecc+2+5)>>4);
		rsdata[kk+7] = (*(store_ecc+4+5) <<2) | (*(store_ecc+3+5)>>6);

		ret = decode_rs(rsdata);

		/* Check for excessive errors */
		if ((ret > tt) || (ret < 0)) {
			printk("%s: uncorrectable error !!!\n",__FUNCTION__); 
			return -1;
		}
	
		/* Copy corrected data */
		for (i=0; i<512; i++)
			data[i] = (unsigned char) rsdata[i];

	return 0;
}

static void mv_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	return;
}

#endif	/* CONFIG_NAND_RS_ECC_SUPPORT */

