/*******************************************************************
 * This file is part of the Emulex Linux Device Driver for         *
 * Enterprise Fibre Channel Host Bus Adapters.                     *
 * Refer to the README file included with this package for         *
 * driver version and adapter support.                             *
 * Copyright (C) 2004 Emulex Corporation.                          *
 * www.emulex.com                                                  *
 *                                                                 *
 * This program is free software; you can redistribute it and/or   *
 * modify it under the terms of the GNU General Public License     *
 * as published by the Free Software Foundation; either version 2  *
 * of the License, or (at your option) any later version.          *
 *                                                                 *
 * This program is distributed in the hope that it will be useful, *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   *
 * GNU General Public License for more details, a copy of which    *
 * can be found in the file COPYING included with this package.    *
 *******************************************************************/

/*
 * $Id$
 */

struct lpfc_hba;

#define list_remove_head(list, entry, type, member)		\
	if (!list_empty(list)) {				\
		entry = list_entry((list)->next, type, member);	\
		list_del_init(&entry->member);			\
	}

#define list_get_first(list, type, member)			\
	(list_empty(list)) ? NULL :				\
	list_entry((list)->next, type, member)

/* per-port data that is allocated in the FC transport for us */
struct lpfc_rport_data {
	struct lpfc_nodelist *pnode;	/* Pointer to the node structure. */
};

struct fcp_rsp {
	uint32_t rspRsvd1;	/* FC Word 0, byte 0:3 */
	uint32_t rspRsvd2;	/* FC Word 1, byte 0:3 */

	uint8_t rspStatus0;	/* FCP_STATUS byte 0 (reserved) */
	uint8_t rspStatus1;	/* FCP_STATUS byte 1 (reserved) */
	uint8_t rspStatus2;	/* FCP_STATUS byte 2 field validity */
#define RSP_LEN_VALID  0x01	/* bit 0 */
#define SNS_LEN_VALID  0x02	/* bit 1 */
#define RESID_OVER     0x04	/* bit 2 */
#define RESID_UNDER    0x08	/* bit 3 */
	uint8_t rspStatus3;	/* FCP_STATUS byte 3 SCSI status byte */

	uint32_t rspResId;	/* Residual xfer if residual count field set in
				   fcpStatus2 */
	/* Received in Big Endian format */
	uint32_t rspSnsLen;	/* Length of sense data in fcpSnsInfo */
	/* Received in Big Endian format */
	uint32_t rspRspLen;	/* Length of FCP response data in fcpRspInfo */
	/* Received in Big Endian format */

	uint8_t rspInfo0;	/* FCP_RSP_INFO byte 0 (reserved) */
	uint8_t rspInfo1;	/* FCP_RSP_INFO byte 1 (reserved) */
	uint8_t rspInfo2;	/* FCP_RSP_INFO byte 2 (reserved) */
	uint8_t rspInfo3;	/* FCP_RSP_INFO RSP_CODE byte 3 */

#define RSP_NO_FAILURE       0x00
#define RSP_DATA_BURST_ERR   0x01
#define RSP_CMD_FIELD_ERR    0x02
#define RSP_RO_MISMATCH_ERR  0x03
#define RSP_TM_NOT_SUPPORTED 0x04	/* Task mgmt function not supported */
#define RSP_TM_NOT_COMPLETED 0x05	/* Task mgmt function not performed */

	uint32_t rspInfoRsvd;	/* FCP_RSP_INFO bytes 4-7 (reserved) */

	uint8_t rspSnsInfo[128];
#define SNS_ILLEGAL_REQ 0x05	/* sense key is byte 3 ([2]) */
#define SNSCOD_BADCMD 0x20	/* sense code is byte 13 ([12]) */
};

struct fcp_cmnd {
	uint32_t fcpLunMsl;	/* most  significant lun word (32 bits) */
	uint32_t fcpLunLsl;	/* least significant lun word (32 bits) */
	/* # of bits to shift lun id to end up in right
	 * payload word, little endian = 8, big = 16.
	 */
#if __BIG_ENDIAN
#define FC_LUN_SHIFT         16
#define FC_ADDR_MODE_SHIFT   24
#else	/*  __LITTLE_ENDIAN */
#define FC_LUN_SHIFT         8
#define FC_ADDR_MODE_SHIFT   0
#endif

	uint8_t fcpCntl0;	/* FCP_CNTL byte 0 (reserved) */
	uint8_t fcpCntl1;	/* FCP_CNTL byte 1 task codes */
#define  SIMPLE_Q        0x00
#define  HEAD_OF_Q       0x01
#define  ORDERED_Q       0x02
#define  ACA_Q           0x04
#define  UNTAGGED        0x05
	uint8_t fcpCntl2;	/* FCP_CTL byte 2 task management codes */
#define  FCP_ABORT_TASK_SET  0x02	/* Bit 1 */
#define  FCP_CLEAR_TASK_SET  0x04	/* bit 2 */
#define  FCP_BUS_RESET       0x08	/* bit 3 */
#define  FCP_LUN_RESET       0x10	/* bit 4 */
#define  FCP_TARGET_RESET    0x20	/* bit 5 */
#define  FCP_CLEAR_ACA       0x40	/* bit 6 */
#define  FCP_TERMINATE_TASK  0x80	/* bit 7 */
	uint8_t fcpCntl3;
#define  WRITE_DATA      0x01	/* Bit 0 */
#define  READ_DATA       0x02	/* Bit 1 */

	uint8_t fcpCdb[16];	/* SRB cdb field is copied here */
	uint32_t fcpDl;		/* Total transfer length */

};

struct lpfc_scsi_buf {
	struct list_head list;
	struct scsi_cmnd *pCmd;
	struct lpfc_hba *scsi_hba;
	struct lpfc_rport_data *rdata;

	uint32_t timeout;

	uint16_t status;	/* From IOCB Word 7- ulpStatus */
	uint32_t result;	/* From IOCB Word 4. */

	uint32_t   seg_cnt;	/* Number of scatter-gather segments returned by
				 * dma_map_sg.  The driver needs this for calls
				 * to dma_unmap_sg. */
	dma_addr_t nonsg_phys;	/* Non scatter-gather physical address. */

	/*
	 * data and dma_handle are the kernel virutal and bus address of the
	 * dma-able buffer containing the fcp_cmd, fcp_rsp and a scatter
	 * gather bde list that supports the sg_tablesize value.
	 */
	void *data;
	dma_addr_t dma_handle;

	struct fcp_cmnd *fcp_cmnd;
	struct fcp_rsp *fcp_rsp;
	struct ulp_bde64 *fcp_bpl;

	/* cur_iocbq has phys of the dma-able buffer.
	 * Iotag is in here
	 */
	struct lpfc_iocbq cur_iocbq;
};

#define LPFC_SCSI_DMA_EXT_SIZE 264
#define LPFC_BPL_SIZE          1024

#define MDAC_DIRECT_CMD                  0x22
