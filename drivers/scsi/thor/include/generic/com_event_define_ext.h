#ifndef COM_EVENT_DEFINE_EXT_H
#define COM_EVENT_DEFINE_EXT_H

//===================================================================================
//===================================================================================
//		All these events are new ones but which are listed in LSI product.
//	Pay attention: All suggested display messages are from from LSI event list.
//  We may make some little change later, especially for new events in Loki.
//===================================================================================
//===================================================================================

//=======================================
//=======================================
//				Event Classes
//=======================================
//=======================================

#define	EVT_CLASS_SAS				7		// SAS, mainly for SAS topology
#define	EVT_CLASS_ENCL				8		// Enclosure
#define	EVT_CLASS_BAT				9       // Battery
#define	EVT_CLASS_FLASH				10      // Flash memory
#define EVT_CLASS_CACHE             11      // Cache related
#define EVT_CLASS_MISC              12      // For other miscellenous events

//=============================================================
//					Event Codes 
//
//	!!!  When adding an EVT_ID, Please put its severity level
//  !!!  and suggested mesage string as comments.  This is the 
//  !!!  only place to document how 'Params' in 'DriverEvent' 
//  !!!  structure is to be used.
//  !!!  Please refer to the EventMessages.doc to get details.
//=============================================================

//
// Event code for EVT_CLASS_SAS (sas)
//

#define EVT_CODE_SAS_LOOP_DETECTED				0  //SAS Topology error: Loop detected
#define EVT_CODE_SAS_UNADDR_DEVICE				1  //SAS Topology error: Unaddressable device
#define EVT_CODE_SAS_MULTIPORT_SAME_ADDR		2  //SAS Topology error: Multiple ports to the same SAS address
#define EVT_CODE_SAS_EXPANDER_ERR				3  //SAS Topology error: Expander error
#define EVT_CODE_SAS_SMP_TIMEOUT				4  //SAS Topology error: SMP timeout
#define EVT_CODE_SAS_OUT_OF_ROUTE_ENTRIES		5  //SAS Topology error: Out of route entries
#define EVT_CODE_SAS_INDEX_NOT_FOUND			6  //SAS Topology error: Index not found
#define EVT_CODE_SAS_SMP_FUNC_FAILED			7  //SAS Topology error: SMP function failed	
#define EVT_CODE_SAS_SMP_CRC_ERR				8  //SAS Topology error: SMP CRC error
#define EVT_CODE_SAS_MULTI_SUBTRACTIVE			9  //SAS Topology error: Multiple subtractive
#define EVT_CODE_SAS_TABEL_TO_TABLE				10 //SAS Topology error: Table to Table
#define EVT_CODE_SAS_MULTI_PATHS				11 //SAS Topology error: Multiple paths
#define EVT_CODE_SAS_WIDE_PORT_LOST_LINK_ON_PHY	12 //SAS wide port %d lost link on PHY %d 
#define EVT_CODE_SAS_WIDE_PORT_REST_LINK_ON_PHY	13 //SAS wide port %d restored link on PHY %d 
#define EVT_CODE_SAS_PHY_EXCEED_ERR_RATE		14 //SAS port %d, PHY %d has exceeded the allowed error rate
#define EVT_CODE_SAS_SATA_MIX_NOT_SUPPORTED		15 //SAS/SATA mixing not supported in enclosure: PD %d disabled

//
// Event code for EVT_CLASS_ENCL (enclosure)
//

