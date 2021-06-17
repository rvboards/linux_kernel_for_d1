/**
 ******************************************************************************
 *
 * rwnx_cmds.c
 *
 * Handles queueing (push to IPC, ack/cfm from IPC) of commands issued to
 * LMAC FW
 *
 * Copyright (C) RivieraWaves 2014-2019
 *
 ******************************************************************************
 */

#include <linux/list.h>
#include <linux/version.h>
#include "aicsdio_txrxif.h"
#include "aicsdio.h"
#include "aic_bsp_driver.h"

static void cmd_dump(const struct rwnx_cmd *cmd)
{
	printk(KERN_CRIT "tkn[%d]  flags:%04x  result:%3d  cmd:%4d - reqcfm(%4d)\n",
		   cmd->tkn, cmd->flags, cmd->result, cmd->id, cmd->reqid);
}

static void cmd_complete(struct rwnx_cmd_mgr *cmd_mgr, struct rwnx_cmd *cmd)
{
	//printk("cmdcmp\n");
	lockdep_assert_held(&cmd_mgr->lock);

	list_del(&cmd->list);
	cmd_mgr->queue_sz--;

	cmd->flags |= RWNX_CMD_FLAG_DONE;
	if (cmd->flags & RWNX_CMD_FLAG_NONBLOCK) {
		kfree(cmd);
	} else {
		if (RWNX_CMD_WAIT_COMPLETE(cmd->flags)) {
			cmd->result = 0;
			complete(&cmd->complete);
		}
	}
}

static int cmd_mgr_queue(struct rwnx_cmd_mgr *cmd_mgr, struct rwnx_cmd *cmd)
{
	bool defer_push = false;

	spin_lock_bh(&cmd_mgr->lock);

	if (cmd_mgr->state == RWNX_CMD_MGR_STATE_CRASHED) {
		printk(KERN_CRIT"cmd queue crashed\n");
		cmd->result = -EPIPE;
		spin_unlock_bh(&cmd_mgr->lock);
		return -EPIPE;
	}

	if (!list_empty(&cmd_mgr->cmds)) {
		struct rwnx_cmd *last;

		if (cmd_mgr->queue_sz == cmd_mgr->max_queue_sz) {
			printk(KERN_CRIT"Too many cmds (%d) already queued\n",
				   cmd_mgr->max_queue_sz);
			cmd->result = -ENOMEM;
			spin_unlock_bh(&cmd_mgr->lock);
			return -ENOMEM;
		}
		last = list_entry(cmd_mgr->cmds.prev, struct rwnx_cmd, list);
		if (last->flags & (RWNX_CMD_FLAG_WAIT_ACK | RWNX_CMD_FLAG_WAIT_PUSH)) {
			cmd->flags |= RWNX_CMD_FLAG_WAIT_PUSH;
			defer_push = true;
		}
	}

	if (cmd->flags & RWNX_CMD_FLAG_REQ_CFM)
		cmd->flags |= RWNX_CMD_FLAG_WAIT_CFM;

	cmd->tkn    = cmd_mgr->next_tkn++;
	cmd->result = -EINTR;

	if (!(cmd->flags & RWNX_CMD_FLAG_NONBLOCK))
		init_completion(&cmd->complete);

	list_add_tail(&cmd->list, &cmd_mgr->cmds);
	cmd_mgr->queue_sz++;
	spin_unlock_bh(&cmd_mgr->lock);

	if (!defer_push) {
		//printk("queue:id=%x, param_len=%u\n", cmd->a2e_msg->id, cmd->a2e_msg->param_len);
		rwnx_set_cmd_tx((void *)(cmd_mgr->sdiodev), cmd->a2e_msg, sizeof(struct lmac_msg) + cmd->a2e_msg->param_len);
		//rwnx_ipc_msg_push(rwnx_hw, cmd, RWNX_CMD_A2EMSG_LEN(cmd->a2e_msg));
		kfree(cmd->a2e_msg);
	} else {
		//WAKE_CMD_WORK(cmd_mgr);
		printk("ERR: never defer push!!!!");
		return 0;
	}

	if (!(cmd->flags & RWNX_CMD_FLAG_NONBLOCK)) {
		unsigned long tout = msecs_to_jiffies(RWNX_80211_CMD_TIMEOUT_MS * cmd_mgr->queue_sz);
		if (!wait_for_completion_killable_timeout(&cmd->complete, tout)) {
			printk(KERN_CRIT"cmd timed-out\n");
			cmd_dump(cmd);
			spin_lock_bh(&cmd_mgr->lock);
			cmd_mgr->state = RWNX_CMD_MGR_STATE_CRASHED;
			if (!(cmd->flags & RWNX_CMD_FLAG_DONE)) {
				cmd->result = -ETIMEDOUT;
				cmd_complete(cmd_mgr, cmd);
			}
			spin_unlock_bh(&cmd_mgr->lock);
		} else {
			kfree(cmd);
		}
	} else {
		cmd->result = 0;
	}

	return 0;
}

