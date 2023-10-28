#ifndef _NVME_H
#define _NVME_H

/** @file
 *
 * NVMe storage driver
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <ipxe/pci.h>
#include <ipxe/interface.h>
#include "nvme-int.h"

/** A NVMe storage device */
struct nvme_device {
	/** Reference count */
	struct refcnt refcnt;
	/** List of devices */
	struct list_head list;
    /** Block control interface */
    struct interface block;

	/** Command process */
	struct process process;
	/** Device opened flag */
	int opened;

    struct pci_device pci_dev;
    struct nvme_ctrl *ctrl;

    /** Current command (if any) */
	//struct nvme_command cmd;
};

#endif /* _NVME_H */
