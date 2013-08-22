/**
 ***************************************************************************
 * SCSI command definitions
 ***************************************************************************
 */

#define SC_TEST_UNIT_READY		(0x00)
#define SC_REWIND    			(0x01)
#define SC_REQUEST_SENSE		(0x03)
#define SC_FORMAT_UNIT			(0x04)
#define SC_READ_BLOCK_LIMITS		(0x05)
#define SC_REASSIGN_BLOCKS		(0x07)
#define SC_INIT_ELEM_STATUS		(0x07)
#define SC_READ6			(0x08)
#define SC_WRITE6			(0x0A)
#define SC_SEEK6			(0x0b)
#define SC_READ_REVERSE			(0x0f)
#define SC_WRITE_FILEMARKS		(0x10)
#define SC_SPACE			(0x11)
#define SC_INQUIRY			(0x12)
#define SC_VERIFY6			(0x13)
#define SC_RECOVER_BUF_DATA		(0x14)
#define SC_MODE_SELECT6			(0x15)
#define SC_RESERVE			(0x16)
#define SC_RELEASE			(0x17)
#define SC_COPY				(0x18)
#define SC_ERASE6			(0x19)
#define SC_MODE_SENSE6			(0x1A)
#define SC_START_STOP_UNIT		(0x1B)
#define SC_LOAD_UNLOAD			(0x1b)
#define SC_RECEIVE_DIAG_RESULTS 	(0x1c)
#define SC_SEND_DIAGNOSTICS		(0x1d)
#define SC_PREV_ALLOW_REMOVAL		(0x1e)
#define SC_READ_FORMAT_CAPACITIES	(0x23)
#define SC_SET_WINDOW			(0x24)
#define SC_READ_CAPACITY10		(0x25)
#define SC_READ10			(0x28)
#define SC_READ_GENERATION		(0x29)
#define SC_WRITE10			(0x2A)
#define SC_SEEK10			(0x2B)
#define SC_ERASE10			(0x2c)
#define SC_READ_UPDATED_BLOCK		(0x2d)
#define SC_WRITE_AND_VERIFY10		(0x2e)
#define SC_VERIFY10			(0x2f)
#define SC_SEARCH_DATA_HIGH10		(0x30)
#define SC_SEARCH_DATA_EQUAL10		(0x31)
#define SC_SEARCH_DATA_LOW10		(0x32)
#define SC_SET_LIMITS10			(0x33)
#define SC_PREFETCH10			(0x34)
#define SC_SYNC_CACHE			(0x35)
#define SC_LOCK_UNLOCK_CACHE10		(0x36)
#define SC_READ_DEFECT_DATA10		(0x37)
#define SC_INIT_ELEM_STATUS10		(0x37)
#define SC_MEDIUM_SCAN			(0x38)
#define SC_COMPARE			(0x39)
#define SC_COPY_AND_VERIFY		(0x3a)
#define SC_WRITE_BUFFER			(0x3b)
#define SC_READ_BUFFER			(0x3c)
#define SC_UPDATE_BLOCK			(0x3d)
#define SC_READ_LONG			(0x3f)
#define SC_CHANGE_DEFINITION		(0x40)
#define SC_WRITE_SAME10			(0x41)
#define SC_REPORT_DENSITY_SUPPORT	(0x44)
#define SC_PLAY_AUDIO10			(0x45)
#define SC_GET_CONFIGURATION		(0x46)
#define SC_PLAY_AUDIO_MSF		(0x47)
#define SC_AUDIO_TRACK_INDEX		(0x48)
#define SC_AUDIO_TRACK_RELATIVE10	(0x49)
#define SC_AUDIO_GET_EVENT_STAT_NOTIF	(0x4a)
#define SC_PAUSE_RESUME			(0x4b)
#define SC_LOG_SELECT			(0x4c)
#define SC_LOG_SENSE			(0x4d)
#define SC_XDWRITE10			(0x50)
#define SC_XPWRITE10			(0x51)
#define SC_XDREAD10			(0x52)
#define SC_XDWRITEREAD10		(0x53)
#define SC_SEND_OPC_INFORMATION		(0x54)
#define SC_MODE_SELECT10		(0x55)
#define SC_RESERVE10			(0x56)
#define SC_RELEASE10			(0x57)
#define SC_REPAIR_TRACK			(0x58)
#define SC_MODE_SENSE10			(0x5A)
#define SC_CLOSE_TRACK_SESSION		(0x5B)
#define SC_READ_BUFFER_CAPACITY		(0x5C)
#define SC_SEND_CUE_SHEET		(0x5d)
#define SC_PERSISTENT_RESERVE_IN	(0x5e)
#define SC_PERSISTENT_RESERVE_OUT	(0x5f)
#define SC_EXTENDED_CDB			(0x7e)
#define SC_VARIABLE_LENGTH_CDB		(0x7f)
#define SC_XDWRITE_EXTENDED16		(0x80)
#define SC_WRITE_FILEMARKS16		(0x80)
#define SC_REBUILD16			(0x81)
#define SC_READ_REVERSE16		(0x81)
#define SC_REGENERATE16			(0x82)
#define SC_EXTENDED_COPY		(0x83)
#define SC_RECEIVE_COPY_RESULTS		(0x84)
#define SC_ATA_CMD_PASS_THROUGH		(0x85)
#define SC_ACCESS_CONTROL_IN		(0x86)
#define SC_ACCESS_CONTROL_OUT		(0x87)
#define SC_READ16			(0x88)
#define SC_COMPARE_AND_WRITE		(0x89)
#define SC_WRITE16			(0x8a)
#define SC_ORWRITE			(0x8b)
#define SC_READ_ATTRIBUTE		(0x8c)
#define SC_WRITE_ATTRIBUTE		(0x8d)
#define SC_WRITE_AND_VERIFY16		(0x8f)
#define SC_PREFETCH16			(0x90)
#define SC_SYNCHRONIZE_CACHE16		(0x91)
#define SC_SPACE16			(0x91)
#define SC_UNLOCK_CACHE16		(0x92)
#define SC_WRITE_SAME16			(0x93)
#define SC_SERVICE_ACTION_IN16		(0x9e)
#define SC_SERVICE_ACTION_OUT16		(0x9f)
#define SC_REPORT_LUNS			(0xa0)
#define SC_ATA_COMMAND_PASS_THROUGH12	(0xa1)
#define SC_SECURITY_PROTOCOL_IN		(0xa2)
#define SC_SEND_EVENT			(0xa2)
#define SC_MAINTENANCE_IN		(0xa3)
#define SC_MAINTENANCE_OUT		(0xa4)
#define SC_MOVE_MEDIUM			(0xa5)
#define SC_PLAY_AUDIO12			(0xa5)
#define SC_EXCHANGE_MEDUM		(0xa6)
#define SC_MOVE_MEDIUM_ATTACHED		(0xa7)
#define SC_READ12			(0xa8)
#define SC_SERVICE_ACTION_OUT12		(0xa9)
#define SC_AUDIO_TRACK_RELATIVE12	(0xa9)
#define SC_WRITE12			(0xaa)
#define SC_SERVICE_ACTION_IN12		(0xab)
#define SC_ERASE12			(0xac)
#define SC_READ_DVD_STRUCTURE		(0xad)
#define SC_WRITE_AND_VERIFY12		(0xae)
#define SC_VERIFY12			(0xaf)
#define SC_SEARCH_DATA_HIGH12		(0xb0)
#define SC_SEARCH_DATA_EQUAL12		(0xb1)
#define SC_SEARCH_DATA_LOW12		(0xb2)
#define SC_SET_LIMITS			(0xb3)
#define SC_READ_ELEMENT_STATUS_ATTACHED	(0xb4)
#define SC_SECURITY_PROTOCOL_OUT	(0xb5)
#define SC_SEND_VOLUME_TAG		(0xb6)
#define SC_READ_DEFECT_DATA12		(0xb7)
#define SC_READ_ELEMENT_STATUS		(0xb8)
#define SC_READ_CD_MSF			(0xb9)
#define SC_REDUNDANCY_GROUP_IN		(0xba)
#define SC_REDUNDANCY_GROUP_OUT		(0xbb)
#define SC_SPARE_IN			(0xbc)
#define SC_PLAY_CD			(0xbc)
#define SC_VOLUME_SET_IN		(0xbe)
#define SC_VOLUME_SET_OUT		(0xbf)