#define	EVT_CODE_ENCL_SES_DISCOVERED			0   // Enclosure(SES) discovered on %d
#define	EVT_CODE_ENCL_SAFTE_DISCOVERED			1   // Enclosure(SAFTE) discovered on %d
#define	EVT_CODE_ENCL_COMMUNICATION_LOST		2   // Enclosure %d communication lost
#define	EVT_CODE_ENCL_COMMUNICATION_RESTORED	3   // Enclosure %d communication restored
#define	EVT_CODE_ENCL_FAN_FAILED				4   // Enclosure %d fan %d failed
#define	EVT_CODE_ENCL_FAN_INSERTED				5   // Enclosure %d fan %d inserted
#define	EVT_CODE_ENCL_FAN_REMOVED				6   // Enclosure %d fan %d removed
#define	EVT_CODE_ENCL_PS_FAILED					7   // Enclosure %d power supply %d failed
#define	EVT_CODE_ENCL_PS_INSERTED				8   // Enclosure %d power supply %d inserted
#define	EVT_CODE_ENCL_PS_REMOVED				9   // Enclosure %d power supply %d removed
#define	EVT_CODE_ENCL_SIM_FAILED				10  // Enclosure %d SIM %d failed
#define	EVT_CODE_ENCL_SIM_INSERTED				11  // Enclosure %d SIM %d inserted
#define	EVT_CODE_ENCL_SIM_REMOVED				12  // Enclosure %d SIM %d removed
#define	EVT_CODE_ENCL_TEMP_SENSOR_BELOW_WARNING	13  // Enclosure %d temperature sensor %d below warning threshold
#define	EVT_CODE_ENCL_TEMP_SENSOR_BELOW_ERR		14  // Enclosure %d temperature sensor %d below error threshold
#define	EVT_CODE_ENCL_TEMP_SENSOR_ABOVE_WARNING	15  // Enclosure %d temperature sensor %d above warning threshold
#define	EVT_CODE_ENCL_TEMP_SENSOR_ABOVE_ERR		16  // Enclosure %d temperature sensor %d above error threshold 
#define EVT_CODE_ENCL_SHUTDOWN					17  // Enclosure %d shutdown
#define EVT_CODE_ENCL_NOT_SUPPORTED				18  // Enclosure %d not supported; too many enclosures connected to port
#define	EVT_CODE_ENCL_FW_MISMATCH				19  // Enclosure %d firmware mismatch 
#define	EVT_CODE_ENCL_SENSOR_BAD				20  // Enclosure %d sensor %d bad
#define	EVT_CODE_ENCL_PHY_BAD					21  // Enclosure %d phy %d bad
#define	EVT_CODE_ENCL_IS_UNSTABLE				22  // Enclosure %d is unstable
#define	EVT_CODE_ENCL_HW_ERR					23  // Enclosure %d hardware error
#define	EVT_CODE_ENCL_NOT_RESPONDING			24  // Enclosure %d not responding
#define	EVT_CODE_ENCL_HOTPLUG_DETECTED			25  // Enclosure(SES) hotplug on %d was detected, but is not supported 
#define	EVT_CODE_ENCL_PS_SWITCHED_OFF			26  // Enclosure %d Power supply %d switched off
#define	EVT_CODE_ENCL_PS_SWITCHED_ON			27  // Enclosure %d Power supply %d switched on
#define	EVT_CODE_ENCL_PS_CABLE_REMOVED			28  // Enclosure %d Power supply %d cable removed
#define	EVT_CODE_ENCL_PS_CABLE_INSERTED			29  // Enclosure %d Power supply %d cable inserted
#define	EVT_CODE_ENCL_FAN_RETURN_TO_NORMAL		30  // Enclosure %d Fan %d returned to normal
#define	EVT_CODE_ENCL_TEMP_RETURN_TO_NORMAL		31  // Enclosure %d Temperature %d returned to normal
#define	EVT_CODE_ENCL_FW_DWLD_IN_PRGS			32  // Enclosure %d Firmware download in progress
#define	EVT_CODE_ENCL_FW_DWLD_FAILED			33  // Enclosure %d Firmware download failed
#define	EVT_CODE_ENCL_TEMP_SENSOR_DIFF_DETECTED	34  // Enclosure %d Temperature sensor %d differential detected
#define	EVT_CODE_ENCL_FAN_SPEED_CHANGED			35  // Enclosure %d fan %d speed changed


//
// Event code for EVT_CLASS_BAT
//

