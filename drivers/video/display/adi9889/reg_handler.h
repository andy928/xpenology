//#include "AD9889_setup.h"
#include "type_def.h"
//#include "AD9889A_i2c_handler.h"
//#include "AD9880_i2c_handler.h"

#ifndef _REG_HANDLER_H_
#define _REG_HANDLER_H_

#define MAX_BYTES 4

DEPLINT adi_get_i2c_reg(BYTE dev_add,DEPLINT reg_add, BYTE mask_reg0, DEPLINT bit2shift);
void adi_set_i2c_reg(BYTE dev_add,DEPLINT reg_add, BYTE mask_reg0, DEPLINT bit2shift, DEPLINT val);
DEPLINT adi_get_i2c_reg_mult(BYTE dev_add,DEPLINT reg_add, BYTE mask_reg0, BYTE mask_reg1, DEPLINT bit2shift, DEPLINT bytes);
void adi_set_i2c_reg_mult(BYTE dev_add,DEPLINT reg_add, BYTE mask_reg0, BYTE mask_reg1, DEPLINT bit2shift, DEPLINT bytes, DEPLINT val);

#endif //_REG_HANDLER_H_