#define STCD_OK				(0x00)
#define STCD_CHECK_CONDITION		(0x02)
#define STCD_CONDITION_MET		(0x04)
#define STCD_BUSY			(0x08)
#define STCD_INTERMEDIATE		(0x10)
#define STCD_INTERM_COND_MET		(0x14)
#define STCD_RESERVATION_CONFLICT	(0x18)
#define STCD_COMMAND_TERMINATED		(0x22)
#define STCD_QUEUE_FULL			(0x28)
#define STCD_ACA_ACTIVE			(0x30)
#define STCD_TASK_ABORTED		(0x40)

/**
 **********************************************************
 * Sense keys from Table 69 of SCSI 2 spec
 **********************************************************
 */
#define SK_NO_SENSE		(0)
#define SK_RECOVERED_ERROR	(1)
#define SK_NOT_READY		(2)
#define SK_MEDIUM_ERROR		(3)
#define		ASQ_WRITE_FAULT		(0x03)
#define 	ASQ_UNREC_READ_ERR	(0x11)
#define SK_HARDWARE_ERROR	(4)
#define SK_ILLEGAL_REQUEST	(5)
#define		ASQ_UNSUPPORTED_CMD	(0x20)
#define 	ASQ_INVALID_FIELD	(0x24)
#define 	ASQ_ILLEGAL_LUN		(0x25)
#define SK_UNIT_ATTENTION	(6)
#define SK_DATA_PROTECT		(7)
#define SK_BLANK_CHECK		(8)
#define SK_VENDOR_SPEC		(9)
#define SK_COPY_ABORTED		(0xa)
#define SK_ABORTED_COMMAND	(0xb)
#define SK_EQUAL		(0xc)
#define SK_VOLUME_OVERFLOW	(0xd)
#define SK_MISSCOMPARE		(0xe)
#define SK_RESERVED		(0xf)
#if 0
#define SK_SOFT_ERR		(1)
//#define SK_SOFT_ERR
#endif

