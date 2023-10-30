/** @file
 *
 * PXE UNDI API
 *
 */

/*
 * Copyright (C) 2004 Michael Brown <mbrown@fensystems.co.uk>.
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
#include <stdio.h>
#include <string.h>
#include <byteswap.h>
#include <basemem_packet.h>
#include <ipxe/netdevice.h>
#include <ipxe/iobuf.h>
#include <ipxe/device.h>
#include <ipxe/pci.h>
#include <ipxe/if_ether.h>
#include <ipxe/ip.h>
#include <ipxe/arp.h>
#include <ipxe/rarp.h>
#include <ipxe/profile.h>
#include "pxe.h"

/**
 * Count of outstanding transmitted packets
 *
 * This is incremented each time PXENV_UNDI_TRANSMIT is called, and
 * decremented each time that PXENV_UNDI_ISR is called with the TX
 * queue empty, stopping when the count reaches zero.  This allows us
 * to provide a pessimistic approximation of TX completion events to
 * the PXE NBP simply by monitoring the netdev's TX queue.
 */
static int undi_tx_count = 0;

struct net_device *pxe_netdev = NULL;

/** Transmit profiler */
static struct profiler undi_tx_profiler __profiler = { .name = "undi.tx" };

/**
 * Set network device as current PXE network device
 *
 * @v netdev		Network device, or NULL
 */
void pxe_set_netdev ( struct net_device *netdev ) {

}

/**
 * Open PXE network device
 *
 * @ret rc		Return status code
 */
static int pxe_netdev_open ( void ) {
	return 0;
}

/**
 * Close PXE network device
 *
 */
static void pxe_netdev_close ( void ) {

}

/**
 * Dump multicast address list
 *
 * @v mcast		PXE multicast address list
 */
static void pxe_dump_mcast_list ( struct s_PXENV_UNDI_MCAST_ADDRESS *mcast ) {
}

/* PXENV_UNDI_STARTUP
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_startup ( struct s_PXENV_UNDI_STARTUP *undi_startup ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_CLEANUP
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_cleanup ( struct s_PXENV_UNDI_CLEANUP *undi_cleanup ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_INITIALIZE
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_initialize ( struct s_PXENV_UNDI_INITIALIZE *undi_initialize ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_RESET_ADAPTER
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_reset_adapter ( struct s_PXENV_UNDI_RESET *undi_reset_adapter ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_SHUTDOWN
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_shutdown ( struct s_PXENV_UNDI_SHUTDOWN *undi_shutdown ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_OPEN
 *
 * Status: working
 */