static int cmd_mgr_run_callback(struct rwnx_cmd_mgr *cmd_mgr, struct rwnx_cmd *cmd,
								struct rwnx_cmd_e2amsg *msg, msg_cb_fct cb)
{
	int res;

	if (!cb) {
		return 0;
	}
	spin_lock(&cmd_mgr->cb_lock);
	res = cb(cmd, msg);
	spin_unlock(&cmd_mgr->cb_lock);

	return res;
}

static int cmd_mgr_msgind(struct rwnx_cmd_mgr *cmd_mgr, struct rwnx_cmd_e2amsg *msg,
						  msg_cb_fct cb)
{
	struct rwnx_cmd *cmd;
	bool found = false;

	//printk("cmd->id=%x\n", msg->id);
	spin_lock(&cmd_mgr->lock);
	list_for_each_entry(cmd, &cmd_mgr->cmds, list) {
		if (cmd->reqid == msg->id &&
			(cmd->flags & RWNX_CMD_FLAG_WAIT_CFM)) {

			if (!cmd_mgr_run_callback(cmd_mgr, cmd, msg, cb)) {
				found = true;
				cmd->flags &= ~RWNX_CMD_FLAG_WAIT_CFM;

				if (WARN((msg->param_len > RWNX_CMD_E2AMSG_LEN_MAX),
						 "Unexpect E2A msg len %d > %d\n", msg->param_len,
						 RWNX_CMD_E2AMSG_LEN_MAX)) {
					msg->param_len = RWNX_CMD_E2AMSG_LEN_MAX;
				}

				if (cmd->e2a_msg && msg->param_len)
					memcpy(cmd->e2a_msg, &msg->param, msg->param_len);

				if (RWNX_CMD_WAIT_COMPLETE(cmd->flags))
					cmd_complete(cmd_mgr, cmd);

				break;
			}
		}
	}
	spin_unlock(&cmd_mgr->lock);

	if (!found)
		cmd_mgr_run_callback(cmd_mgr, NULL, msg, cb);

	return 0;
}

static void cmd_mgr_print(struct rwnx_cmd_mgr *cmd_mgr)
{
	struct rwnx_cmd *cur;

	spin_lock_bh(&cmd_mgr->lock);
	printk("q_sz/max: %2d / %2d - next tkn: %d\n",
			 cmd_mgr->queue_sz, cmd_mgr->max_queue_sz,
			 cmd_mgr->next_tkn);
	list_for_each_entry(cur, &cmd_mgr->cmds, list) {
		cmd_dump(cur);
	}
	spin_unlock_bh(&cmd_mgr->lock);
}

static void cmd_mgr_drain(struct rwnx_cmd_mgr *cmd_mgr)
{
	struct rwnx_cmd *cur, *nxt;

	spin_lock_bh(&cmd_mgr->lock);
	list_for_each_entry_safe(cur, nxt, &cmd_mgr->cmds, list) {
		list_del(&cur->list);
		cmd_mgr->queue_sz--;
		if (!(cur->flags & RWNX_CMD_FLAG_NONBLOCK))
			complete(&cur->complete);
	}
	spin_unlock_bh(&cmd_mgr->lock);
}

void rwnx_cmd_mgr_init(struct rwnx_cmd_mgr *cmd_mgr)
{
	cmd_mgr->max_queue_sz = RWNX_CMD_MAX_QUEUED;
	INIT_LIST_HEAD(&cmd_mgr->cmds);
	cmd_mgr->state = RWNX_CMD_MGR_STATE_INITED;
	spin_lock_init(&cmd_mgr->lock);
	spin_lock_init(&cmd_mgr->cb_lock);
	cmd_mgr->queue  = &cmd_mgr_queue;
	cmd_mgr->print  = &cmd_mgr_print;
	cmd_mgr->drain  = &cmd_mgr_drain;
	cmd_mgr->llind  = NULL;//&cmd_mgr_llind;
	cmd_mgr->msgind = &cmd_mgr_msgind;

#if 0
	INIT_WORK(&cmd_mgr->cmdWork, cmd_mgr_task_process);
	cmd_mgr->cmd_wq = create_singlethread_workqueue("cmd_wq");
	if (!cmd_mgr->cmd_wq) {
		txrx_err("insufficient memory to create cmd workqueue.\n");
		return;
	}
#endif
}