#define EVT_CODE_BAT_PRESENT					0	// Battery present
#define EVT_CODE_BAT_NOT_PRESENT				1   // Battery not present
#define EVT_CODE_BAT_NEW_BAT_DETECTED			2   // New battery detected
#define EVT_CODE_BAT_REPLACED					3   // Battery has been replaced 
#define EVT_CODE_BAT_TEMP_IS_HIGH				4   // Battery temperature is high (%dC)
#define EVT_CODE_BAT_VOLTAGE_LOW				5   // Battery voltage low (%f V)
#define EVT_CODE_BAT_STARTED_CHARGING			6   // Battery started charging
#define EVT_CODE_BAT_DISCHARGING				7   // Battery is discharging
#define EVT_CODE_BAT_TEMP_IS_NORMAL				8	// Battery temperature is normal
#define EVT_CODE_BAT_NEED_REPLACE				9	// Battery needs to be replacement, SOH bad
#define EVT_CODE_BAT_RELEARN_STARTED			10  // Battery relearn started
#define EVT_CODE_BAT_RELEARN_IN_PGRS			11  // Battery relearn in progress
#define EVT_CODE_BAT_RELEARN_COMPLETED			12  // Battery relearn completed
#define EVT_CODE_BAT_RELEARN_TIMED_OUT			13  // Battery relearn timed out
#define EVT_CODE_BAT_RELEARN_PENDING			14  // Battery relearn pending: Battery is under charge
#define EVT_CODE_BAT_RELEARN_POSTPONED			15  // Battery relearn postponed
#define EVT_CODE_BAT_START_IN_4_DAYS			16  // Battery relearn will start in 4 days
#define EVT_CODE_BAT_START_IN_2_DAYS			17	// Battery relearn will start in 2 days
#define EVT_CODE_BAT_START_IN_1_DAY				18	// Battery relearn will start in 1 days
#define EVT_CODE_BAT_START_IN_5_HOURS			19	// Battery relearn will start in 5 hours
#define EVT_CODE_BAT_REMOVED					20  // Battery removed
#define EVT_CODE_BAT_CHARGE_CMPLT				21  // Battery charged complete
#define EVT_CODE_BAT_CHARGER_PROBLEM_DETECTED	22  // Battery/charger problems detected: SOH bad
#define EVT_CODE_BAT_CAPACITY_BELOW_THRESHOLD	23  // Current capacity (%d) of the battery is below threshold (%d)
#define EVT_CODE_BAT_CAPACITY_ABOVE_THRESHOLD	24  // Current capacity (%d) of the battery is above threshold (%d)



//
// Event code for EVT_CLASS_FLASH
//

#define EVT_CODE_FLASH_DWLDED_IMAGE_CORRUPTED	0	// Flash downloaded image corrupt
#define EVT_CODE_FLASH_ERASE_ERR				1   // Flash erase error
#define EVT_CODE_FLASH_ERASE_TIMEOUT			2   // Flash timeout during erase
#define EVT_CODE_FLASH_FLASH_ERR				3	// Flash error
#define EVT_CODE_FLASHING_IMAGE					4	// Flashing image: %d
#define EVT_CODE_FLASHING_NEW_IMAGE_DONE		5   // Flash of new firmware images complete
#define EVT_CODE_FLASH_PROGRAMMING_ERR			6   // Flash programming error
#define EVT_CODE_FLASH_PROGRAMMING_TIMEOUT		7   // Flash timeout during programming
#define EVT_CODE_FLASH_UNKNOWN_CHIP_TYPE		8   // Flash chip type unknown 
#define EVT_CODE_FLASH_UNKNOWN_CMD_SET			9   // Flash command set unknown
#define EVT_CODE_FLASH_VERIFY_FAILURE			10  // Flash verify failure
#define EVT_CODE_NVRAM_CORRUPT					11	// NVRAM is corrupt; reinitializing
#define EVT_CODE_NVRAM_MISMACTH_OCCURED			12  // NVRAM mismatch occured


//
// Event code for EVT_CLASS_CACHE(Cache)
//

#define EVT_CODE_CACHE_NOT_RECV_FROM_TBBU		0	// Unable to recover cache data from TBBU
#define EVT_CODE_CACHE_RECVD_FROM_TBBU			1   // Cache data recovered from TBBU successfully
#define EVT_CODE_CACHE_CTRLER_CACHE_DISCARDED	2   // Controller cache discarded due to memory/battery problems
#define EVT_CODE_CACHE_FAIL_RECV_DUETO_MISMATCH	3   // Unable to recover cache data due to configuration mismatch 
#define EVT_CODE_CACHE_DIRTY_DATA_DISCARDED		4	// Dirty cache data discarded by user
#define EVT_CODE_CACHE_FLUSH_RATE_CHANGED		5   // Flush rate changed to %d seconds.


//
// Event code for EVT_CLASS_MISC
//

