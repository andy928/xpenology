/**************************************************************************

Module Name:

   CSMISAS.H


Abstract:

   This file contains constants and data structure definitions used by drivers
   that support the Common Storage Management Interface specification for
   SAS or SATA in either the Windows or Linux.

   This should be considered as a reference implementation only.  Changes may 
   be necessary to accommodate a specific build environment or target OS.

Revision History:

   001  SEF   8/12/03  Initial release.
   002  SEF   8/20/03  Cleanup to match documentation.
   003  SEF   9/12/03  Additional cleanup, created combined header
   004  SEF   9/23/03  Changed base types to match linux defaults
                       Added RAID signature
                       Added bControllerFlags to CSMI_SAS_CNTLR_CONFIG
                       Changed CSMI_SAS_BEGIN_PACK to 8 for common structures
                       Fixed other typos identified in first compilation test
   005  SEF  10/03/03  Additions to match first version of CSMI document
   006  SEF  10/14/03  Fixed typedef struct _CSMI_SAS_SMP_PASSTHRU_BUFFER
                       Added defines for bConnectionRate
   007  SEF  10/15/03  Added Firmware Download Control Code and support
                       Added CSMI revision support
   008  SEF  10/30/03  No functional change, just updated version to track
                       spec changes
   009  SEF  12/09/03  No functional change, just updated version to track
                       spec changes
   010  SEF   3/11/04  Fixed typedef struct CSMI_SAS_RAID_DRIVES to include the
                       bFirmware member that is defined in the spec, but
                       was missing in this file,
                       added CC_CSMI_SAS_TASK_MANAGEMENT
   011  SEF   4/02/04  No functional change, added comment line before
                       CC_CSMI_SAS_TASK_MANAGEMENT
   012  SEF   4/16/04  Added IOControllerNumber to linux header,
                       Modified linux control codes to have upper word of
                       0xCC77.... to indicate CSMI version 77
                       Added bSignalClass to CC_CSMI_SET_PHY_INFO
                       Added CC_CSMI_SAS_PHY_CONTROL support
   013  SEF   5/14/04  Added CC_CSMI_SAS_GET_CONNECTOR_INFO support
   014  SEF   5/24/04  No functional change, just updated version to track spec
                       changes
   015  SEF   6/16/04  changed bPinout to uPinout to reflect proper size,
                       changed width of bLocation defines to reflect size
   016  SEF   6/17/04  changed bLengthOfControls in CSMI_SAS_PHY_CONTROL
                       to be proper size
   017  SEF   9/17/04  added CSMI_SAS_SATA_PORT_SELECTOR,
                       CSMI_SAS_LINK_VIRTUAL, CSMI_SAS_CON_NOT_PRESENT, and
                       CSMI_SAS_CON_NOT_CONNECTED
   018  SEF   9/20/04  added CSMI_SAS_PHY_USER_PATTERN, 
                       changed definition of CSMI_SAS_PHY_FIXED_PATTERN to not
                       conflict with activate definition
   019  SEF  12/06/04  added CSMI_SAS_GET_LOCATION
                       added bSSPStatus to CSMI_SAS_SSP_PASSTHRU_STATUS 
                       structure

**************************************************************************/

#ifndef _CSMI_SAS_H_
#define _CSMI_SAS_H_

// CSMI Specification Revision, the intent is that all versions of the
// specification will be backward compatible after the 1.00 release.
// Major revision number, corresponds to xxxx. of CSMI specification
// Minor revision number, corresponds to .xxxx of CSMI specification
#define CSMI_MAJOR_REVISION   0
#define CSMI_MINOR_REVISION   83

/*************************************************************************/
/* TARGET OS LINUX SPECIFIC CODE                                         */
/*************************************************************************/

// EDM #ifdef _linux
#ifdef __KERNEL__


// Linux base types

#include <linux/types.h>

// pack definition

// EDM #define CSMI_SAS_BEGIN_PACK(x)    pack(x)
// EDM #define CSMI_SAS_END_PACK         pack()

// IOCTL Control Codes
// (IoctlHeader.ControlCode)

// Control Codes prior to 0.77

// Control Codes requiring CSMI_ALL_SIGNATURE

// #define CC_CSMI_SAS_GET_DRIVER_INFO    0x12345678
// #define CC_CSMI_SAS_GET_CNTLR_CONFIG   0x23456781
// #define CC_CSMI_SAS_GET_CNTLR_STATUS   0x34567812
// #define CC_CSMI_SAS_FIRMWARE_DOWNLOAD  0x92345678

// Control Codes requiring CSMI_RAID_SIGNATURE

// #define CC_CSMI_SAS_GET_RAID_INFO      0x45678123
// #define CC_CSMI_SAS_GET_RAID_CONFIG    0x56781234

// Control Codes requiring CSMI_SAS_SIGNATURE

// #define CC_CSMI_SAS_GET_PHY_INFO       0x67812345
// #define CC_CSMI_SAS_SET_PHY_INFO       0x78123456
// #define CC_CSMI_SAS_GET_LINK_ERRORS    0x81234567
// #define CC_CSMI_SAS_SMP_PASSTHRU       0xA1234567
// #define CC_CSMI_SAS_SSP_PASSTHRU       0xB1234567
// #define CC_CSMI_SAS_STP_PASSTHRU       0xC1234567
// #define CC_CSMI_SAS_GET_SATA_SIGNATURE 0xD1234567
// #define CC_CSMI_SAS_GET_SCSI_ADDRESS   0xE1234567
// #define CC_CSMI_SAS_GET_DEVICE_ADDRESS 0xF1234567
// #define CC_CSMI_SAS_TASK_MANAGEMENT    0xA2345678

// Control Codes for 0.77 and later

// Control Codes requiring CSMI_ALL_SIGNATURE

#define CC_CSMI_SAS_GET_DRIVER_INFO    0xCC770001
#define CC_CSMI_SAS_GET_CNTLR_CONFIG   0xCC770002
#define CC_CSMI_SAS_GET_CNTLR_STATUS   0xCC770003
#define CC_CSMI_SAS_FIRMWARE_DOWNLOAD  0xCC770004

// Control Codes requiring CSMI_RAID_SIGNATURE

#define CC_CSMI_SAS_GET_RAID_INFO      0xCC77000A
#define CC_CSMI_SAS_GET_RAID_CONFIG    0xCC77000B

// Control Codes requiring CSMI_SAS_SIGNATURE