void rwnx_cmd_mgr_deinit(struct rwnx_cmd_mgr *cmd_mgr)
{
	cmd_mgr->print(cmd_mgr);
	cmd_mgr->drain(cmd_mgr);
	cmd_mgr->print(cmd_mgr);
	memset(cmd_mgr, 0, sizeof(*cmd_mgr));
}

void rwnx_set_cmd_tx(void *dev, struct lmac_msg *msg, uint len)
{
	struct aic_sdio_dev *sdiodev = (struct aic_sdio_dev *)dev;
	struct aicwf_bus *bus = sdiodev->bus_if;
	u8 *buffer = bus->cmd_buf;
	u16 index = 0;

	memset(buffer, 0, CMD_BUF_MAX);
	buffer[0] = (len+4) & 0x00ff;
	buffer[1] = ((len+4) >> 8) &0x0f;
	buffer[2] = 0x11;
	buffer[3] = 0x0;
	index += 4;
	//there is a dummy word
	index += 4;

	//make sure little endian
	put_u16(&buffer[index], msg->id);
	index += 2;
	put_u16(&buffer[index], msg->dest_id);
	index += 2;
	put_u16(&buffer[index], msg->src_id);
	index += 2;
	put_u16(&buffer[index], msg->param_len);
	index += 2;
	memcpy(&buffer[index], (u8 *)msg->param, msg->param_len);

	aicwf_bus_txmsg(bus, buffer, len + 8);
}

static inline void *rwnx_msg_zalloc(lmac_msg_id_t const id,
									lmac_task_id_t const dest_id,
									lmac_task_id_t const src_id,
									uint16_t const param_len)
{
	struct lmac_msg *msg;
	gfp_t flags;

	if (in_softirq())
		flags = GFP_ATOMIC;
	else
		flags = GFP_KERNEL;

	msg = (struct lmac_msg *)kzalloc(sizeof(struct lmac_msg) + param_len,
									 flags);
	if (msg == NULL) {
		printk(KERN_CRIT "%s: msg allocation failed\n", __func__);
		return NULL;
	}
	msg->id = id;
	msg->dest_id = dest_id;
	msg->src_id = src_id;
	msg->param_len = param_len;

	return msg->param;
}

static void rwnx_msg_free(struct lmac_msg *msg, const void *msg_params)
{
	kfree(msg);
}


static int rwnx_send_msg(struct aic_sdio_dev *sdiodev, const void *msg_params,
						 int reqcfm, lmac_msg_id_t reqid, void *cfm)
{
	struct lmac_msg *msg;
	struct rwnx_cmd *cmd;
	bool nonblock;
	int ret;

	msg = container_of((void *)msg_params, struct lmac_msg, param);
	if (sdiodev->bus_if->state == BUS_DOWN_ST) {
		rwnx_msg_free(msg, msg_params);
		printk("bus is down\n");
		return 0;
	}

	nonblock = 0;
	cmd = kzalloc(sizeof(struct rwnx_cmd), nonblock ? GFP_ATOMIC : GFP_KERNEL);
	cmd->result  = -EINTR;
	cmd->id      = msg->id;
	cmd->reqid   = reqid;
	cmd->a2e_msg = msg;
	cmd->e2a_msg = cfm;
	if (nonblock)
		cmd->flags = RWNX_CMD_FLAG_NONBLOCK;
	if (reqcfm)
		cmd->flags |= RWNX_CMD_FLAG_REQ_CFM;

	if (reqcfm) {
		cmd->flags &= ~RWNX_CMD_FLAG_WAIT_ACK; // we don't need ack any more
		ret = sdiodev->cmd_mgr.queue(&sdiodev->cmd_mgr, cmd);
	} else {
		rwnx_set_cmd_tx((void *)(sdiodev), cmd->a2e_msg, sizeof(struct lmac_msg) + cmd->a2e_msg->param_len);
	}

	if (!reqcfm)
		kfree(cmd);

	return 0;
}


