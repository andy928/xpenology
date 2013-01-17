/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2006 Analog Devices Inc.                                      *
* All rights reserved.                                                        *
*                                                                             *
* This source code is intended for the recipient only under the guidelines of *
* the non-disclosure agreement with Analog Devices Inc.                       *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
* This software is intended for use with the AD9889A and derivative parts only*
*                                                                             *
******************************************************************************/

/******************************************************************************
*                                                                             *
* THIS IS A READ-ONLY SOURCE FILE. DO NOT EDIT IT DIRECTLY.                   *
* Author: Matthew McCarn (matthew.mccarn@analog.com)                          *
* Date: May 16, 2006                                                          *
*                                                                             *
******************************************************************************/
#ifndef _AD9889_MACROS_H_
#define _AD9889_MACROS_H_

#define AD9889_ADDRESS		0x72
#define AD9889_MEMORY_ADDRESS	0x7E
#define AD9889A			0
#define AD9389A			1
#define HDMI_TX			AD9889A
#define HDCP_OPTION

#define ERROR_MESSAGE(A)	printk(KERN_ERR A)
#define DEBUG_MESSAGE(A)	printk(A)
#define DEBUG_MESSAGE_1(A,B)	printk(A,B)
#define DEBUG_MESSAGE_2(A,B,C)	printk(A,B,C)

#define ST_MAX			5
#define DTD_MAX			5
#define SAD_MAX			5
#define SVD_MAX			5

#define BKSV_MAX		75
#define MAX_HDCP_NACK		4

#define TIMER_CALLS_PER_2_SEC	20

void *chkmalloc(size_t sz);
void *chkrealloc(void *ptr,size_t sz);

#endif //_AD9889_MACROS_H_