#define CC_CSMI_SAS_GET_PHY_INFO       0xCC770014
#define CC_CSMI_SAS_SET_PHY_INFO       0xCC770015
#define CC_CSMI_SAS_GET_LINK_ERRORS    0xCC770016
#define CC_CSMI_SAS_SMP_PASSTHRU       0xCC770017
#define CC_CSMI_SAS_SSP_PASSTHRU       0xCC770018
#define CC_CSMI_SAS_STP_PASSTHRU       0xCC770019
#define CC_CSMI_SAS_GET_SATA_SIGNATURE 0xCC770020
#define CC_CSMI_SAS_GET_SCSI_ADDRESS   0xCC770021
#define CC_CSMI_SAS_GET_DEVICE_ADDRESS 0xCC770022
#define CC_CSMI_SAS_TASK_MANAGEMENT    0xCC770023
#define CC_CSMI_SAS_GET_CONNECTOR_INFO 0xCC770024
#define CC_CSMI_SAS_GET_LOCATION       0xCC770025

// Control Codes requiring CSMI_PHY_SIGNATURE

#define CC_CSMI_SAS_PHY_CONTROL        0xCC77003C

// EDM #pragma CSMI_SAS_BEGIN_PACK(8)
#pragma pack(8)

// IOCTL_HEADER
typedef struct _IOCTL_HEADER {
	__u32 IOControllerNumber;
	__u32 Length;
	__u32 ReturnCode;
	__u32 Timeout;
	__u16 Direction;
} IOCTL_HEADER, *PIOCTL_HEADER;

// EDM #pragma CSMI_SAS_END_PACK
#pragma pack()

#endif

#define __i8    char

/*************************************************************************/
/* TARGET OS WINDOWS SPECIFIC CODE                                       */
/*************************************************************************/

#ifdef _WIN32

// windows IOCTL definitions

#ifndef _NTDDSCSIH_
#include <ntddscsi.h>
#endif

// pack definition

#if defined _MSC_VER
   #define CSMI_SAS_BEGIN_PACK(x)    pack(push,x)
   #define CSMI_SAS_END_PACK         pack(pop)
#elif defined __BORLANDC__
   #define CSMI_SAS_BEGIN_PACK(x)    option -a##x
   #define CSMI_SAS_END_PACK         option -a.
#else
   #error "CSMISAS.H - Must externally define a pack compiler designator."
#endif

// base types

#define __u8    unsigned char
#define __u32   unsigned long
#define __u16   unsigned short

#define __i8    char

// IOCTL Control Codes
// (IoctlHeader.ControlCode)

// Control Codes requiring CSMI_ALL_SIGNATURE

#define CC_CSMI_SAS_GET_DRIVER_INFO    1
#define CC_CSMI_SAS_GET_CNTLR_CONFIG   2
#define CC_CSMI_SAS_GET_CNTLR_STATUS   3
#define CC_CSMI_SAS_FIRMWARE_DOWNLOAD  4

// Control Codes requiring CSMI_RAID_SIGNATURE

#define CC_CSMI_SAS_GET_RAID_INFO      10
#define CC_CSMI_SAS_GET_RAID_CONFIG    11

// Control Codes requiring CSMI_SAS_SIGNATURE

#define CC_CSMI_SAS_GET_PHY_INFO       20
#define CC_CSMI_SAS_SET_PHY_INFO       21
#define CC_CSMI_SAS_GET_LINK_ERRORS    22
#define CC_CSMI_SAS_SMP_PASSTHRU       23
#define CC_CSMI_SAS_SSP_PASSTHRU       24
#define CC_CSMI_SAS_STP_PASSTHRU       25
#define CC_CSMI_SAS_GET_SATA_SIGNATURE 26
#define CC_CSMI_SAS_GET_SCSI_ADDRESS   27
#define CC_CSMI_SAS_GET_DEVICE_ADDRESS 28
#define CC_CSMI_SAS_TASK_MANAGEMENT    29
#define CC_CSMI_SAS_GET_CONNECTOR_INFO 30
#define CC_CSMI_SAS_GET_LOCATION       31

// Control Codes requiring CSMI_PHY_SIGNATURE

#define CC_CSMI_SAS_PHY_CONTROL        60

#define IOCTL_HEADER SRB_IO_CONTROL
#define PIOCTL_HEADER PSRB_IO_CONTROL

#else /* _WIN32 */
#define _WIN32                          0
#endif /* _WIN32 */

/*************************************************************************/
/* TARGET OS NOT DEFINED ERROR                                           */
/*************************************************************************/

// EDM #if (!_WIN32 && !_linux)
#if (!_WIN32 && !__KERNEL__)
   #error "Unknown target OS."
#endif

/*************************************************************************/
/* OS INDEPENDENT CODE                                                   */
/*************************************************************************/

/* * * * * * * * * * Class Independent IOCTL Constants * * * * * * * * * */

// Return codes for all IOCTL's regardless of class
// (IoctlHeader.ReturnCode)

#define CSMI_SAS_STATUS_SUCCESS              0
#define CSMI_SAS_STATUS_FAILED               1
#define CSMI_SAS_STATUS_BAD_CNTL_CODE        2
#define CSMI_SAS_STATUS_INVALID_PARAMETER    3
#define CSMI_SAS_STATUS_WRITE_ATTEMPTED      4

// Signature value
// (IoctlHeader.Signature)

#define CSMI_ALL_SIGNATURE    "CSMIALL"

// Timeout value default of 60 seconds
// (IoctlHeader.Timeout)

#define CSMI_ALL_TIMEOUT      60

//  Direction values for data flow on this IOCTL
// (IoctlHeader.Direction, Linux only)
#define CSMI_SAS_DATA_READ    0
#define CSMI_SAS_DATA_WRITE   1

// I/O Bus Types
// ISA and EISA bus types are not supported
// (bIoBusType)

#define CSMI_SAS_BUS_TYPE_PCI       3
#define CSMI_SAS_BUS_TYPE_PCMCIA    4

// Controller Status
// (uStatus)

#define CSMI_SAS_CNTLR_STATUS_GOOD     1
#define CSMI_SAS_CNTLR_STATUS_FAILED   2
#define CSMI_SAS_CNTLR_STATUS_OFFLINE  3
#define CSMI_SAS_CNTLR_STATUS_POWEROFF 4

// Offline Status Reason
// (uOfflineReason)

#define CSMI_SAS_OFFLINE_REASON_NO_REASON             0
#define CSMI_SAS_OFFLINE_REASON_INITIALIZING          1
#define CSMI_SAS_OFFLINE_REASON_BACKSIDE_BUS_DEGRADED 2
#define CSMI_SAS_OFFLINE_REASON_BACKSIDE_BUS_FAILURE  3

// Controller Class
// (bControllerClass)