/** Sense keys */
#if 0
No Sense 0 00 00 No error
    0 5 D 00 No sense - PFA threshold reached
    Soft Error 1 01 00 Recovered Write error - no index
    1 02 00 Recovered no seek completion
    1 03 00 Recovered Write error - write fault
    1 09 00 Track following error
    1 0 B 01 Temperature warning
    1 0 C 01 Recovered Write error with auto - realloc - reallocated
    1 0 C 03 Recovered Write error - recommend reassign
    1 12 01 Recovered data without ECC using prev logical block ID
    1 12 02 Recovered data with ECC using prev logical block ID
    1 14 01 Recovered Record Not Found
    1 16 00 Recovered Write error - Data Sync Mark Error
    1 16 01 Recovered Write error - Data Sync Error - data rewritten
    1 16 02 Recovered Write error - Data Sync Error - recommend rewrite
    1 16 03 Recovered Write error - Data Sync Error - data auto - reallocated
    1 16 04 Recovered Write error - Data Sync Error - recommend reassignment
    1 17 00 Recovered data with no error correction applied
    1 17 01 Recovered Read error - with retries
    1 17 02 Recovered data using positive offset
    1 17 03 Recovered data using negative offset
    1 17 05 Recovered data using previous logical block ID
    1 17 06 Recovered Read error - without ECC, auto reallocated
    1 17 07 Recovered Read error - without ECC, recommend reassign
    1 17 08 Recovered Read error - without ECC, recommend rewrite
    1 17 09 Recovered Read error - without ECC, data rewritten
    1 18 00 Recovered Read error - with ECC
    1 18 01 Recovered data with ECC and retries
    1 18 02 Recovered Read error - with ECC, auto reallocated
    1 18 05 Recovered Read error - with ECC, recommend reassign
    1 18 06 Recovered data using ECC and offsets
    1 18 07 Recovered Read error - with ECC, data rewritten
    1 1 C 00 Defect List not found
    1 1 C 01 Primary defect list not found
    1 1 C 02 Grown defect list not found
    1 1F 00 Partial defect list transferred
    1 44 00 Internal target failure
    1 5 D 00 PFA threshold reached
    Not Ready 2 04 00 Not Ready - Cause not reportable.2 04 01 Not Ready - becoming ready
    2 04 02 Not Ready - need initialise