int rwnx_send_dbg_mem_block_write_req(struct aic_sdio_dev *sdiodev, u32 mem_addr,
									  u32 mem_size, u32 *mem_data)
{
	struct dbg_mem_block_write_req *mem_blk_write_req;

	/* Build the DBG_MEM_BLOCK_WRITE_REQ message */
	mem_blk_write_req = rwnx_msg_zalloc(DBG_MEM_BLOCK_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
										sizeof(struct dbg_mem_block_write_req));
	if (!mem_blk_write_req)
		return -ENOMEM;

	/* Set parameters for the DBG_MEM_BLOCK_WRITE_REQ message */
	mem_blk_write_req->memaddr = mem_addr;
	mem_blk_write_req->memsize = mem_size;
	memcpy(mem_blk_write_req->memdata, mem_data, mem_size);

	/* Send the DBG_MEM_BLOCK_WRITE_REQ message to LMAC FW */
	return rwnx_send_msg(sdiodev, mem_blk_write_req, 1, DBG_MEM_BLOCK_WRITE_CFM, NULL);
}

int rwnx_send_dbg_mem_read_req(struct aic_sdio_dev *sdiodev, u32 mem_addr,
							   struct dbg_mem_read_cfm *cfm)
{
	struct dbg_mem_read_req *mem_read_req;

	/* Build the DBG_MEM_READ_REQ message */
	mem_read_req = rwnx_msg_zalloc(DBG_MEM_READ_REQ, TASK_DBG, DRV_TASK_ID,
								   sizeof(struct dbg_mem_read_req));
	if (!mem_read_req)
		return -ENOMEM;

	/* Set parameters for the DBG_MEM_READ_REQ message */
	mem_read_req->memaddr = mem_addr;

	/* Send the DBG_MEM_READ_REQ message to LMAC FW */
	return rwnx_send_msg(sdiodev, mem_read_req, 1, DBG_MEM_READ_CFM, cfm);
}


int rwnx_send_dbg_mem_write_req(struct aic_sdio_dev *sdiodev, u32 mem_addr, u32 mem_data)
{
	struct dbg_mem_write_req *mem_write_req;

	/* Build the DBG_MEM_WRITE_REQ message */
	mem_write_req = rwnx_msg_zalloc(DBG_MEM_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
									sizeof(struct dbg_mem_write_req));
	if (!mem_write_req)
		return -ENOMEM;

	/* Set parameters for the DBG_MEM_WRITE_REQ message */
	mem_write_req->memaddr = mem_addr;
	mem_write_req->memdata = mem_data;

	/* Send the DBG_MEM_WRITE_REQ message to LMAC FW */
	return rwnx_send_msg(sdiodev, mem_write_req, 1, DBG_MEM_WRITE_CFM, NULL);
}

int rwnx_send_dbg_mem_mask_write_req(struct aic_sdio_dev *sdiodev, u32 mem_addr,
									 u32 mem_mask, u32 mem_data)
{
	struct dbg_mem_mask_write_req *mem_mask_write_req;

	/* Build the DBG_MEM_MASK_WRITE_REQ message */
	mem_mask_write_req = rwnx_msg_zalloc(DBG_MEM_MASK_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
										 sizeof(struct dbg_mem_mask_write_req));
	if (!mem_mask_write_req)
		return -ENOMEM;

	/* Set parameters for the DBG_MEM_MASK_WRITE_REQ message */
	mem_mask_write_req->memaddr = mem_addr;
	mem_mask_write_req->memmask = mem_mask;
	mem_mask_write_req->memdata = mem_data;

	/* Send the DBG_MEM_MASK_WRITE_REQ message to LMAC FW */
	return rwnx_send_msg(sdiodev, mem_mask_write_req, 1, DBG_MEM_MASK_WRITE_CFM, NULL);
}


int rwnx_send_dbg_start_app_req(struct aic_sdio_dev *sdiodev, u32 boot_addr, u32 boot_type)
{
	struct dbg_start_app_req *start_app_req;

	/* Build the DBG_START_APP_REQ message */
	start_app_req = rwnx_msg_zalloc(DBG_START_APP_REQ, TASK_DBG, DRV_TASK_ID,
									sizeof(struct dbg_start_app_req));
	if (!start_app_req)
		return -ENOMEM;

	/* Set parameters for the DBG_START_APP_REQ message */
	start_app_req->bootaddr = boot_addr;
	start_app_req->boottype = boot_type;

	/* Send the DBG_START_APP_REQ message to LMAC FW */
	return rwnx_send_msg(sdiodev, start_app_req, 1, DBG_START_APP_CFM, NULL);
}


static msg_cb_fct dbg_hdlrs[MSG_I(DBG_MAX)] = {
};