#define CSMI_SAS_CNTLR_CLASS_HBA    5

// Controller Flag bits
// (uControllerFlags)

#define CSMI_SAS_CNTLR_SAS_HBA   0x00000001
#define CSMI_SAS_CNTLR_SAS_RAID  0x00000002
#define CSMI_SAS_CNTLR_SATA_HBA  0x00000004
#define CSMI_SAS_CNTLR_SATA_RAID 0x00000008

// for firmware download
#define CSMI_SAS_CNTLR_FWD_SUPPORT  0x00010000
#define CSMI_SAS_CNTLR_FWD_ONLINE   0x00020000
#define CSMI_SAS_CNTLR_FWD_SRESET   0x00040000
#define CSMI_SAS_CNTLR_FWD_HRESET   0x00080000
#define CSMI_SAS_CNTLR_FWD_RROM     0x00100000

// Download Flag bits
// (uDownloadFlags)
#define CSMI_SAS_FWD_VALIDATE       0x00000001
#define CSMI_SAS_FWD_SOFT_RESET     0x00000002
#define CSMI_SAS_FWD_HARD_RESET     0x00000004

// Firmware Download Status
// (usStatus)
#define CSMI_SAS_FWD_SUCCESS        0
#define CSMI_SAS_FWD_FAILED         1
#define CSMI_SAS_FWD_USING_RROM     2
#define CSMI_SAS_FWD_REJECT         3
#define CSMI_SAS_FWD_DOWNREV        4

// Firmware Download Severity
// (usSeverity>
#define CSMI_SAS_FWD_INFORMATION    0
#define CSMI_SAS_FWD_WARNING        1
#define CSMI_SAS_FWD_ERROR          2
#define CSMI_SAS_FWD_FATAL          3

/* * * * * * * * * * SAS RAID Class IOCTL Constants  * * * * * * * * */

// Return codes for the RAID IOCTL's regardless of class
// (IoctlHeader.ControlCode)

#define CSMI_SAS_RAID_SET_OUT_OF_RANGE       1000

// Signature value
// (IoctlHeader.Signature)

#define CSMI_RAID_SIGNATURE    "CSMIARY"

// Timeout value default of 60 seconds
// (IoctlHeader.Timeout)

#define CSMI_RAID_TIMEOUT      60

// RAID Types
// (bRaidType)
#define CSMI_SAS_RAID_TYPE_NONE     0
#define CSMI_SAS_RAID_TYPE_0        1
#define CSMI_SAS_RAID_TYPE_1        2
#define CSMI_SAS_RAID_TYPE_10       3
#define CSMI_SAS_RAID_TYPE_5        4
#define CSMI_SAS_RAID_TYPE_15       5
#define CSMI_SAS_RAID_TYPE_OTHER    255

// RAID Status
// (bStatus)
#define CSMI_SAS_RAID_SET_STATUS_OK          0
#define CSMI_SAS_RAID_SET_STATUS_DEGRADED    1
#define CSMI_SAS_RAID_SET_STATUS_REBUILDING  2
#define CSMI_SAS_RAID_SET_STATUS_FAILED      3

// RAID Drive Status
// (bDriveStatus)
#define CSMI_SAS_DRIVE_STATUS_OK          0
#define CSMI_SAS_DRIVE_STATUS_REBUILDING  1
#define CSMI_SAS_DRIVE_STATUS_FAILED      2
#define CSMI_SAS_DRIVE_STATUS_DEGRADED    3

// RAID Drive Usage
// (bDriveUsage)
#define CSMI_SAS_DRIVE_CONFIG_NOT_USED 0
#define CSMI_SAS_DRIVE_CONFIG_MEMBER   1
#define CSMI_SAS_DRIVE_CONFIG_SPARE    2

/* * * * * * * * * * SAS HBA Class IOCTL Constants * * * * * * * * * */

// Return codes for SAS IOCTL's
// (IoctlHeader.ReturnCode)

#define CSMI_SAS_PHY_INFO_CHANGED            CSMI_SAS_STATUS_SUCCESS
#define CSMI_SAS_PHY_INFO_NOT_CHANGEABLE     2000
#define CSMI_SAS_LINK_RATE_OUT_OF_RANGE      2001

#define CSMI_SAS_PHY_DOES_NOT_EXIST          2002
#define CSMI_SAS_PHY_DOES_NOT_MATCH_PORT     2003
#define CSMI_SAS_PHY_CANNOT_BE_SELECTED      2004
#define CSMI_SAS_SELECT_PHY_OR_PORT          2005
#define CSMI_SAS_PORT_DOES_NOT_EXIST         2006
#define CSMI_SAS_PORT_CANNOT_BE_SELECTED     2007
#define CSMI_SAS_CONNECTION_FAILED           2008

#define CSMI_SAS_NO_SATA_DEVICE              2009
#define CSMI_SAS_NO_SATA_SIGNATURE           2010
#define CSMI_SAS_SCSI_EMULATION              2011
#define CSMI_SAS_NOT_AN_END_DEVICE           2012
#define CSMI_SAS_NO_SCSI_ADDRESS             2013
#define CSMI_SAS_NO_DEVICE_ADDRESS           2014

// Signature value
// (IoctlHeader.Signature)

#define CSMI_SAS_SIGNATURE    "CSMISAS"

// Timeout value default of 60 seconds
// (IoctlHeader.Timeout)

#define CSMI_SAS_TIMEOUT      60

// Device types
// (bDeviceType)

#define CSMI_SAS_PHY_UNUSED               0x00
#define CSMI_SAS_NO_DEVICE_ATTACHED       0x00
#define CSMI_SAS_END_DEVICE               0x10
#define CSMI_SAS_EDGE_EXPANDER_DEVICE     0x20
#define CSMI_SAS_FANOUT_EXPANDER_DEVICE   0x30

// Protocol options
// (bInitiatorPortProtocol, bTargetPortProtocol)

#define CSMI_SAS_PROTOCOL_SATA   0x01
#define CSMI_SAS_PROTOCOL_SMP    0x02
#define CSMI_SAS_PROTOCOL_STP    0x04
#define CSMI_SAS_PROTOCOL_SSP    0x08

// Negotiated and hardware link rates
// (bNegotiatedLinkRate, bMinimumLinkRate, bMaximumLinkRate)

#define CSMI_SAS_LINK_RATE_UNKNOWN  0x00
#define CSMI_SAS_PHY_DISABLED       0x01
#define CSMI_SAS_LINK_RATE_FAILED   0x02
#define CSMI_SAS_SATA_SPINUP_HOLD   0x03
#define CSMI_SAS_SATA_PORT_SELECTOR 0x04
#define CSMI_SAS_LINK_RATE_1_5_GBPS 0x08
#define CSMI_SAS_LINK_RATE_3_0_GBPS 0x09
#define CSMI_SAS_LINK_VIRTUAL       0x10