#define EVT_CODE_MISC_CONFIG_CLEARED				0	// Configuration cleared
#define EVT_CODE_MISC_CHANGE_BACK_ACTIVITY_RATE		1	// Background activity rate changed to %d%%
#define EVT_CODE_MISC_FATAL_FW_ERR					2   // Fatal firmware error: %d
#define EVT_CODE_MISC_FACTORY_DEFAULTS_RESTORED		3   // Factory defaults restored
#define EVT_CODE_MISC_GET_HIBER_CMD					4   // Hibernation command received from host
#define EVT_CODE_MISC_MUTLI_BIT_ECC_ERR				5	// Multi-bit ECC error: ECAR=%x ELOG=%x, (%d)
#define EVT_CODE_MISC_SINGLE_BIT_ECC_ERR			6   // Single-bit ECC error: ECAR=%x ELOG=%x, (%d)
#define EVT_CODE_MISC_GET_SHUTDOWN_CMD				7	// Shutdown command received from host
#define EVT_CODE_MISC_TIME_ESTABLISHED				8	// Time established as %d; (%d seconds since power on)
#define EVT_CODE_MISC_USER_ENTERED_DEBUGGER			9   // User entered firmware debugger

#define EVT_CODE_MISC_FORMAT_COMPLETE				10	// Format complete on %d
#define EVT_CODE_MISC_FORMAT_STARTED				11	// Format started on %d 
#define EVT_CODE_MISC_REASSIGN_WRITE_OP				12	// Reassign write operation on %d is %d
#define EVT_CODE_MISC_UNEXPECTED_SENSE				13	// Unexpected sense: %d, CDB%d, Sense: %d
#define EVT_CODE_MISC_REPLACED_MISSING				14	// Replaced missing as %d on array %d row %d
#define EVT_CODE_MISC_NOT_A_CERTIFIED_DRIVE			15  // %d is not a certificated derive

/* May put into other group???*/
#define EVT_CODE_MISC_PD_MISSING_FROM_CONFIG_AT_BOOT	16	// PDs missing from configuration on boot	
#define EVT_CODE_MISC_VD_MISSING_DRIVES					17  // VDs missing drives and will go offline at boot: %d
#define EVT_CODE_MISC_VD_MISSING_AT_BOOT				18  // VDs missing at boot: %d
#define EVT_CODE_MISC_PREVIOUS_CONFIG_MISSING_AT_BOOT	19  // Previous configuration completely missing at boot
#define EVT_CODE_MISC_PD_TOO_SMALL_FOR_AUTOREBUILD		20  // PD too small to be used for auto-rebuild on %d.

//=======================================
//=======================================
//				Event IDs
//=======================================
//=======================================

//
// Event id for EVT_CLASS_SAS
//

#define _CLASS_SAS(x)                (EVT_CLASS_SAS << 16 | (x))

#define EVT_ID_SAS_LOOP_DETECTED					_CLASS_SAS(EVT_CODESAS_LOOP_DETECTED)
#define EVT_ID_SAS_UNADDR_DEVICE					_CLASS_SAS(EVT_CODESAS_UNADDR_DEVICE)			
#define EVT_ID_SAS_MULTIPORT_SAME_ADDR				_CLASS_SAS(EVT_CODESAS_MULTIPORT_SAME_ADDR)
#define EVT_ID_SAS_EXPANDER_ERR						_CLASS_SAS(EVT_CODESAS_EXPANDER_ERR)
#define EVT_ID_SAS_SMP_TIMEOUT						_CLASS_SAS(EVT_CODESAS_SMP_TIMEOUT)
#define EVT_ID_SAS_OUT_OF_ROUTE_ENTRIES				_CLASS_SAS(EVT_CODESAS_OUT_OF_ROUTE_ENTRIES)
#define EVT_ID_SAS_INDEX_NOT_FOUND					_CLASS_SAS(EVT_CODESAS_INDEX_NOT_FOUND)
#define EVT_ID_SAS_SMP_FUNC_FAILED					_CLASS_SAS(EVT_CODESAS_SMP_FUNC_FAILED)
#define EVT_ID_SAS_SMP_CRC_ERR						_CLASS_SAS(EVT_CODESAS_SMP_CRC_ERR)
#define EVT_ID_SAS_MULTI_SUBTRACTIVE				_CLASS_SAS(EVT_CODESAS_MULTI_SUBTRACTIVE)
#define EVT_ID_SAS_TABEL_TO_TABLE					_CLASS_SAS(EVT_CODESAS_TABEL_TO_TABLE)
#define EVT_ID_SAS_MULTI_PATHS						_CLASS_SAS(EVT_CODESAS_MULTI_PATHS)
#define EVT_ID_SAS_WIDE_PORT_LOST_LINK_ON_PHY		_CLASS_SAS(EVT_CODESAS_WIDE_PORT_LOST_LINK_ON_PHY)
#define EVT_ID_SAS_WIDE_PORT_REST_LINK_ON_PHY		_CLASS_SAS(EVT_CODESAS_WIDE_PORT_REST_LINK_ON_PHY)
#define EVT_ID_SAS_PHY_EXCEED_ERR_RATE				_CLASS_SAS(EVT_CODESAS_PHY_EXCEED_ERR_RATE)
#define EVT_ID_SAS_SATA_MIX_NOT_SUPPORTED			_CLASS_SAS(EVT_CODESAS_SATA_MIX_NOT_SUPPORTED)