static msg_cb_fct *msg_hdlrs[] = {
	[TASK_DBG] = dbg_hdlrs,
};

void rwnx_rx_handle_msg(struct aic_sdio_dev *sdiodev, struct ipc_e2a_msg *msg)
{
	sdiodev->cmd_mgr.msgind(&sdiodev->cmd_mgr, msg,
							msg_hdlrs[MSG_T(msg->id)][MSG_I(msg->id)]);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
#endif

#define AICBT_MODE_BT_ONLY          0 // bt only mode
#define AICBT_MODE_BT_WIFI_COMBO    1 // combo mode
#define AICBT_MODE                  AICBT_MODE_BT_WIFI_COMBO

#if (AICBT_MODE == AICBT_MODE_BT_ONLY)
///shall be init b4 fw init
const uint32_t aicbt_patch_en_table_b4_init[][2] = {
	///0x00161928
	{0x00161b38, 0x0016b311},///sys ctrl patch
	{0x00161b20, 0x0016b0d9},///bt only mdm config
	{0x00161b30, 0x0016b2fd},///rf config patch
	{0x00161998, 0x0016b43d},///rwip setting init patch
	{0x00161b18, 0x0016ad65},///uart init patch
	{0x001619b0, 0x0016b489},///lm read features patch
	{0x00161A6c, 0x0016b4c1},///ld acl sched patch
	{0x00161D7C, 0x0016b685},///fw init start set patch
	{0x00161AB0, 0x0016b6c9},///hci send 2 host patch
	{0x00161DB0, 0x0016b77d},///hal intersysy irq patch
	{0x00161D8C, 0x0016b7f1},///sleep post process patch
	{0x00161D88, 0x0016b825},///sleep pre process patch
	{0x00161940, 0x0016b831},///rwip schedule patch
	{0x00161b2c, 0x0016b84d},///power config patch

	///patch num
	{0x0016AD5c, 0x00000000},///
};

///shall be init after fw init
const uint32_t aicbt_patch_en_table_after_init[][2] = {
	{0x00160edc, 0x0016afad},///lmp feats req ext
	{0x00160eec, 0x0016af81},///lmp feats res ext
	{0x00160dc4, 0x0016afdd},///hci rd remote ext feat
	{0x0016097c, 0x00101ac9},///hci tx burst
	{0x0016098c, 0x00101aed},///hci rx test
	{0x00160994, 0x00101b69},///hci bb test stop
	{0x0016146C, 0x00101b11},///lc tst rx mode ind

	{0x40503070, 0x00000000},
	{0x40503074, 0x00000000},
	{0x40503078, 0x00000000},
	{0x4050307c, 0x00000000},
};

const uint32_t aicbt_trap_config_table[][2] = {
	{0x0016f000, 0xe0250793},//
	{0x40080000, 0x0008bd64},///0x0008bd64&0xfffff
	{0x40080084, 0x0016f000},///trap out base addr
	{0x40080080, 0x00000001},///trap en
	{0x40100058, 0x00000000},///trap bypass
};

#elif(AICBT_MODE == AICBT_MODE_BT_WIFI_COMBO)
///shall be init b4 fw init
const uint32_t aicbt_patch_en_table_b4_init[][2] = {
	///0x00161928
	{0x00161b38, 0x0016b315},//sys ctrl patch
	{0x00161998, 0x0016b49d},///rwip setting init patch
	{0x00161b18, 0x0016ad65},///uart init patch
	{0x00161b28, 0x0016b16d},///combo mdm config patch
	{0x001619b0, 0x0016bdb1},///lm read features patch
	{0x00161A70, 0x0016be21},///ld acl rx patch
	{0x00161A6c, 0x0016bec5},///ld acl sched patch
	{0x00161AE0, 0x0016c1a1},///sch prog end isr patch
	{0x001619E0, 0x0016b64f},///pta en config
	{0x001619DC, 0x0016b64d},///pta set patch
	{0x00161938, 0x0016c299},///hci reset patch
	{0x00161b24, 0x0016c2c5},///wlan rf config patch
	///{0x0016b3aC, 0xe00c2801},///
	{0x00161D7C, 0x0016c53d},///fw init start set patch
	{0x00161AB0, 0x0016C581},///hci send 2 host patch
	{0x00161DB0, 0x0016C645},///hal intersysy irq patch
	{0x00161D8C, 0x0016C6c5},///sleep post process patch
	{0x00161D88, 0x0016C6f9},///sleep pre process patch
	{0x00161940, 0x0016c705},///rwip schedule patch
	{0x00161b2c, 0x0016c721},///power config patch

	///patch num
	{0x0016AD5c, 0x00000000},///
};

///shall be init after fw init
const uint32_t aicbt_patch_en_table_after_init[][2] = {
	{0x00160edc, 0x0016b521},///lmp feats req ext
	{0x00160eec, 0x0016b4f5},///lmp feats res ext
	{0x00160dc4, 0x0016b551},///hci rd remote ext feat
	{0x0016097c, 0x00101ac9},///hci tx burst
	{0x0016098c, 0x00101aed},///hci rx test
	{0x00160994, 0x00101b69},///hci bb test stop
	{0x0016146C, 0x00101b11},///lc tst rx mode ind

	{0x40503070, 0x00000000},
	{0x40503074, 0x00000000},
	{0x40503078, 0x00000000},
	{0x4050307c, 0x00000000},
};

const uint32_t aicbt_trap_config_table[][2] = {
	{0x0016f000, 0xe0250793},//
	{0x40080000, 0x0008bd64},///0x0008bd64&0xfffff
	{0x0016f004, 0xe0052b00},///
	{0x40080004, 0x000b5570},///0x000b55c8&0xfffff
	{0x0016f008, 0xe7b12b00},///
	{0x40080008, 0x000b5748},///0x000b5748&0xfffff
	{0x0016f00c, 0x001ad00e},//
	{0x4008000c, 0x000ac8f8},///0x000ac8f8&0xfffff
	{0x40080084, 0x0016f000},///trap out base addr
	{0x40080080, 0x0000000f},///trap en
	{0x40100058, 0x00000000},///trap bypass
};
#else
#AICBT_MOD_ERR
#endif

static const char *aic_bt_path = "/vendor/etc/firmware";
#define FW_PATH_MAX 200

static int rwnx_load_firmware(u32 **fw_buf, const char *name, struct device *device)
{
	void *buffer = NULL;
	char *path = NULL;
	struct file *fp = NULL;
	int size = 0, len = 0, i = 0;
	ssize_t rdlen = 0;
	u32 *src = NULL, *dst = NULL;

	/* get the firmware path */
	path = __getname();
	if (!path) {
		*fw_buf = NULL;
		return -1;
	}

	len = snprintf(path, FW_PATH_MAX, "%s/%s", aic_bt_path, name);
	if (len >= FW_PATH_MAX) {
		printk("%s: %s file's path too long\n", __func__, name);
		*fw_buf = NULL;
		__putname(path);
		return -1;
	}

	printk("%s :firmware path = %s  \n", __func__, path);

	/* open the firmware file */
	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(fp) || (!fp)) {
		printk("%s: %s file failed to open\n", __func__, name);
		if (IS_ERR(fp))
			printk("is_Err\n");
		if ((!fp))
			printk("null\n");
		*fw_buf = NULL;
		__putname(path);
		fp = NULL;
		return -1;
	}

	size = i_size_read(file_inode(fp));
	if (size <= 0) {
		printk("%s: %s file size invalid %d\n", __func__, name, size);
		*fw_buf = NULL;
		__putname(path);
		filp_close(fp, NULL);
		fp = NULL;
		return -1;
	}

	/* start to read from firmware file */
	buffer = kzalloc(size, GFP_KERNEL);
	if (!buffer) {
		*fw_buf = NULL;
		__putname(path);
		filp_close(fp, NULL);
		fp = NULL;
		return -1;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 13, 16)
	rdlen = kernel_read(fp, buffer, size, &fp->f_pos);
#else
	rdlen = kernel_read(fp, fp->f_pos, buffer, size);
#endif

	if (size != rdlen) {
		printk("%s: %s file rdlen invalid %ld\n", __func__, name, rdlen);
		*fw_buf = NULL;
		__putname(path);
		filp_close(fp, NULL);
		fp = NULL;
		kfree(buffer);
		buffer = NULL;
		return -1;
	}
	if (rdlen > 0) {
		fp->f_pos += rdlen;
	}

   /*start to transform the data format*/
	src = (u32 *)buffer;
	printk("malloc dst\n");
	dst = (u32 *)kzalloc(size, GFP_KERNEL);

	if (!dst) {
		*fw_buf = NULL;
		__putname(path);
		filp_close(fp, NULL);
		fp = NULL;
		kfree(buffer);
		buffer = NULL;
		return -1;
	}

	for (i = 0; i < (size/4); i++) {
		dst[i] = src[i];
	}

	__putname(path);
	filp_close(fp, NULL);
	fp = NULL;
	kfree(buffer);
	buffer = NULL;
	*fw_buf = dst;

	return size;
}