// Discover state
// (bAutoDiscover)

#define CSMI_SAS_DISCOVER_NOT_SUPPORTED   0x00
#define CSMI_SAS_DISCOVER_NOT_STARTED     0x01
#define CSMI_SAS_DISCOVER_IN_PROGRESS     0x02
#define CSMI_SAS_DISCOVER_COMPLETE        0x03
#define CSMI_SAS_DISCOVER_ERROR           0x04

// Programmed link rates
// (bMinimumLinkRate, bMaximumLinkRate)
// (bProgrammedMinimumLinkRate, bProgrammedMaximumLinkRate)

#define CSMI_SAS_PROGRAMMED_LINK_RATE_UNCHANGED 0x00
#define CSMI_SAS_PROGRAMMED_LINK_RATE_1_5_GBPS  0x08
#define CSMI_SAS_PROGRAMMED_LINK_RATE_3_0_GBPS  0x09

// Link rate
// (bNegotiatedLinkRate in CSMI_SAS_SET_PHY_INFO)

#define CSMI_SAS_LINK_RATE_NEGOTIATE      0x00
#define CSMI_SAS_LINK_RATE_PHY_DISABLED   0x01

// Signal class
// (bSignalClass in CSMI_SAS_SET_PHY_INFO)

#define CSMI_SAS_SIGNAL_CLASS_UNKNOWN     0x00
#define CSMI_SAS_SIGNAL_CLASS_DIRECT      0x01
#define CSMI_SAS_SIGNAL_CLASS_SERVER      0x02
#define CSMI_SAS_SIGNAL_CLASS_ENCLOSURE   0x03

// Link error reset
// (bResetCounts)

#define CSMI_SAS_LINK_ERROR_DONT_RESET_COUNTS   0x00
#define CSMI_SAS_LINK_ERROR_RESET_COUNTS        0x01

// Phy identifier
// (bPhyIdentifier)

#define CSMI_SAS_USE_PORT_IDENTIFIER   0xFF

// Port identifier
// (bPortIdentifier)

#define CSMI_SAS_IGNORE_PORT           0xFF

// Programmed link rates
// (bConnectionRate)

#define CSMI_SAS_LINK_RATE_NEGOTIATED  0x00
#define CSMI_SAS_LINK_RATE_1_5_GBPS    0x08
#define CSMI_SAS_LINK_RATE_3_0_GBPS    0x09

// Connection status
// (bConnectionStatus)

#define CSMI_SAS_OPEN_ACCEPT                          0
#define CSMI_SAS_OPEN_REJECT_BAD_DESTINATION          1
#define CSMI_SAS_OPEN_REJECT_RATE_NOT_SUPPORTED       2
#define CSMI_SAS_OPEN_REJECT_NO_DESTINATION           3
#define CSMI_SAS_OPEN_REJECT_PATHWAY_BLOCKED          4
#define CSMI_SAS_OPEN_REJECT_PROTOCOL_NOT_SUPPORTED   5
#define CSMI_SAS_OPEN_REJECT_RESERVE_ABANDON          6
#define CSMI_SAS_OPEN_REJECT_RESERVE_CONTINUE         7
#define CSMI_SAS_OPEN_REJECT_RESERVE_INITIALIZE       8
#define CSMI_SAS_OPEN_REJECT_RESERVE_STOP             9
#define CSMI_SAS_OPEN_REJECT_RETRY                    10
#define CSMI_SAS_OPEN_REJECT_STP_RESOURCES_BUSY       11
#define CSMI_SAS_OPEN_REJECT_WRONG_DESTINATION        12

// SSP Status
// (bSSPStatus)

#define CSMI_SAS_SSP_STATUS_UNKNOWN     0x00
#define CSMI_SAS_SSP_STATUS_WAITING     0x01
#define CSMI_SAS_SSP_STATUS_COMPLETED   0x02
#define CSMI_SAS_SSP_STATUS_FATAL_ERROR 0x03
#define CSMI_SAS_SSP_STATUS_RETRY       0x04
#define CSMI_SAS_SSP_STATUS_NO_TAG      0x05

// SSP Flags
// (uFlags)

#define CSMI_SAS_SSP_READ           0x00000001
#define CSMI_SAS_SSP_WRITE          0x00000002
#define CSMI_SAS_SSP_UNSPECIFIED    0x00000004

#define CSMI_SAS_SSP_TASK_ATTRIBUTE_SIMPLE         0x00000000
#define CSMI_SAS_SSP_TASK_ATTRIBUTE_HEAD_OF_QUEUE  0x00000010
#define CSMI_SAS_SSP_TASK_ATTRIBUTE_ORDERED        0x00000020
#define CSMI_SAS_SSP_TASK_ATTRIBUTE_ACA            0x00000040

// SSP Data present
// (bDataPresent)

#define CSMI_SAS_SSP_NO_DATA_PRESENT         0x00
#define CSMI_SAS_SSP_RESPONSE_DATA_PRESENT   0x01
#define CSMI_SAS_SSP_SENSE_DATA_PRESENT      0x02

// STP Flags
// (uFlags)

#define CSMI_SAS_STP_READ           0x00000001
#define CSMI_SAS_STP_WRITE          0x00000002
#define CSMI_SAS_STP_UNSPECIFIED    0x00000004
#define CSMI_SAS_STP_PIO            0x00000010
#define CSMI_SAS_STP_DMA            0x00000020
#define CSMI_SAS_STP_PACKET         0x00000040
#define CSMI_SAS_STP_DMA_QUEUED     0x00000080
#define CSMI_SAS_STP_EXECUTE_DIAG   0x00000100
#define CSMI_SAS_STP_RESET_DEVICE   0x00000200

// Task Management Flags
// (uFlags)

#define CSMI_SAS_TASK_IU               0x00000001
#define CSMI_SAS_HARD_RESET_SEQUENCE   0x00000002
#define CSMI_SAS_SUPPRESS_RESULT       0x00000004

// Task Management Functions
// (bTaskManagement)

#define CSMI_SAS_SSP_ABORT_TASK           0x01
#define CSMI_SAS_SSP_ABORT_TASK_SET       0x02
#define CSMI_SAS_SSP_CLEAR_TASK_SET       0x04
#define CSMI_SAS_SSP_LOGICAL_UNIT_RESET   0x08
#define CSMI_SAS_SSP_CLEAR_ACA            0x40
#define CSMI_SAS_SSP_QUERY_TASK           0x80