//
// Event id for EVT_CLASS_ENCL (enclosure)
//

#define _CLASS_ENCL(x)                (EVT_CLASS_ENCL << 16 | (x))

#define	EVT_ID_ENCL_SES_DISCOVERED					_CLASS_ENCL(EVT_CODE_ENCL_SES_DISCOVERED)				
#define	EVT_ID_ENCL_SAFTE_DISCOVERED				_CLASS_ENCL(EVT_CODE_ENCL_SAFTE_DISCOVERED)
#define	EVT_ID_ENCL_COMMUNICATION_LOST				_CLASS_ENCL(EVT_CODE_ENCL_COMMUNICATION_LOST)
#define	EVT_ID_ENCL_COMMUNICATION_RESTORED		    _CLASS_ENCL(EVT_CODE_ENCL_COMMUNICATION_RESTORED)
#define	EVT_ID_ENCL_FAN_FAILED						_CLASS_ENCL(EVT_CODE_ENCL_FAN_FAILED)
#define	EVT_ID_ENCL_FAN_INSERTED					_CLASS_ENCL(EVT_CODE_ENCL_FAN_INSERTED)
#define	EVT_ID_ENCL_FAN_REMOVED						_CLASS_ENCL(EVT_CODE_ENCL_FAN_REMOVED)
#define	EVT_ID_ENCL_PS_FAILED						_CLASS_ENCL(EVT_CODE_ENCL_PS_FAILED)
#define	EVT_ID_ENCL_PS_INSERTED						_CLASS_ENCL(EVT_CODE_ENCL_PS_INSERTED)
#define	EVT_ID_ENCL_PS_REMOVED						_CLASS_ENCL(EVT_CODE_ENCL_PS_REMOVED)
#define	EVT_ID_ENCL_SIM_FAILED						_CLASS_ENCL(EVT_CODE_ENCL_SIM_FAILED)
#define	EVT_ID_ENCL_SIM_INSERTED					_CLASS_ENCL(EVT_CODE_ENCL_SIM_INSERTED)
#define	EVT_ID_ENCL_SIM_REMOVED						_CLASS_ENCL(EVT_CODE_ENCL_SIM_REMOVED)
#define	EVT_ID_ENCL_TEMP_SENSOR_BELOW_WARNING		_CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_BELOW_WARNING)
#define	EVT_ID_ENCL_TEMP_SENSOR_BELOW_ERR			_CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_BELOW_ERR)
#define	EVT_ID_ENCL_TEMP_SENSOR_ABOVE_WARNING		_CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_ABOVE_WARNING)
#define	EVT_ID_ENCL_TEMP_SENSOR_ABOVE_ERR			_CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_ABOVE_ERR)
#define EVT_ID_ENCL_SHUTDOWN						_CLASS_ENCL(EVT_CODE_ENCL_SHUTDOWN)
#define EVT_ID_ENCL_NOT_SUPPORTED				    _CLASS_ENCL(EVT_CODE_ENCL_NOT_SUPPORTED)
#define	EVT_ID_ENCL_FW_MISMATCH						_CLASS_ENCL(EVT_CODE_ENCL_FW_MISMATCH)
#define	EVT_ID_ENCL_SENSOR_BAD						_CLASS_ENCL(EVT_CODE_ENCL_SENSOR_BAD)
#define	EVT_ID_ENCL_PHY_BAD							_CLASS_ENCL(EVT_CODE_ENCL_PHY_BAD)
#define	EVT_ID_ENCL_IS_UNSTABLE						_CLASS_ENCL(EVT_CODE_ENCL_IS_UNSTABLE)
#define	EVT_ID_ENCL_HW_ERR							_CLASS_ENCL(EVT_CODE_ENCL_HW_ERR)
#define	EVT_ID_ENCL_NOT_RESPONDING					_CLASS_ENCL(EVT_CODE_ENCL_NOT_RESPONDING)
#define	EVT_ID_ENCL_HOTPLUG_DETECTED				_CLASS_ENCL(EVT_CODE_ENCL_HOTPLUG_DETECTED)
#define	EVT_ID_ENCL_PS_SWITCHED_OFF					_CLASS_ENCL(EVT_CODE_ENCL_PS_SWITCHED_OFF	)	
#define	EVT_ID_ENCL_PS_SWITCHED_ON					_CLASS_ENCL(EVT_CODE_ENCL_PS_SWITCHED_ON)
#define	EVT_ID_ENCL_PS_CABLE_REMOVED				_CLASS_ENCL(EVT_CODE_ENCL_PS_CABLE_REMOVED)
#define	EVT_ID_ENCL_PS_CABLE_INSERTED				_CLASS_ENCL(EVT_CODE_ENCL_PS_CABLE_INSERTED)
#define	EVT_ID_ENCL_FAN_RETURN_TO_NORMAL			_CLASS_ENCL(EVT_CODE_ENCL_FAN_RETURN_TO_NORMAL)	
#define	EVT_ID_ENCL_TEMP_RETURN_TO_NORMAL			_CLASS_ENCL(EVT_CODE_ENCL_TEMP_RETURN_TO_NORMAL)		
#define	EVT_ID_ENCL_FW_DWLD_IN_PRGS					_CLASS_ENCL(EVT_CODE_ENCL_FW_DWLD_IN_PRGS	)
#define	EVT_ID_ENCL_FW_DWLD_FAILED					_CLASS_ENCL(EVT_CODE_ENCL_FW_DWLD_FAILED)	
#define	EVT_ID_ENCL_TEMP_SENSOR_DIFF_DETECTED		_CLASS_ENCL(EVT_CODE_ENCL_TEMP_SENSOR_DIFF_DETECTED)
#define	EVT_ID_ENCL_FAN_SPEED_CHANGED				_CLASS_ENCL(EVT_CODE_ENCL_FAN_SPEED_CHANGED)