command(start unit)
2 04 03 Not Ready - manual intervention required
    2 04 04 Not Ready - format in progress
    2 04 09 Not Ready - self - test in progress
    2 31 00 Not Ready - medium format corrupted
    2 31 01 Not Ready - format command failed
    2 35 02 Not Ready - enclosure services unavailable
    2 3 A 00 Not Ready - medium not present
    2 3 A 01 Not Ready - medium not present - tray closed
    2 3 A 02 Not Ready - medium not present - tray open
    2 4 C 00 Diagnostic Failure - config not loaded
    Medium Error 3 02 00 Medium Error - No Seek Complete
    3 03 00 Medium Error - write fault
    3 10 00 Medium Error - ID CRC error
    3 11 00 Medium Error - unrecovered read error
    3 11 01 Medium Error - read retries exhausted
    3 11 02 Medium Error - error too long to correct
    3 11 04 Medium Error - unrecovered read error - auto re - alloc failed
    3 11 0 B Medium Error - unrecovered read error - recommend reassign
    3 14 01 Medium Error - record not found
    3 16 00 Medium Error - Data Sync Mark error
    3 16 04 Medium Error - Data Sync Error - recommend reassign
    3 19 00 Medium Error - defect list error
    3 19 01 Medium Error - defect list not available
    3 19 02 Medium Error - defect list error in primary list
    3 19 03 Medium Error - defect list error in grown list
    3 19 0E  Medium Error - fewer than 50 % defect list copies
    3 31 00 Medium Error - medium format corrupted
    3 31 01 Medium Error - format command failed
    Hardware Error 4 01 00 Hardware Error - no index or sector
    4 02 00 Hardware Error - no seek complete
    4 03 00 Hardware Error - write fault
    4 09 00 Hardware Error - track following error
    4 11 00 Hardware Error - unrecovered read error in reserved area
    4 16 00 Hardware Error - Data Sync Mark error in reserved area
    4 19 00 Hardware Error - defect list error
    4 19 02 Hardware Error - defect list error in Primary List
    4 19 03 Hardware Error - defect list error in Grown List
    4 31 00 Hardware Error - reassign failed
    4 32 00 Hardware Error - no defect spare available
    4 35 01 Hardware Error - unsupported enclosure function
    4 35 02 Hardware Error - enclosure services unavailable
    4 35 03 Hardware Error - enclosure services transfer failure
    4 35 04 Hardware Error - enclosure services refused
    4 35 05 Hardware Error - enclosure services checksum error
    4 3E 03 Hardware Error - self - test failed
    4 3E 04 Hardware Error - unable to update self - test
    4 44 00 Hardware Error - internal target failure
    Illegal Request 5 1 A 00 Illegal Request - parm list length error
    5 20 00 Illegal Request - invalid / unsupported command code
    5 21 00 Illegal Request - LBA out of range 5 24 00 Illegal Request - invalid field in