// Task Management Information
// (uInformation)

#define CSMI_SAS_SSP_TEST           1
#define CSMI_SAS_SSP_EXCEEDED       2
#define CSMI_SAS_SSP_DEMAND         3
#define CSMI_SAS_SSP_TRIGGER        4

// Connector Pinout Information
// (uPinout)

#define CSMI_SAS_CON_UNKNOWN              0x00000001
#define CSMI_SAS_CON_SFF_8482             0x00000002
#define CSMI_SAS_CON_SFF_8470_LANE_1      0x00000100
#define CSMI_SAS_CON_SFF_8470_LANE_2      0x00000200
#define CSMI_SAS_CON_SFF_8470_LANE_3      0x00000400
#define CSMI_SAS_CON_SFF_8470_LANE_4      0x00000800
#define CSMI_SAS_CON_SFF_8484_LANE_1      0x00010000
#define CSMI_SAS_CON_SFF_8484_LANE_2      0x00020000
#define CSMI_SAS_CON_SFF_8484_LANE_3      0x00040000
#define CSMI_SAS_CON_SFF_8484_LANE_4      0x00080000

// Connector Location Information
// (bLocation)

// same as uPinout above...
// #define CSMI_SAS_CON_UNKNOWN              0x01
#define CSMI_SAS_CON_INTERNAL             0x02
#define CSMI_SAS_CON_EXTERNAL             0x04
#define CSMI_SAS_CON_SWITCHABLE           0x08
#define CSMI_SAS_CON_AUTO                 0x10
#define CSMI_SAS_CON_NOT_PRESENT          0x20
#define CSMI_SAS_CON_NOT_CONNECTED        0x80

// Device location identification
// (bIdentify)

#define CSMI_SAS_LOCATE_UNKNOWN           0x00
#define CSMI_SAS_LOCATE_FORCE_OFF         0x01
#define CSMI_SAS_LOCATE_FORCE_ON          0x02

// Location Valid flags
// (uLocationFlags)

#define CSMI_SAS_LOCATE_SAS_ADDRESS_VALID           0x00000001
#define CSMI_SAS_LOCATE_SAS_LUN_VALID               0x00000002
#define CSMI_SAS_LOCATE_ENCLOSURE_IDENTIFIER_VALID  0x00000004
#define CSMI_SAS_LOCATE_ENCLOSURE_NAME_VALID        0x00000008
#define CSMI_SAS_LOCATE_BAY_PREFIX_VALID            0x00000010
#define CSMI_SAS_LOCATE_BAY_IDENTIFIER_VALID        0x00000020
#define CSMI_SAS_LOCATE_LOCATION_STATE_VALID        0x00000040

/* * * * * * * * SAS Phy Control Class IOCTL Constants * * * * * * * * */

// Return codes for SAS Phy Control IOCTL's
// (IoctlHeader.ReturnCode)

// Signature value
// (IoctlHeader.Signature)

#define CSMI_PHY_SIGNATURE    "CSMIPHY"

// Phy Control Functions
// (bFunction)

// values 0x00 to 0xFF are consistent in definition with the SMP PHY CONTROL 
// function defined in the SAS spec
#define CSMI_SAS_PC_NOP                   0x00000000
#define CSMI_SAS_PC_LINK_RESET            0x00000001
#define CSMI_SAS_PC_HARD_RESET            0x00000002
#define CSMI_SAS_PC_PHY_DISABLE           0x00000003
// 0x04 to 0xFF reserved...
#define CSMI_SAS_PC_GET_PHY_SETTINGS      0x00000100

// Link Flags
#define CSMI_SAS_PHY_ACTIVATE_CONTROL     0x00000001
#define CSMI_SAS_PHY_UPDATE_SPINUP_RATE   0x00000002
#define CSMI_SAS_PHY_AUTO_COMWAKE         0x00000004

// Device Types for Phy Settings
// (bType)
#define CSMI_SAS_UNDEFINED 0x00
#define CSMI_SAS_SATA      0x01
#define CSMI_SAS_SAS       0x02

// Transmitter Flags
// (uTransmitterFlags)
#define CSMI_SAS_PHY_PREEMPHASIS_DISABLED    0x00000001

// Receiver Flags
// (uReceiverFlags)
#define CSMI_SAS_PHY_EQUALIZATION_DISABLED   0x00000001

// Pattern Flags
// (uPatternFlags)
// #define CSMI_SAS_PHY_ACTIVATE_CONTROL     0x00000001
#define CSMI_SAS_PHY_DISABLE_SCRAMBLING      0x00000002
#define CSMI_SAS_PHY_DISABLE_ALIGN           0x00000004
#define CSMI_SAS_PHY_DISABLE_SSC             0x00000008

#define CSMI_SAS_PHY_FIXED_PATTERN           0x00000010
#define CSMI_SAS_PHY_USER_PATTERN            0x00000020

// Fixed Patterns
// (bFixedPattern)
#define CSMI_SAS_PHY_CJPAT                   0x00000001
#define CSMI_SAS_PHY_ALIGN                   0x00000002

// Type Flags
// (bTypeFlags)
#define CSMI_SAS_PHY_POSITIVE_DISPARITY      0x01
#define CSMI_SAS_PHY_NEGATIVE_DISPARITY      0x02
#define CSMI_SAS_PHY_CONTROL_CHARACTER       0x04

// Miscellaneous
#define SLOT_NUMBER_UNKNOWN   0xFFFF

/*************************************************************************/
/* DATA STRUCTURES                                                       */
/*************************************************************************/

/* * * * * * * * * * Class Independent Structures * * * * * * * * * */

// EDM #pragma CSMI_SAS_BEGIN_PACK(8)
#pragma pack(8)

// CC_CSMI_SAS_DRIVER_INFO

typedef struct _CSMI_SAS_DRIVER_INFO {
   __u8  szName[81];
   __u8  szDescription[81];
   __u16 usMajorRevision;
   __u16 usMinorRevision;
   __u16 usBuildRevision;
   __u16 usReleaseRevision;
   __u16 usCSMIMajorRevision;
   __u16 usCSMIMinorRevision;
} CSMI_SAS_DRIVER_INFO,
  *PCSMI_SAS_DRIVER_INFO;

typedef struct _CSMI_SAS_DRIVER_INFO_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_DRIVER_INFO Information;
} CSMI_SAS_DRIVER_INFO_BUFFER,
  *PCSMI_SAS_DRIVER_INFO_BUFFER;

// CC_CSMI_SAS_CNTLR_CONFIGURATION

