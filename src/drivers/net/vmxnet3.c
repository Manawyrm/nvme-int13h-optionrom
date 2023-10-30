/*
 * Copyright (C) 2011 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <byteswap.h>
#include <ipxe/pci.h>
#include <ipxe/io.h>
#include <ipxe/malloc.h>
#include <ipxe/profile.h>
#include <ipxe/iobuf.h>
#include <ipxe/netdevice.h>
#include <ipxe/if_ether.h>
#include <ipxe/ethernet.h>
#include "vmxnet3.h"

/**
 * @file
 *
 * VMware vmxnet3 virtual NIC driver
 *
 */

/**
 * Transmit packet
 *
 * @v netdev		Network device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int vmxnet3_transmit ( struct net_device *netdev,
			      struct io_buffer *iobuf ) {
	return 0;
}

/**
 * Poll network device
 *
 * @v netdev		Network device
 */
static void vmxnet3_poll ( struct net_device *netdev ) {
}

/**
 * Enable/disable interrupts
 *
 * @v netdev		Network device
 * @v enable		Interrupts should be enabled
 */
static void vmxnet3_irq ( struct net_device *netdev, int enable ) {
}

/**
 * Open NIC
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int vmxnet3_open ( struct net_device *netdev ) {
	return 0;
}

/**
 * Close NIC
 *
 * @v netdev		Network device
 */
static void vmxnet3_close ( struct net_device *netdev ) {
}

/** vmxnet3 net device operations */
static struct net_device_operations vmxnet3_operations = {
	.open		= vmxnet3_open,
	.close		= vmxnet3_close,
	.transmit	= vmxnet3_transmit,
	.poll		= vmxnet3_poll,
	.irq		= vmxnet3_irq,
};

/**
 * Check version
 *
 * @v vmxnet		vmxnet3 NIC
 * @ret rc		Return status code
 */
static int vmxnet3_check_version ( struct vmxnet3_nic *vmxnet ) {
	return 0;
}

/**
 * Get permanent MAC address
 *
 * @v vmxnet		vmxnet3 NIC
 * @v hw_addr		Hardware address to fill in
 */
static void vmxnet3_get_hw_addr ( struct vmxnet3_nic *vmxnet, void *hw_addr ) {

}

/**
 * Probe PCI device
 *
 * @v pci		PCI device
 * @v id		PCI ID
 * @ret rc		Return status code
 */
static int vmxnet3_probe ( struct pci_device *pci ) {
	return 0;
}

/**
 * Remove PCI device
 *
 * @v pci		PCI device
 */
static void vmxnet3_remove ( struct pci_device *pci ) {

}

/** vmxnet3 PCI IDs */
static struct pci_device_id vmxnet3_nics[] = {
	PCI_ROM ( 0x15ad, 0x07b0, "vmxnet3", "vmxnet3 virtual NIC", 0 ),
};

/** vmxnet3 PCI driver */
struct pci_driver vmxnet3_driver __pci_driver = {
	.ids = vmxnet3_nics,
	.id_count = ( sizeof ( vmxnet3_nics ) / sizeof ( vmxnet3_nics[0] ) ),
	.probe = vmxnet3_probe,
	.remove = vmxnet3_remove,
};