int rwnx_plat_bin_fw_upload_android(struct aic_sdio_dev *sdiodev, u32 fw_addr,
							   char *filename)
{
	struct device *dev = sdiodev->dev;
	unsigned int i = 0;
	int size;
	u32 *dst = NULL;
	int err = 0;

	/* load aic firmware */
	size = rwnx_load_firmware(&dst, filename, dev);
	if (size <= 0) {
		printk("wrong size of firmware file\n");
		kfree(dst);
		dst = NULL;
		return -1;
	}

	/* Copy the file on the Embedded side */
	printk("\n### Upload %s firmware, @ = %x  size=%d\n", filename, fw_addr, size);

	if (size > 1024) {// > 1KB data
		for (i = 0; i < (size - 1024); i += 1024) {//each time write 1KB
			err = rwnx_send_dbg_mem_block_write_req(sdiodev, fw_addr + i, 1024, dst + i / 4);
			if (err) {
				printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
				break;
			}
		}
	}

	if (!err && (i < size)) {// <1KB data
		err = rwnx_send_dbg_mem_block_write_req(sdiodev, fw_addr + i, size - i, dst + i / 4);
		if (err) {
			printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
		}
	}

	if (dst) {
		kfree(dst);
		dst = NULL;
	}

	printk("fw download complete\n");

	return err;
}