typedef struct _CSMI_SAS_PCI_BUS_ADDRESS {
   __u8  bBusNumber;
   __u8  bDeviceNumber;
   __u8  bFunctionNumber;
   __u8  bReserved;
} CSMI_SAS_PCI_BUS_ADDRESS,
  *PCSMI_SAS_PCI_BUS_ADDRESS;

typedef union _CSMI_SAS_IO_BUS_ADDRESS {
   CSMI_SAS_PCI_BUS_ADDRESS PciAddress;
   __u8  bReserved[32];
} CSMI_SAS_IO_BUS_ADDRESS,
  *PCSMI_SAS_IO_BUS_ADDRESS;

typedef struct _CSMI_SAS_CNTLR_CONFIG {
   __u32 uBaseIoAddress;
   struct {
      __u32 uLowPart;
      __u32 uHighPart;
   } BaseMemoryAddress;
   __u32 uBoardID;
   __u16 usSlotNumber;
   __u8  bControllerClass;
   __u8  bIoBusType;
   CSMI_SAS_IO_BUS_ADDRESS BusAddress;
   __u8  szSerialNumber[81];
   __u16 usMajorRevision;
   __u16 usMinorRevision;
   __u16 usBuildRevision;
   __u16 usReleaseRevision;
   __u16 usBIOSMajorRevision;
   __u16 usBIOSMinorRevision;
   __u16 usBIOSBuildRevision;
   __u16 usBIOSReleaseRevision;
   __u32 uControllerFlags;
   __u16 usRromMajorRevision;
   __u16 usRromMinorRevision;
   __u16 usRromBuildRevision;
   __u16 usRromReleaseRevision;
   __u16 usRromBIOSMajorRevision;
   __u16 usRromBIOSMinorRevision;
   __u16 usRromBIOSBuildRevision;
   __u16 usRromBIOSReleaseRevision;
   __u8  bReserved[7];
} CSMI_SAS_CNTLR_CONFIG,
  *PCSMI_SAS_CNTLR_CONFIG;

typedef struct _CSMI_SAS_CNTLR_CONFIG_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_CNTLR_CONFIG Configuration;
} CSMI_SAS_CNTLR_CONFIG_BUFFER,
  *PCSMI_SAS_CNTLR_CONFIG_BUFFER;

// CC_CSMI_SAS_CNTLR_STATUS

typedef struct _CSMI_SAS_CNTLR_STATUS {
   __u32 uStatus;
   __u32 uOfflineReason;
   __u8  bReserved[28];
} CSMI_SAS_CNTLR_STATUS,
  *PCSMI_SAS_CNTLR_STATUS;

typedef struct _CSMI_SAS_CNTLR_STATUS_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_CNTLR_STATUS Status;
} CSMI_SAS_CNTLR_STATUS_BUFFER,
  *PCSMI_SAS_CNTLR_STATUS_BUFFER;

// CC_CSMI_SAS_FIRMWARE_DOWNLOAD

typedef struct _CSMI_SAS_FIRMWARE_DOWNLOAD {
   __u32 uBufferLength;
   __u32 uDownloadFlags;
   __u8  bReserved[32];
   __u16 usStatus;
   __u16 usSeverity;
} CSMI_SAS_FIRMWARE_DOWNLOAD,
  *PCSMI_SAS_FIRMWARE_DOWNLOAD;

typedef struct _CSMI_SAS_FIRMWARE_DOWNLOAD_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_FIRMWARE_DOWNLOAD Information;
   __u8  bDataBuffer[1];
} CSMI_SAS_FIRMWARE_DOWNLOAD_BUFFER,
  *PCSMI_SAS_FIRMWARE_DOWNLOAD_BUFFER;

// CC_CSMI_SAS_RAID_INFO

typedef struct _CSMI_SAS_RAID_INFO {
   __u32 uNumRaidSets;
   __u32 uMaxDrivesPerSet;
   __u8  bReserved[92];
} CSMI_SAS_RAID_INFO,
  *PCSMI_SAS_RAID_INFO;

typedef struct _CSMI_SAS_RAID_INFO_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_RAID_INFO Information;
} CSMI_SAS_RAID_INFO_BUFFER,
  *PCSMI_SAS_RAID_INFO_BUFFER;

// CC_CSMI_SAS_GET_RAID_CONFIG

typedef struct _CSMI_SAS_RAID_DRIVES {
   __u8  bModel[40];
   __u8  bFirmware[8];
   __u8  bSerialNumber[40];
   __u8  bSASAddress[8];
   __u8  bSASLun[8];
   __u8  bDriveStatus;
   __u8  bDriveUsage;
   __u8  bReserved[30];
} CSMI_SAS_RAID_DRIVES,
   *PCSMI_SAS_RAID_DRIVES;

typedef struct _CSMI_SAS_RAID_CONFIG {
   __u32 uRaidSetIndex;
   __u32 uCapacity;
   __u32 uStripeSize;
   __u8  bRaidType;
   __u8  bStatus;
   __u8  bInformation;
   __u8  bDriveCount;
   __u8  bReserved[20];
   CSMI_SAS_RAID_DRIVES Drives[1];
} CSMI_SAS_RAID_CONFIG,
   *PCSMI_SAS_RAID_CONFIG;

typedef struct _CSMI_SAS_RAID_CONFIG_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_RAID_CONFIG Configuration;
} CSMI_SAS_RAID_CONFIG_BUFFER,
  *PCSMI_SAS_RAID_CONFIG_BUFFER;


/* * * * * * * * * * SAS HBA Class Structures * * * * * * * * * */

// CC_CSMI_SAS_GET_PHY_INFO

typedef struct _CSMI_SAS_IDENTIFY {
   __u8  bDeviceType;
   __u8  bRestricted;
   __u8  bInitiatorPortProtocol;
   __u8  bTargetPortProtocol;
   __u8  bRestricted2[8];
   __u8  bSASAddress[8];
   __u8  bPhyIdentifier;
   __u8  bSignalClass;
   __u8  bReserved[6];
} CSMI_SAS_IDENTIFY,
  *PCSMI_SAS_IDENTIFY;

typedef struct _CSMI_SAS_PHY_ENTITY {
   CSMI_SAS_IDENTIFY Identify;
   __u8  bPortIdentifier;
   __u8  bNegotiatedLinkRate;
   __u8  bMinimumLinkRate;
   __u8  bMaximumLinkRate;
   __u8  bPhyChangeCount;
   __u8  bAutoDiscover;
   __u8  bReserved[2];
   CSMI_SAS_IDENTIFY Attached;
} CSMI_SAS_PHY_ENTITY,
  *PCSMI_SAS_PHY_ENTITY;