//
// Event id for EVT_CLASS_BAT
//

#define _CLASS_BAT(x)                (EVT_CLASS_BAT << 16 | (x))

#define EVT_ID_BAT_PRESENT						_CLASS_BAT(EVT_CODE_BAT_PRESENT)
#define EVT_ID_BAT_NOT_PRESENT					_CLASS_BAT(EVT_CODE_BAT_NOT_PRESENT)
#define EVT_ID_BAT_NEW_BAT_DETECTED				_CLASS_BAT(EVT_CODE_BAT_NEW_BAT_DETECTED)
#define EVT_ID_BAT_REPLACED						_CLASS_BAT(EVT_CODE_BAT_REPLACED)
#define EVT_ID_BAT_TEMP_IS_HIGH					_CLASS_BAT(EVT_CODE_BAT_TEMP_IS_HIGH)
#define EVT_ID_BAT_VOLTAGE_LOW					_CLASS_BAT(EVT_CODE_BAT_VOLTAGE_LOW)
#define EVT_ID_BAT_STARTED_CHARGING				_CLASS_BAT(EVT_CODE_BAT_STARTED_CHARGING)
#define EVT_ID_BAT_DISCHARGING					_CLASS_BAT(EVT_CODE_BAT_DISCHARGING)
#define EVT_ID_BAT_TEMP_IS_NORMAL				_CLASS_BAT(EVT_CODE_BAT_TEMP_IS_NORMAL)
#define EVT_ID_BAT_NEED_REPLACE					_CLASS_BAT(EVT_CODE_BAT_NEED_REPLACE)
#define EVT_ID_BAT_RELEARN_STARTED				_CLASS_BAT(EVT_CODE_BAT_RELEARN_STARTED)
#define EVT_ID_BAT_RELEARN_IN_PGRS				_CLASS_BAT(EVT_CODE_BAT_RELEARN_IN_PGRS)
#define EVT_ID_BAT_RELEARN_COMPLETED			_CLASS_BAT(EVT_CODE_BAT_RELEARN_COMPLETED)
#define EVT_ID_BAT_RELEARN_TIMED_OUT			_CLASS_BAT(EVT_CODE_BAT_RELEARN_TIMED_OUT)
#define EVT_ID_BAT_RELEARN_PENDING				_CLASS_BAT(EVT_CODE_BAT_RELEARN_PENDING)
#define EVT_ID_BAT_RELEARN_POSTPONED			_CLASS_BAT(EVT_CODE_BAT_RELEARN_POSTPONED)
#define EVT_ID_BAT_START_IN_4_DAYS				_CLASS_BAT(EVT_CODE_BAT_START_IN_4_DAYS)
#define EVT_ID_BAT_START_IN_2_DAYS				_CLASS_BAT(EVT_CODE_BAT_START_IN_2_DAYS)
#define EVT_ID_BAT_START_IN_1_DAY				_CLASS_BAT(EVT_CODE_BAT_START_IN_1_DAY)
#define EVT_ID_BAT_START_IN_5_HOURS				_CLASS_BAT(EVT_CODE_BAT_START_IN_5_HOURS)
#define EVT_ID_BAT_REMOVED						_CLASS_BAT(EVT_CODE_BAT_REMOVED)
#define EVT_ID_BAT_CHARGE_CMPLT					_CLASS_BAT(EVT_CODE_BAT_CHARGE_CMPLT)
#define EVT_ID_BAT_CHARGER_PROBLEM_DETECTED		_CLASS_BAT(EVT_CODE_BAT_CHARGER_PROBLEM_DETECTED)
#define EVT_ID_BAT_CAPACITY_BELOW_THRESHOLD		_CLASS_BAT(EVT_CODE_BAT_CAPACITY_BELOW_THRESHOLD)
#define EVT_ID_BAT_CAPACITY_ABOVE_THRESHOLD		_CLASS_BAT(EVT_CODE_BAT_CAPACITY_ABOVE_THRESHOLD)

