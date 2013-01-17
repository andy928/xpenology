//#include <stdio.h>
#include "reg_handler.h"
#include "ad9889_macros.h"

extern DEPLINT (*i2c_write_bytes)(DEPLINT dev_addr_in, DEPLINT reg_addr_in, BYTE *data_in, DEPLINT count_in);
extern DEPLINT (*i2c_read_bytes)(DEPLINT dev_addr_in, DEPLINT reg_addr_in, BYTE *data_out, DEPLINT count_in);


DEPLINT adi_get_i2c_reg(BYTE dev_add, DEPLINT reg_add, BYTE mask_reg0, DEPLINT bit2shift)
{
	BYTE x;

	i2c_read_bytes(dev_add, reg_add, &x, 1);
	
	return ((DEPLINT) x & mask_reg0) >> bit2shift;
}


void adi_set_i2c_reg(BYTE dev_add, DEPLINT reg_add, BYTE mask_reg0, DEPLINT bit2shift, DEPLINT val)
{
	BYTE x;
  
	i2c_read_bytes(dev_add, reg_add, &x, 1);
         
	x = (x & ~mask_reg0) | (BYTE)((val<<bit2shift) & mask_reg0);
         
	i2c_write_bytes(dev_add, reg_add, &x, 1);
}


DEPLINT adi_get_i2c_reg_mult(BYTE dev_add, DEPLINT reg_add, BYTE mask_reg0, BYTE mask_reg1, DEPLINT bit2shift, DEPLINT bytes)
{
         
	BYTE x[MAX_BYTES];
	DEPLINT i;
	DEPLINT val = 0;

	for (i = 0; i < MAX_BYTES; i++)
		x[i] = 0;

	i2c_read_bytes(dev_add, reg_add, x, bytes);
	
	for(i = 0; i < bytes; i++)
	{
		
		if(i == (bytes-1))
		{
			val += ((DEPLINT) x[i] & mask_reg1) >> bit2shift;
		}
		else
		{
			val += ((DEPLINT) x[i] & mask_reg0) << (8*(bytes-i-1)-bit2shift);
		}
	}
	return val;
}


void adi_set_i2c_reg_mult(BYTE dev_add, DEPLINT reg_add, BYTE mask_reg0, BYTE mask_reg1, DEPLINT bit2shift, DEPLINT bytes, DEPLINT val)
{
	BYTE x[MAX_BYTES];
	DEPLINT i;

	for (i = 0; i < MAX_BYTES; i++)
		x[i] = 0;

	i2c_read_bytes(dev_add, reg_add, x, bytes);
	
	for (i = 0; i < bytes; i++)
	{
		//lsb
		if(i == (bytes-1))
		{
			x[i] = (x[i] & ~mask_reg1) | ((val<<bit2shift) & mask_reg1);
		}
		//msb
		else if(i == 0)
		{
			x[i] = (x[i] & ~mask_reg0) | (val>>(8*(bytes-1)-bit2shift)&mask_reg0);
		}
		//middle
		else
		{
			x[i] = x[i] | (val>>(8*(bytes-i-1)-bit2shift));
				
		}
	}

	i2c_write_bytes(AD9889_ADDRESS, reg_add, x, bytes);
}