typedef struct _CSMI_SAS_PHY_INFO {
   __u8  bNumberOfPhys;
   __u8  bReserved[3];
   CSMI_SAS_PHY_ENTITY Phy[32];
} CSMI_SAS_PHY_INFO,
  *PCSMI_SAS_PHY_INFO;

typedef struct _CSMI_SAS_PHY_INFO_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_PHY_INFO Information;
} CSMI_SAS_PHY_INFO_BUFFER,
  *PCSMI_SAS_PHY_INFO_BUFFER;

// CC_CSMI_SAS_SET_PHY_INFO

typedef struct _CSMI_SAS_SET_PHY_INFO {
   __u8  bPhyIdentifier;
   __u8  bNegotiatedLinkRate;
   __u8  bProgrammedMinimumLinkRate;
   __u8  bProgrammedMaximumLinkRate;
   __u8  bSignalClass;
   __u8  bReserved[3];
} CSMI_SAS_SET_PHY_INFO,
  *PCSMI_SAS_SET_PHY_INFO;

typedef struct _CSMI_SAS_SET_PHY_INFO_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_SET_PHY_INFO Information;
} CSMI_SAS_SET_PHY_INFO_BUFFER,
  *PCSMI_SAS_SET_PHY_INFO_BUFFER;

// CC_CSMI_SAS_GET_LINK_ERRORS

typedef struct _CSMI_SAS_LINK_ERRORS {
   __u8  bPhyIdentifier;
   __u8  bResetCounts;
   __u8  bReserved[2];
   __u32 uInvalidDwordCount;
   __u32 uRunningDisparityErrorCount;
   __u32 uLossOfDwordSyncCount;
   __u32 uPhyResetProblemCount;
} CSMI_SAS_LINK_ERRORS,
  *PCSMI_SAS_LINK_ERRORS;

typedef struct _CSMI_SAS_LINK_ERRORS_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_LINK_ERRORS Information;
} CSMI_SAS_LINK_ERRORS_BUFFER,
  *PCSMI_SAS_LINK_ERRORS_BUFFER;

// CC_CSMI_SAS_SMP_PASSTHRU

typedef struct _CSMI_SAS_SMP_REQUEST {
   __u8  bFrameType;
   __u8  bFunction;
   __u8  bReserved[2];
   __u8  bAdditionalRequestBytes[1016];
} CSMI_SAS_SMP_REQUEST,
  *PCSMI_SAS_SMP_REQUEST;

typedef struct _CSMI_SAS_SMP_RESPONSE {
   __u8  bFrameType;
   __u8  bFunction;
   __u8  bFunctionResult;
   __u8  bReserved;
   __u8  bAdditionalResponseBytes[1016];
} CSMI_SAS_SMP_RESPONSE,
  *PCSMI_SAS_SMP_RESPONSE;

typedef struct _CSMI_SAS_SMP_PASSTHRU {
   __u8  bPhyIdentifier;
   __u8  bPortIdentifier;
   __u8  bConnectionRate;
   __u8  bReserved;
   __u8  bDestinationSASAddress[8];
   __u32 uRequestLength;
   CSMI_SAS_SMP_REQUEST Request;
   __u8  bConnectionStatus;
   __u8  bReserved2[3];
   __u32 uResponseBytes;
   CSMI_SAS_SMP_RESPONSE Response;
} CSMI_SAS_SMP_PASSTHRU,
  *PCSMI_SAS_SMP_PASSTHRU;

typedef struct _CSMI_SAS_SMP_PASSTHRU_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_SMP_PASSTHRU Parameters;
} CSMI_SAS_SMP_PASSTHRU_BUFFER,
  *PCSMI_SAS_SMP_PASSTHRU_BUFFER;

// CC_CSMI_SAS_SSP_PASSTHRU

typedef struct _CSMI_SAS_SSP_PASSTHRU {
   __u8  bPhyIdentifier;
   __u8  bPortIdentifier;
   __u8  bConnectionRate;
   __u8  bReserved;
   __u8  bDestinationSASAddress[8];
   __u8  bLun[8];
   __u8  bCDBLength;
   __u8  bAdditionalCDBLength;
   __u8  bReserved2[2];
   __u8  bCDB[16];
   __u32 uFlags;
   __u8  bAdditionalCDB[24];
   __u32 uDataLength;
} CSMI_SAS_SSP_PASSTHRU,
  *PCSMI_SAS_SSP_PASSTHRU;

typedef struct _CSMI_SAS_SSP_PASSTHRU_STATUS {
   __u8  bConnectionStatus;
   __u8  bSSPStatus;
   __u8  bReserved[2];
   __u8  bDataPresent;
   __u8  bStatus;
   __u8  bResponseLength[2];
   __u8  bResponse[256];
   __u32 uDataBytes;
} CSMI_SAS_SSP_PASSTHRU_STATUS,
  *PCSMI_SAS_SSP_PASSTHRU_STATUS;

typedef struct _CSMI_SAS_SSP_PASSTHRU_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_SSP_PASSTHRU Parameters;
   CSMI_SAS_SSP_PASSTHRU_STATUS Status;
   __u8  bDataBuffer[1];
} CSMI_SAS_SSP_PASSTHRU_BUFFER,
  *PCSMI_SAS_SSP_PASSTHRU_BUFFER;

// CC_CSMI_SAS_STP_PASSTHRU

typedef struct _CSMI_SAS_STP_PASSTHRU {
   __u8  bPhyIdentifier;
   __u8  bPortIdentifier;
   __u8  bConnectionRate;
   __u8  bReserved;
   __u8  bDestinationSASAddress[8];
   __u8  bReserved2[4];
   __u8  bCommandFIS[20];
   __u32 uFlags;
   __u32 uDataLength;
} CSMI_SAS_STP_PASSTHRU,
  *PCSMI_SAS_STP_PASSTHRU;

typedef struct _CSMI_SAS_STP_PASSTHRU_STATUS {
   __u8  bConnectionStatus;
   __u8  bReserved[3];
   __u8  bStatusFIS[20];
   __u32 uSCR[16];
   __u32 uDataBytes;
} CSMI_SAS_STP_PASSTHRU_STATUS,
  *PCSMI_SAS_STP_PASSTHRU_STATUS;

typedef struct _CSMI_SAS_STP_PASSTHRU_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_STP_PASSTHRU Parameters;
   CSMI_SAS_STP_PASSTHRU_STATUS Status;
   __u8  bDataBuffer[1];
} CSMI_SAS_STP_PASSTHRU_BUFFER,
  *PCSMI_SAS_STP_PASSTHRU_BUFFER;

