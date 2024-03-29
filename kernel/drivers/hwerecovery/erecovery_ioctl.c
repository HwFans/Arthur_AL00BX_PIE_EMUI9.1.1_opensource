/*
 * erecovery_ioctl.c
 *
 * ioctl implementaion for erecovery
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/sched.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/vmalloc.h>
#include <linux/aio.h>
#include <uapi/linux/uio.h>
#include <asm/ioctls.h>
#include <chipset_common/hwlogger/hw_logger.h>
#include "chipset_common/hwerecovery/erecovery.h"
#include "erecovery_common.h"
#include "erecovery_transtation.h"

long erecovery_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = ERECOVERY_CMD_INVALID;

	switch (cmd) {
	case ERECOVERY_WRITE_EVENT:
		ret = erecovery_write_event_user((void *)arg);
		break;
	case ERECOVERY_READ_EVENT:
		ret = erecovery_read_event((void *)arg);
	default:
		break;
	}

	return ret;
}
EXPORT_SYMBOL(erecovery_ioctl);