static PXENV_EXIT_t pxenv_undi_open ( struct s_PXENV_UNDI_OPEN *undi_open ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_CLOSE
 *
 * Status: working
 */
static PXENV_EXIT_t pxenv_undi_close ( struct s_PXENV_UNDI_CLOSE *undi_close ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_TRANSMIT
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_transmit ( struct s_PXENV_UNDI_TRANSMIT *undi_transmit ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_SET_MCAST_ADDRESS
 *
 * Status: working (for NICs that support receive-all-multicast)
 */
static PXENV_EXIT_t
pxenv_undi_set_mcast_address ( struct s_PXENV_UNDI_SET_MCAST_ADDRESS
			       *undi_set_mcast_address ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_SET_STATION_ADDRESS
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_set_station_address ( struct s_PXENV_UNDI_SET_STATION_ADDRESS
				 *undi_set_station_address ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_SET_PACKET_FILTER
 *
 * Status: won't implement (would require driver API changes for no
 * real benefit)
 */
static PXENV_EXIT_t
pxenv_undi_set_packet_filter ( struct s_PXENV_UNDI_SET_PACKET_FILTER
			       *undi_set_packet_filter ) {

	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_GET_INFORMATION
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_get_information ( struct s_PXENV_UNDI_GET_INFORMATION
			     *undi_get_information ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_GET_STATISTICS
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_get_statistics ( struct s_PXENV_UNDI_GET_STATISTICS
			    *undi_get_statistics ) {

	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_CLEAR_STATISTICS
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_clear_statistics ( struct s_PXENV_UNDI_CLEAR_STATISTICS
			      *undi_clear_statistics ) {

	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_INITIATE_DIAGS
 *
 * Status: won't implement (would require driver API changes for no
 * real benefit)
 */
static PXENV_EXIT_t
pxenv_undi_initiate_diags ( struct s_PXENV_UNDI_INITIATE_DIAGS
			    *undi_initiate_diags ) {

	return PXENV_EXIT_FAILURE;
}

/* PXENV_UNDI_FORCE_INTERRUPT
 *
 * Status: won't implement (would require driver API changes for no
 * perceptible benefit)
 */
static PXENV_EXIT_t
pxenv_undi_force_interrupt ( struct s_PXENV_UNDI_FORCE_INTERRUPT
			     *undi_force_interrupt ) {

	return PXENV_EXIT_FAILURE;
}

/* PXENV_UNDI_GET_MCAST_ADDRESS
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_get_mcast_address ( struct s_PXENV_UNDI_GET_MCAST_ADDRESS
			       *undi_get_mcast_address ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_GET_NIC_TYPE
 *
 * Status: working
 */
static PXENV_EXIT_t pxenv_undi_get_nic_type ( struct s_PXENV_UNDI_GET_NIC_TYPE
					      *undi_get_nic_type ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_GET_IFACE_INFO
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_undi_get_iface_info ( struct s_PXENV_UNDI_GET_IFACE_INFO
			    *undi_get_iface_info ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_UNDI_GET_STATE
 *
 * Status: impossible due to opcode collision
 */

/* PXENV_UNDI_ISR
 *
 * Status: working
 */
static PXENV_EXIT_t pxenv_undi_isr ( struct s_PXENV_UNDI_ISR *undi_isr ) {
	return PXENV_EXIT_SUCCESS;
}

/** PXE UNDI API */
struct pxe_api_call pxe_undi_api[] __pxe_api_call = {
	PXE_API_CALL ( PXENV_UNDI_STARTUP, pxenv_undi_startup,
		       struct s_PXENV_UNDI_STARTUP ),
	PXE_API_CALL ( PXENV_UNDI_CLEANUP, pxenv_undi_cleanup,
		       struct s_PXENV_UNDI_CLEANUP ),
	PXE_API_CALL ( PXENV_UNDI_INITIALIZE, pxenv_undi_initialize,
		       struct s_PXENV_UNDI_INITIALIZE ),
	PXE_API_CALL ( PXENV_UNDI_RESET_ADAPTER, pxenv_undi_reset_adapter,
		       struct s_PXENV_UNDI_RESET ),
	PXE_API_CALL ( PXENV_UNDI_SHUTDOWN, pxenv_undi_shutdown,
		       struct s_PXENV_UNDI_SHUTDOWN ),
	PXE_API_CALL ( PXENV_UNDI_OPEN, pxenv_undi_open,
		       struct s_PXENV_UNDI_OPEN ),
	PXE_API_CALL ( PXENV_UNDI_CLOSE, pxenv_undi_close,
		       struct s_PXENV_UNDI_CLOSE ),
	PXE_API_CALL ( PXENV_UNDI_TRANSMIT, pxenv_undi_transmit,
		       struct s_PXENV_UNDI_TRANSMIT ),
	PXE_API_CALL ( PXENV_UNDI_SET_MCAST_ADDRESS,
		       pxenv_undi_set_mcast_address,
		       struct s_PXENV_UNDI_SET_MCAST_ADDRESS ),
	PXE_API_CALL ( PXENV_UNDI_SET_STATION_ADDRESS,
		       pxenv_undi_set_station_address,
		       struct s_PXENV_UNDI_SET_STATION_ADDRESS ),
	PXE_API_CALL ( PXENV_UNDI_SET_PACKET_FILTER,
		       pxenv_undi_set_packet_filter,
		       struct s_PXENV_UNDI_SET_PACKET_FILTER ),
	PXE_API_CALL ( PXENV_UNDI_GET_INFORMATION, pxenv_undi_get_information,
		       struct s_PXENV_UNDI_GET_INFORMATION ),
	PXE_API_CALL ( PXENV_UNDI_GET_STATISTICS, pxenv_undi_get_statistics,
		       struct s_PXENV_UNDI_GET_STATISTICS ),
	PXE_API_CALL ( PXENV_UNDI_CLEAR_STATISTICS, pxenv_undi_clear_statistics,
		       struct s_PXENV_UNDI_CLEAR_STATISTICS ),
	PXE_API_CALL ( PXENV_UNDI_INITIATE_DIAGS, pxenv_undi_initiate_diags,
		       struct s_PXENV_UNDI_INITIATE_DIAGS ),
	PXE_API_CALL ( PXENV_UNDI_FORCE_INTERRUPT, pxenv_undi_force_interrupt,
		       struct s_PXENV_UNDI_FORCE_INTERRUPT ),
	PXE_API_CALL ( PXENV_UNDI_GET_MCAST_ADDRESS,
		       pxenv_undi_get_mcast_address,
		       struct s_PXENV_UNDI_GET_MCAST_ADDRESS ),
	PXE_API_CALL ( PXENV_UNDI_GET_NIC_TYPE, pxenv_undi_get_nic_type,
		       struct s_PXENV_UNDI_GET_NIC_TYPE ),
	PXE_API_CALL ( PXENV_UNDI_GET_IFACE_INFO, pxenv_undi_get_iface_info,
		       struct s_PXENV_UNDI_GET_IFACE_INFO ),
	PXE_API_CALL ( PXENV_UNDI_ISR, pxenv_undi_isr,
		       struct s_PXENV_UNDI_ISR ),
};