// CC_CSMI_SAS_GET_SATA_SIGNATURE

typedef struct _CSMI_SAS_SATA_SIGNATURE {
   __u8  bPhyIdentifier;
   __u8  bReserved[3];
   __u8  bSignatureFIS[20];
} CSMI_SAS_SATA_SIGNATURE,
  *PCSMI_SAS_SATA_SIGNATURE;

typedef struct _CSMI_SAS_SATA_SIGNATURE_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_SATA_SIGNATURE Signature;
} CSMI_SAS_SATA_SIGNATURE_BUFFER,
  *PCSMI_SAS_SATA_SIGNATURE_BUFFER;

// CC_CSMI_SAS_GET_SCSI_ADDRESS

typedef struct _CSMI_SAS_GET_SCSI_ADDRESS_BUFFER {
   IOCTL_HEADER IoctlHeader;
   __u8  bSASAddress[8];
   __u8  bSASLun[8];
   __u8  bHostIndex;
   __u8  bPathId;
   __u8  bTargetId;
   __u8  bLun;
} CSMI_SAS_GET_SCSI_ADDRESS_BUFFER,
   *PCSMI_SAS_GET_SCSI_ADDRESS_BUFFER;

// CC_CSMI_SAS_GET_DEVICE_ADDRESS

typedef struct _CSMI_SAS_GET_DEVICE_ADDRESS_BUFFER {
   IOCTL_HEADER IoctlHeader;
   __u8  bHostIndex;
   __u8  bPathId;
   __u8  bTargetId;
   __u8  bLun;
   __u8  bSASAddress[8];
   __u8  bSASLun[8];
} CSMI_SAS_GET_DEVICE_ADDRESS_BUFFER,
  *PCSMI_SAS_GET_DEVICE_ADDRESS_BUFFER;

// CC_CSMI_SAS_TASK_MANAGEMENT

typedef struct _CSMI_SAS_SSP_TASK_IU {
   __u8  bHostIndex;
   __u8  bPathId;
   __u8  bTargetId;
   __u8  bLun;
   __u32 uFlags;
   __u32 uQueueTag;
   __u32 uReserved;
   __u8  bTaskManagementFunction;
   __u8  bReserved[7];
   __u32 uInformation;
} CSMI_SAS_SSP_TASK_IU,
  *PCSMI_SAS_SSP_TASK_IU;

typedef struct _CSMI_SAS_SSP_TASK_IU_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_SSP_TASK_IU Parameters;
   CSMI_SAS_SSP_PASSTHRU_STATUS Status;
} CSMI_SAS_SSP_TASK_IU_BUFFER,
  *PCSMI_SAS_SSP_TASK_IU_BUFFER;

// CC_CSMI_SAS_GET_CONNECTOR_INFO

typedef struct _CSMI_SAS_GET_CONNECTOR_INFO {
   __u32 uPinout;
   __u8  bConnector[16];
   __u8  bLocation;
   __u8  bReserved[15];
} CSMI_SAS_CONNECTOR_INFO,
  *PCSMI_SAS_CONNECTOR_INFO;

typedef struct _CSMI_SAS_CONNECTOR_INFO_BUFFER {
   IOCTL_HEADER IoctlHeader;
   CSMI_SAS_CONNECTOR_INFO Reference[32];
} CSMI_SAS_CONNECTOR_INFO_BUFFER,
  *PCSMI_SAS_CONNECTOR_INFO_BUFFER;

// CC_CSMI_SAS_GET_LOCATION

typedef struct _CSMI_SAS_LOCATION_IDENTIFIER {
   __u32 bLocationFlags;
   __u8  bSASAddress[8];
   __u8  bSASLun[8];
   __u8  bEnclosureIdentifier[8];
   __u8  bEnclosureName[32];
   __u8  bBayPrefix[32];
   __u8  bBayIdentifier;
   __u8  bLocationState;
   __u8  bReserved[2];
} CSMI_SAS_LOCATION_IDENTIFIER,
  *PCSMI_SAS_LOCATION_IDENTIFIER;

typedef struct _CSMI_SAS_GET_LOCATION_BUFFER {
   IOCTL_HEADER IoctlHeader;
   __u8  bHostIndex;
   __u8  bPathId;
   __u8  bTargetId;
   __u8  bLun;
   __u8  bIdentify;
   __u8  bNumberOfLocationIdentifiers;
   __u8  bLengthOfLocationIdentifier;
   CSMI_SAS_LOCATION_IDENTIFIER Location[1];
} CSMI_SAS_GET_LOCATION_BUFFER,
  *PCSMI_SAS_GET_LOCATION_BUFFER;

// CC_CSMI_SAS_PHY_CONTROL

typedef struct _CSMI_SAS_CHARACTER {
   __u8  bTypeFlags;
   __u8  bValue;
} CSMI_SAS_CHARACTER,
  *PCSMI_SAS_CHARACTER;

typedef struct _CSMI_SAS_PHY_CONTROL {
   __u8  bType;
   __u8  bRate;
   __u8  bReserved[6];
   __u32 uVendorUnique[8];
   __u32 uTransmitterFlags;
   __i8  bTransmitAmplitude;
   __i8  bTransmitterPreemphasis;
   __i8  bTransmitterSlewRate;
   __i8  bTransmitterReserved[13];
   __u8  bTransmitterVendorUnique[64];
   __u32 uReceiverFlags;
   __i8  bReceiverThreshold;
   __i8  bReceiverEqualizationGain;
   __i8  bReceiverReserved[14];
   __u8  bReceiverVendorUnique[64];
   __u32 uPatternFlags;
   __u8  bFixedPattern;
   __u8  bUserPatternLength;
   __u8  bPatternReserved[6];
   CSMI_SAS_CHARACTER UserPatternBuffer[16];
} CSMI_SAS_PHY_CONTROL,
  *PCSMI_SAS_PHY_CONTROL;

typedef struct _CSMI_SAS_PHY_CONTROL_BUFFER {
   IOCTL_HEADER IoctlHeader;
   __u32 uFunction;
   __u8  bPhyIdentifier;
   __u16 usLengthOfControl;
   __u8  bNumberOfControls;
   __u8  bReserved[4];
   __u32 uLinkFlags;
   __u8  bSpinupRate;
   __u8  bLinkReserved[7];
   __u32 uVendorUnique[8];
   CSMI_SAS_PHY_CONTROL Control[1];
} CSMI_SAS_PHY_CONTROL_BUFFER,
  *PCSMI_SAS_PHY_CONTROL_BUFFER;

// EDM #pragma CSMI_SAS_END_PACK
#pragma pack()

#endif // _CSMI_SAS_H_
