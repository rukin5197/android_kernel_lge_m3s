
#ifndef LG_BACKUP_ITEMS_H
#define LG_BACKUP_ITEMS_H

/*
* MISC Partition Usage
+-------+------------------------+----------+
|MISC(0~4)  |  FRST(8)  |  MEID(12)  |  NVCRC(16)  |  ESN (20)  |  WLAN_MAC(24)  |  PF_NIC(28)  
|MSMPID(32)|USBID(36)|WEBSID(40)| XCALBACKUP(44)
|SRD Backup (2048~4095)
| LCD cal backup (4096 ~ 4106)
+-------+------------------------+----------+
MISC		: Recovery Message 저장용
FRST		: Factory reset flag
MEID		: NV_MEID_I backup
NVCRC		: NV crc data backup
ESN		: NV_ESN_I backup
PID			: NV_FACTORY_INFO_I backup
WLAN_MAC	: NV_WLAN_MAC_ADDRESS_I backup
PF_NIC		: NV_LG_LTE_PF_NIC_MAC_I backup
WEB_SID		: WEB DOWNLOAD SID REMARK
XCALBACKUP	: RF CAL Golden Copy 저장용
*/

/*
 * page offset in the partition
 */
#define PTN_FRST_PERSIST_OFFSET_IN_MISC_PARTITION   8
#define PTN_MEID_PERSIST_OFFSET_IN_MISC_PARTITION   12
#define PTN_NVCRC_PERSIST_OFFSET_IN_MISC_PARTITION  16
#define PTN_ESN_PERSIST_OFFSET_IN_MISC_PARTITION   20
#if 1//LGE_MAC_ADDRESS_BACKUP
#define PTN_WLAN_MAC_PERSIST_OFFSET_IN_MISC_PARTITION  24
#define PTN_PF_NIC_MAC_PERSIST_OFFSET_IN_MISC_PARTITION  28
#endif
#define PTN_PID_PERSIST_OFFSET_IN_MISC_PARTITION    32
#define PTN_USBID_PERSIST_OFFSET_IN_MISC_PARTITION  36
#define PTN_WEB_SID_PERSIST_OFFSET_IN_MISC_PARTITION 40
#define PTN_XCAL_OFFSET_IN_MISC_PARTITION           44
#define PTN_SRD_BACKUP_OFFSET_IN_MISC_PARTITION     2048  // CONFIG_LGE_DIAG_SRD
//LCD_Cal
#define PTN_LCD_K_CAL_OFFSET_IN_MISC_PARTITION 4096//0x3400
#define PTN_FRST_PERSIST_POSITION_IN_MISC_PARTITION     (512*PTN_FRST_PERSIST_OFFSET_IN_MISC_PARTITION)
#define PTN_MEID_PERSIST_POSITION_IN_MISC_PARTITION     (512*PTN_MEID_PERSIST_OFFSET_IN_MISC_PARTITION)
#define PTN_ESN_PERSIST_POSITION_IN_MISC_PARTITION     (512*PTN_ESN_PERSIST_OFFSET_IN_MISC_PARTITION)
#define PTN_NVCRC_PERSIST_POSITION_IN_MISC_PARTITION    (512*PTN_NVCRC_PERSIST_OFFSET_IN_MISC_PARTITION)
#if 1//LGE_MAC_ADDRESS_BACKUP
#define PTN_WLAN_MAC_PERSIST_POSITION_IN_MISC_PARTITION (512*PTN_WLAN_MAC_PERSIST_OFFSET_IN_MISC_PARTITION)
#define PTN_PF_NIC_MAC_PERSIST_POSITION_IN_MISC_PARTITION  (512*PTN_PF_NIC_MAC_PERSIST_OFFSET_IN_MISC_PARTITION)
#endif
#define PTN_PID_PERSIST_POSITION_IN_MISC_PARTITION      (512*PTN_PID_PERSIST_OFFSET_IN_MISC_PARTITION)
#define PTN_USBID_PERSIST_POSITION_IN_MISC_PARTITION    (512*PTN_USBID_PERSIST_OFFSET_IN_MISC_PARTITION)
#define PTN_WEB_SID_POSITION_IN_MISC_PARTITION          (512*PTN_WEB_SID_PERSIST_OFFSET_IN_MISC_PARTITION)
#define PTN_XCAL_POSITION_IN_MISC_PARTITION             (512*PTN_XCAL_OFFSET_IN_MISC_PARTITION)
#define PTN_SRD_BACKUP_POSITION_IN_MISC_PARTITION       (512*PTN_SRD_BACKUP_OFFSET_IN_MISC_PARTITION) // CONFIG_LGE_DIAG_SRD
//LCD_Cal
#define K_CAL_DATA_OFFSET_IN_BYTES 4096*512//0x680000 //6.5MB 