//
// Event id for EVT_CLASS_FLASH
//
#define _CLASS_FLASH(x)                (EVT_CLASS_FLASH << 16 | (x))

#define EVT_ID_FLASH_DWLDED_IMAGE_CORRUPTED		_CLASS_FLASH(EVT_CODE_FLASH_DWLDED_IMAGE_CORRUPTED)
#define EVT_ID_FLASH_ERASE_ERR					_CLASS_FLASH(EVT_CODE_FLASH_ERASE_ERR)
#define EVT_ID_FLASH_ERASE_TIMEOUT				_CLASS_FLASH(EVT_CODE_FLASH_ERASE_TIMEOUT)
#define EVT_ID_FLASH_FLASH_ERR					_CLASS_FLASH(EVT_CODE_FLASH_FLASH_ERR)
#define EVT_ID_FLASHING_IMAGE					_CLASS_FLASH(EVT_CODE_FLASHING_IMAGE)
#define EVT_ID_FLASHING_NEW_IMAGE_DONE			_CLASS_FLASH(EVT_CODE_FLASHING_NEW_IMAGE_DONE)
#define EVT_ID_FLASH_PROGRAMMING_ERR			_CLASS_FLASH(EVT_CODE_FLASH_PROGRAMMING_ERR)
#define EVT_ID_FLASH_PROGRAMMING_TIMEOUT		_CLASS_FLASH(EVT_CODE_FLASH_PROGRAMMING_TIMEOUT)
#define EVT_ID_FLASH_UNKNOWN_CHIP_TYPE			_CLASS_FLASH(EVT_CODE_FLASH_UNKNOWN_CHIP_TYPE)
#define EVT_ID_FLASH_UNKNOWN_CMD_SET			_CLASS_FLASH(EVT_CODE_FLASH_UNKNOWN_CMD_SET)
#define EVT_ID_FLASH_VERIFY_FAILURE				_CLASS_FLASH(EVT_CODE_FLASH_VERIFY_FAILURE)
#define EVT_ID_NVRAM_CORRUPT					_CLASS_FLASH(EVT_CODE_NVRAM_CORRUPT)
#define EVT_ID_NVRAM_MISMACTH_OCCURED			_CLASS_FLASH(EVT_CODE_NVRAM_MISMACTH_OCCURED)


// Event code for EVT_CLASS_CACHE(Cache)
//

#define _CLASS_CACHE(x)                (EVT_CLASS_CACHE << 16 | (x))

