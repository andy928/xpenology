#ifndef __CORE_SAT_H__
#define __CORE_SAT_H__

#include "core_inter.h"

// ATA Specification Command Register Values (Commands)
#define ATA_IDENTIFY_DEVICE             0xec                                              
#define ATA_IDENTIFY_PACKET_DEVICE      0xa1
#define ATA_SMART_CMD                   0xb0
#define ATA_CHECK_POWER_MODE            0xe5

// ATA Specification Feature Register Values (SMART Subcommands).
// Note that some are obsolete as of ATA-7.
#define ATA_SMART_READ_VALUES           0xd0
#define ATA_SMART_READ_THRESHOLDS       0xd1
#define ATA_SMART_AUTOSAVE              0xd2
#define ATA_SMART_SAVE                  0xd3
#define ATA_SMART_IMMEDIATE_OFFLINE     0xd4
#define ATA_SMART_READ_LOG_SECTOR       0xd5
#define ATA_SMART_WRITE_LOG_SECTOR      0xd6
#define ATA_SMART_WRITE_THRESHOLDS      0xd7
#define ATA_SMART_ENABLE                0xd8
#define ATA_SMART_DISABLE               0xd9
#define ATA_SMART_STATUS                0xda
// SFF 8035i Revision 2 Specification Feature Register Value (SMART
// Subcommand)
#define ATA_SMART_AUTO_OFFLINE          0xdb

/* ANSI SCSI-3 Log Pages retrieved by LOG SENSE. */
#define SUPPORTED_LPAGES                        0x00
#define BUFFER_OVERRUN_LPAGE                    0x01
#define WRITE_ERROR_COUNTER_LPAGE               0x02
#define READ_ERROR_COUNTER_LPAGE                0x03
#define READ_REVERSE_ERROR_COUNTER_LPAGE        0x04
#define VERIFY_ERROR_COUNTER_LPAGE              0x05
#define NON_MEDIUM_ERROR_LPAGE                  0x06
#define LAST_N_ERROR_LPAGE                      0x07
#define FORMAT_STATUS_LPAGE                     0x08
#define TEMPERATURE_LPAGE                       0x0d
#define STARTSTOP_CYCLE_COUNTER_LPAGE           0x0e
#define APPLICATION_CLIENT_LPAGE                0x0f
#define SELFTEST_RESULTS_LPAGE                  0x10
#define BACKGROUND_RESULTS_LPAGE                0x15   /* SBC-3 */
#define IE_LPAGE                                0x2f

/* Seagate vendor specific log pages. */
#define SEAGATE_CACHE_LPAGE                     0x37
#define SEAGATE_FACTORY_LPAGE                   0x3e

enum mode_sense_page_code {
        DIRECT_ACCESS_BLOCK_DEVICE_MODE_PAGE    = 0x00,
        RW_ERROR_RECOVERY_MODE_PAGE             = 0x01,
        CACHE_MODE_PAGE                         = 0x08,
        CONTROL_MODE_PAGE                       = 0x0A,
        PORT_MODE_PAGE                          = 0x19,
        INFORMATIONAL_EXCEPTIONS_CONTROL_MODE_PAGE = 0x1C,
        ALL_MODE_PAGE                           = 0x3F,
};

/**************************************************************/
/**
* Self-Test Code translation for Send Diagnostic Command
**/
#define DEFAULT_SELF_TEST					0x0
#define BACKGROUND_SHORT_SELF_TEST			0x1
#define BACKGROUND_EXTENDED_SELF_TEST		0x2
#define ABORT_BACKGROUND_SELF_TEST			0x4
#define FOREGROUND_SHORT_SELF_TEST			0x5
#define FOREGROUND_EXTENDED_SELF_TEST		0x6

void mvScsiReadDefectData(PCore_Driver_Extension pCore, PMV_Request req);
void  mvScsiLogSenseTranslation(PCore_Driver_Extension pCore,  PMV_Request pReq);
MV_BOOLEAN mvScsiModeSelect(PCore_Driver_Extension pCore, PMV_Request pReq);
void mvScsiModeSense(PCore_Driver_Extension pCore, PMV_Request pReq);
void Core_Fill_SendDiagTaskfile( PDomain_Device device,PMV_Request req, PATA_TaskFile taskfile);

#endif /* __CORE_SAT_H__ */
  