/*
 * command codes for the backup operation
 */
#define CALBACKUP_GETFREE_SPACE         0
#define CALBACKUP_CAL_READ              1
#define CALBACKUP_MEID_READ             2
#define CALBACKUP_MEID_WRITE            4
#define CALBACKUP_ERASE                 5
#define CALBACKUP_CAL_WRITE             6
#define NVCRC_BACKUP_READ               7
#define NVCRC_BACKUP_WRITE              8
#define PID_BACKUP_READ                 9
#define PID_BACKUP_WRITE                10

/* BEGIN: 0015981 jihoon.lee@lge.com 20110214 */
/* ADD 0015981: [MANUFACTURE] BACK UP MAC ADDRESS NV */
#if 1//LGE_MAC_ADDRESS_BACKUP
#define WLAN_MAC_ADDRESS_BACKUP_READ	11
#define WLAN_MAC_ADDRESS_BACKUP_WRITE	12
#define PF_NIC_MAC_BACKUP_READ			13
#define PF_NIC_MAC_BACKUP_WRITE			14
#define USBID_REMOTE_WRITE              15
#endif
/* END: 0015981 jihoon.lee@lge.com 20110214 */
#if 1 // def LG_FW_WEB_DOWNLOAD
#define WEB_SID_REMARK_READ				16
#define WEB_SID_REMARK_WRITE			17
#endif

#define CALBACKUP_ESN_READ             18
#define CALBACKUP_ESN_WRITE            19

#define CALBACKUP_CALFILE		0
#define CALBACKUP_MEIDFILE		1


/*
 * set magic code 8 byte for validation
 */
#define MEID_MAGIC_CODE 0x4D4549444D454944 /*MEIDMEID*/
#define MEID_MAGIC_CODE_SIZE 8

#define ESN_MAGIC_CODE 0x45534E5F /*ESN_*/
#define ESN_MAGIC_CODE_SIZE 4

#define FACTORYINFO_MAGIC_CODE 0x5049445F /*PID_*/
#define FACTORYINFO_MAGIC_CODE_SIZE 4

#define WLAN_MAGIC_CODE 0x574C /*WL*/
#define PFNIC_MAGIC_CODE 0x5046 /*PF*/
#define MACADDR_MAGIC_CODE_SIZE 2

#define MEIDBACKUP_SECTOR_SIZE		1

#define USBID_MAGIC_CODE 0x55534249 /*USBI*/
#define USBID_MAGIC_CODE_SIZE 4

/*
 * actual item size declaration, smem allocation size should be 64-bit aligned
 */
//#define MEIDBACKUP_BYTES_SIZE				8
#define MEID_BACKUP_SIZE					8
#define MEID_BACKUP_64BIT_ALIGNED_SIZE	(MEID_MAGIC_CODE_SIZE+MEID_BACKUP_SIZE)

#define ESN_BACKUP_SIZE					4
#define ESN_BACKUP_64BIT_ALIGNED_SIZE	(ESN_MAGIC_CODE_SIZE+ESN_BACKUP_SIZE)

#define USBID_BACKUP_SIZE					4
#define USBID_BACKUP_64BIT_ALIGNED_SIZE	(USBID_MAGIC_CODE_SIZE+USBID_BACKUP_SIZE)

#define FACTORYINFO_BACKUP_SIZE					100
#define FACTORYINFO_BACKUP_64BIT_ALIGNED_SIZE	(FACTORYINFO_MAGIC_CODE_SIZE + FACTORYINFO_BACKUP_SIZE)


/* BEGIN: 0015981 jihoon.lee@lge.com 20110214 */
/* ADD 0015981: [MANUFACTURE] BACK UP MAC ADDRESS NV */
#if 1//LGE_MAC_ADDRESS_BACKUP
#define MACADDR_BACKUP_SIZE					6
#define MACADDR_BACKUP_64BIT_ALIGNED_SIZE	(MACADDR_MAGIC_CODE_SIZE + MACADDR_BACKUP_SIZE)
#endif
/* END: 0015981 jihoon.lee@lge.com 20110214 */

//1 2011.06.23 current smem_alloc size is 44932
#define BACKUP_TOTAL_SIZE 44932

#endif /* LG_BACKUP_ITEMS_H */