bool aicbt_patch_trap_data_load(struct aic_sdio_dev *sdiodev)
{
	bool status = false;
	do {
		if (rwnx_plat_bin_fw_upload_android(sdiodev, FW_RAM_ADID_BASE_ADDR, FW_ADID_BIN_NAME))
			break;
#if (AICBT_MODE == AICBT_MODE_BT_ONLY)
		if (rwnx_plat_bin_fw_upload_android(sdiodev, FW_RAM_PATCH_BASE_ADDR, FW_PATCH_BIN_BTONLY_NAME))
			break;
#elif(AICBT_MODE == AICBT_MODE_BT_WIFI_COMBO)
		if (rwnx_plat_bin_fw_upload_android(sdiodev, FW_RAM_PATCH_BASE_ADDR, FW_PATCH_BIN_COMBO_NAME))
			break;
#else
#AICBT_MOD_ERR
#endif
		//if (rwnx_plat_bin_fw_upload_android(sdiodev, FW_PATCH_TEST_BASE_ADDR, FW_PATCH_TEST_BIN_NAME))
			// break;
		status =  true;
	} while (0);
	return status;
}

void aicbt_trap_config(struct aic_sdio_dev *sdiodev)
{
	uint8_t i = 0;
	uint32_t trap_len = sizeof(aicbt_trap_config_table)/sizeof(aicbt_trap_config_table[0]);
	for (i = 0; i < trap_len; i++)
		rwnx_send_dbg_mem_write_req(sdiodev, aicbt_trap_config_table[i][0], aicbt_trap_config_table[i][1]);
}

void aicbt_patch_init_b4_fw_init(struct aic_sdio_dev *sdiodev)
{
	uint8_t i = 0;
	uint32_t patch_len = sizeof(aicbt_patch_en_table_b4_init)/sizeof(aicbt_patch_en_table_b4_init[0]);
	for (i = 0; i < patch_len; i++)
		rwnx_send_dbg_mem_write_req(sdiodev, aicbt_patch_en_table_b4_init[i][0], aicbt_patch_en_table_b4_init[i][1]);
}

void aicbt_patch_init_after_fw_init(struct aic_sdio_dev *sdiodev)
{
	uint8_t i = 0;
	uint32_t patch_len = sizeof(aicbt_patch_en_table_after_init)/sizeof(aicbt_patch_en_table_after_init[0]);
	for (i = 0; i < patch_len; i++)
		rwnx_send_dbg_mem_write_req(sdiodev, aicbt_patch_en_table_after_init[i][0], aicbt_patch_en_table_after_init[i][1]);
}

void aicbt_power_on(struct aic_sdio_dev *sdiodev)
{
	rwnx_send_dbg_mem_write_req(sdiodev, 0x40500058, 0x80000);
	rwnx_send_dbg_mem_write_req(sdiodev, 0x40500124, 0x20);
}

void aicbt_init(struct aic_sdio_dev *sdiodev)
{
	aicbt_patch_trap_data_load(sdiodev);
	aicbt_trap_config(sdiodev);
	aicbt_patch_init_b4_fw_init(sdiodev);
	aicbt_power_on(sdiodev);
	udelay(500);
	aicbt_patch_init_after_fw_init(sdiodev);
}