#define EVT_ID_CACHE_NOT_RECV_FROM_TBBU			_CLASS_CACHE(EVT_CODE_CACHE_NOT_RECV_FROM_TBBU)
#define EVT_ID_CACHE_RECVD_FROM_TBBU			_CLASS_CACHE(EVT_CODE_CACHE_RECVD_FROM_TBBU)
#define EVT_ID_CACHE_CTRLER_CACHE_DISCARDED		_CLASS_CACHE(EVT_CODE_CACHE_CTRLER_CACHE_DISCARDED)
#define EVT_ID_CACHE_FAIL_RECV_DUETO_MISMATCH	_CLASS_CACHE(EVT_CODE_CACHE_FAIL_RECV_DUETO_MISMATCH)
#define EVT_ID_CACHE_DIRTY_DATA_DISCARDED		_CLASS_CACHE(EVT_CODE_CACHE_DIRTY_DATA_DISCARDED)
#define EVT_ID_CACHE_FLUSH_RATE_CHANGED			_CLASS_CACHE(EVT_CODE_CACHE_FLUSH_RATE_CHANGED)


//
// Event code for EVT_CLASS_MISC
//

#define _CLASS_MISC(x)                (EVT_CLASS_MISC << 16 | (x))

#define EVT_ID_MISC_CONFIG_CLEARED					_CLASS_MISC(EVT_CODE_MISC_CONFIG_CLEARED)		
#define EVT_ID_MISC_CHANGE_BACK_ACTIVITY_RATE		_CLASS_MISC(EVT_CODE_MISC_CHANGE_BACK_ACTIVITY_RATE)
#define EVT_ID_MISC_FATAL_FW_ERR					_CLASS_MISC(EVT_CODE_MISC_FATAL_FW_ERR)
#define EVT_ID_MISC_FACTORY_DEFAULTS_RESTORED		_CLASS_MISC(EVT_CODE_MISC_FACTORY_DEFAULTS_RESTORED)
#define EVT_ID_MISC_GET_HIBER_CMD					_CLASS_MISC(EVT_CODE_MISC_GET_HIBER_CMD)
#define EVT_ID_MISC_MUTLI_BIT_ECC_ERR				_CLASS_MISC(EVT_CODE_MISC_MUTLI_BIT_ECC_ERR)
#define EVT_ID_MISC_SINGLE_BIT_ECC_ERR				_CLASS_MISC(EVT_CODE_MISC_SINGLE_BIT_ECC_ERR)
#define EVT_ID_MISC_GET_SHUTDOWN_CMD				_CLASS_MISC(EVT_CODE_MISC_GET_SHUTDOWN_CMD)
#define EVT_ID_MISC_TIME_ESTABLISHED				_CLASS_MISC(EVT_CODE_MISC_TIME_ESTABLISHED)
#define EVT_ID_MISC_USER_ENTERED_DEBUGGER			_CLASS_MISC(EVT_CODE_MISC_USER_ENTERED_DEBUGGER)
#define EVT_ID_MISC_FORMAT_COMPLETE					_CLASS_MISC(EVT_CODE_MISC_FORMAT_COMPLETE)
#define EVT_ID_MISC_FORMAT_STARTED					_CLASS_MISC(EVT_CODE_MISC_FORMAT_STARTED)
#define EVT_ID_MISC_REASSIGN_WRITE_OP				_CLASS_MISC(EVT_CODE_MISC_REASSIGN_WRITE_OP)
#define EVT_ID_MISC_UNEXPECTED_SENSE				_CLASS_MISC(EVT_CODE_MISC_UNEXPECTED_SENSE)
#define EVT_ID_MISC_REPLACED_MISSING				_CLASS_MISC(EVT_CODE_MISC_REPLACED_MISSING)
#define EVT_ID_MISC_NOT_A_CERTIFIED_DRIVE			_CLASS_MISC(EVT_CODE_MISC_NOT_A_CERTIFIED_DRIVE)

/* May put into other group???*/
#define EVT_ID_MISC_PD_MISSING_FROM_CONFIG_AT_BOOT	_CLASS_MISC(EVT_CODE_MISC_PD_MISSING_FROM_CONFIG_AT_BOOT)
#define EVT_ID_MISC_VD_MISSING_DRIVES				_CLASS_MISC(EVT_CODE_MISC_VD_MISSING_DRIVES)
#define EVT_ID_MISC_VD_MISSING_AT_BOOT				_CLASS_MISC(EVT_CODE_MISC_VD_MISSING_AT_BOOT)
#define EVT_ID_MISC_PREVIOUS_CONFIG_MISSING_AT_BOOT _CLASS_MISC(EVT_CODE_MISC_PREVIOUS_CONFIG_MISSING_AT_BOOT)
#define EVT_ID_MISC_PD_TOO_SMALL_FOR_AUTOREBUILD	_CLASS_MISC(EVT_CODE_MISC_PD_TOO_SMALL_FOR_AUTOREBUILD)

#endif