CDB(Command Descriptor Block)
5 25 00 Illegal Request - invalid LUN
    5 26 00 Illegal Request - invalid fields in parm list
    5 26 01 Illegal Request - parameter not supported
    5 26 02 Illegal Request - invalid parm value
    5 26 03 Illegal Request - invalid field parameter - threshold parameter
    5 26 04 Illegal Request - invalid release of persistent reservation
    5 2 C 00 Illegal Request - command sequence error
    5 35 01 Illegal Request - unsupported enclosure function
    5 49 00 Illegal Request - invalid message
    5 53 00 Illegal Request - media load or eject failed
    5 53 01 Illegal Request - unload tape failure
    5 53 02 Illegal Request - medium removal prevented
    5 55 00 Illegal Request - system resource failure
    5 55 01 Illegal Request - system buffer full
    5 55 04 Illegal Request - Insufficient Registration Resources
    Unit Attention 6 28 00 Unit Attention - not - ready to ready
transition(format complete)
6 29 00 Unit Attention - POR or device reset occurred
    6 29 01 Unit Attention - POR occurred
    6 29 02 Unit Attention - SCSI bus reset occurred
    6 29 03 Unit Attention - TARGET RESET occurred
    6 29 04 Unit Attention - self - initiated - reset occurred
    6 29 05 Unit Attention - transceiver mode change to SE
    6 29 06 Unit Attention - transceiver mode change to LVD
    6 2 A 00 Unit Attention - parameters changed
    6 2 A 01 Unit Attention - mode parameters changed
    6 2 A 02 Unit Attention - log select parms changed
    6 2 A 03 Unit Attention - Reservations pre - empted
    6 2 A 04 Unit Attention - Reservations released
    6 2 A 05 Unit Attention - Registrations pre - empted
    6 2F 00 Unit Attention - commands cleared by another initiator
    6 3F 00 Unit Attention - target operating conditions have changed
    6 3F 01 Unit Attention - microcode changed
    6 3F 02 Unit Attention - changed operating definition
    6 3F 03 Unit Attention - inquiry parameters changed
    6 3F 05 Unit Attention - device identifier changed
    6 5 D 00 Unit Attention - PFA threshold reached
    Write Protect 7 27 00 Write Protect - command not allowed
    Aborted Command B 00 00 Aborted Command - no additional sense code
    B 1 B 00 Aborted Command - sync data transfer
error(extra ACK)
B 25 00 Aborted Command - unsupported LUN
    B 3F 0F Aborted Command - echo buffer overwritten
    B 43 00 Aborted Command - message reject error
    B 44 00 Aborted Command - internal target failure
    B 45 00 Aborted Command - Selection / Reselection failure
    B 47 00 Aborted Command - SCSI parity error
    B 48 00 Aborted Command - initiator - detected error message received
    B 49 00 Aborted Command - inappropriate / illegal message
    B 4 B 00 Aborted Command - data phase error
    B 4E 00 Aborted Command - overlapped commands attempted
    B 4F 00 Aborted Command - due to loop initialisation
    Other E 1 D 00 Miscompare - during verify byte check operation
    x 05 00 Illegal request
    x 06 00 Unit attention
    x 07 00 Data protect
    x 08 00 LUN communication failure
    x 08 01 LUN communication timeout
    x 08 02 LUN communication parity error
    x 08 03 LUN communication CRC error
    x 09 00 vendor specific sense key
    x 09 01 servo fault
    x 09 04 head select fault
    x 0 A 00 error log overflow
    x 0 B 00 aborted command
    x 0 C 00 write error
    x 0 C 02 write error - auto - realloc failed
    x 0E 00 data miscompare
    x 12 00 address mark not found for ID field
    x 14 00 logical block not found
    x 15 00 random positioning error
    x 15 01 mechanical positioning error
    x 15 02 positioning error detected by read of medium
    x 27 00 write protected
    x 29 00 POR or bus reset occurred
    x 31 01 format failed
    x 32 01 defect list update error
    x 32 02 no spares available
    x 35 01 unspecified enclosure services failure
    x 37 00 parameter rounded
    x 3 D 00 invalid bits in identify message
    x 3E 00 LUN not self - configured yet
    x 40 01 DRAM parity error
    x 40 02 DRAM parity error
    x 42 00 power - on or self - test failure
    x 4 C 00 LUN failed self - configuration
    x 5 C 00 RPL status change
    x 5 C 01 spindles synchronised x 5 C 02 spindles not synchronised x 65 00 voltage fault
#endif