#define RAM_FMAC_FW_ADDR 0x00110000
static int aicwifi_start_from_bootrom(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;

	/* memory access */
	const u32 fw_addr = RAM_FMAC_FW_ADDR;

	/* fw start */
	printk("Start app: %08x\n", fw_addr);
	ret = rwnx_send_dbg_start_app_req(sdiodev, fw_addr, HOST_START_APP_AUTO);
	if (ret) {
		return -1;
	}
	return 0;
}

u32 patch_tbl[][2] = {

};

u32 syscfg_tbl_masked[][3] = {
	{0x40506024, 0x000000FF, 0x000000DF}, // for clk gate lp_level
};

u32 rf_tbl_masked[][3] = {
	{0x40344058, 0x00800000, 0x00000000},// pll trx
};

static void aicwifi_sys_config(struct aic_sdio_dev *sdiodev)
{
	int ret, cnt;
	int syscfg_num = sizeof(syscfg_tbl_masked) / sizeof(u32) / 3;
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_mask_write_req(sdiodev,
			syscfg_tbl_masked[cnt][0], syscfg_tbl_masked[cnt][1], syscfg_tbl_masked[cnt][2]);
		if (ret) {
			printk("%x mask write fail: %d\n", syscfg_tbl_masked[cnt][0], ret);
		}
	}

	ret = rwnx_send_dbg_mem_mask_write_req(sdiodev,
				rf_tbl_masked[0][0], rf_tbl_masked[0][1], rf_tbl_masked[0][2]);
	if (ret) {
		printk("rf config %x write fail: %d\n", rf_tbl_masked[0][0], ret);
	}
}

static void aicwifi_patch_config(struct aic_sdio_dev *sdiodev)
{
	const u32 rd_patch_addr = RAM_FMAC_FW_ADDR + 0x0180;
	u32 config_base;
	uint32_t start_addr = 0x1e6000;
	u32 patch_addr = start_addr;
	u32 patch_num = sizeof(patch_tbl)/4;
	struct dbg_mem_read_cfm rd_patch_addr_cfm;
	int ret = 0;
	u16 cnt = 0;

	printk("Read FW mem: %08x\n", rd_patch_addr);
	ret = rwnx_send_dbg_mem_read_req(sdiodev, rd_patch_addr, &rd_patch_addr_cfm);
	if (ret) {
		printk("patch rd fail\n");
	}

	printk("%x=%x\n", rd_patch_addr_cfm.memaddr, rd_patch_addr_cfm.memdata);
	config_base = rd_patch_addr_cfm.memdata;

	ret = rwnx_send_dbg_mem_write_req(sdiodev, 0x1e4d28, patch_addr);
	if (ret) {
		printk("%x write fail\n", 0x1e4d28);
	}

	ret = rwnx_send_dbg_mem_write_req(sdiodev, 0x1e4d2c, patch_num);
	if (ret) {
		printk("%x write fail\n", 0x1e4d2c);
	}

	for (cnt = 0; cnt < patch_num/2; cnt += 1) {
		ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt, patch_tbl[cnt][0]+config_base);
		if (ret) {
			printk("%x write fail\n", start_addr+8*cnt);
		}

		ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt+4, patch_tbl[cnt][1]);
		if (ret) {
			printk("%x write fail\n", start_addr+8*cnt+4);
		}
	}
}

void aicwifi_init(struct aic_sdio_dev *sdiodev)
{
	if (rwnx_plat_bin_fw_upload_android(sdiodev, FW_WIFI_RAM_ADDR, FW_WIFI_BIN_NAME)) {
		printk("download wifi fw fail\n");
		return;
	}

	printk("wifi config\n");
	aicwifi_patch_config(sdiodev);
	aicwifi_sys_config(sdiodev);

	aicwifi_start_from_bootrom(sdiodev);
}

int aicbsp_platform_init(struct aic_sdio_dev *sdiodev)
{
	rwnx_cmd_mgr_init(&sdiodev->cmd_mgr);
	sdiodev->cmd_mgr.sdiodev = (void *)sdiodev;
	return 0;
}

void aicbsp_platform_deinit(struct aic_sdio_dev *sdiodev)
{
	(void)sdiodev;
}

void aicbsp_driver_fw_init(struct aic_sdio_dev *sdiodev)
{
	aicbt_init(sdiodev);
	aicwifi_init(sdiodev);
}
