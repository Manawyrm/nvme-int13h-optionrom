/** @file
 *
 * PXE Preboot API
 *
 */

/* PXE API interface for Etherboot.
 *
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
#include <string.h>
#include <stdlib.h>
#include <ipxe/uaccess.h>
#include <ipxe/dhcp.h>
#include <ipxe/fakedhcp.h>
#include <ipxe/device.h>
#include <ipxe/netdevice.h>
#include <ipxe/isapnp.h>
#include <ipxe/init.h>
#include <ipxe/if_ether.h>
#include <basemem_packet.h>
#include <biosint.h>
#include <rmsetjmp.h>
#include "pxe.h"
#include "pxe_call.h"

/* Avoid dragging in isapnp.o unnecessarily */
uint16_t isapnp_read_port;

/** Zero-based versions of PXENV_GET_CACHED_INFO::PacketType */
enum pxe_cached_info_indices {
	CACHED_INFO_DHCPDISCOVER = ( PXENV_PACKET_TYPE_DHCP_DISCOVER - 1 ),
	CACHED_INFO_DHCPACK = ( PXENV_PACKET_TYPE_DHCP_ACK - 1 ),
	CACHED_INFO_BINL = ( PXENV_PACKET_TYPE_CACHED_REPLY - 1 ),
	NUM_CACHED_INFOS
};

/** A cached DHCP packet */
union pxe_cached_info {
	struct dhcphdr dhcphdr;
	/* This buffer must be *exactly* the size of a BOOTPLAYER_t
	 * structure, otherwise WinPE will die horribly.  It takes the
	 * size of *our* buffer and feeds it in to us as the size of
	 * one of *its* buffers.  If our buffer is larger than it
	 * expects, we therefore end up overwriting part of its data
	 * segment, since it tells us to do so.  (D'oh!)
	 *
	 * Note that a BOOTPLAYER_t is not necessarily large enough to
	 * hold a DHCP packet; this is a flaw in the PXE spec.
	 */
	BOOTPLAYER_t packet;
} __attribute__ (( packed ));

/** A PXE DHCP packet creator */
struct pxe_dhcp_packet_creator {
	/** Create DHCP packet
	 *
	 * @v netdev		Network device
	 * @v data		Buffer for DHCP packet
	 * @v max_len		Size of DHCP packet buffer
	 * @ret rc		Return status code
	 */
	int ( * create ) ( struct net_device *netdev, void *data,
			   size_t max_len );
};

/** PXE DHCP packet creators */
static struct pxe_dhcp_packet_creator pxe_dhcp_packet_creators[] = {
	[CACHED_INFO_DHCPDISCOVER] = { create_fakedhcpdiscover },
	[CACHED_INFO_DHCPACK] = { create_fakedhcpack },
	[CACHED_INFO_BINL] = { create_fakepxebsack },
};

/**
 * Name PXENV_GET_CACHED_INFO packet type
 *
 * @v packet_type	Packet type
 * @ret name		Name of packet type
 */
static inline __attribute__ (( always_inline )) const char *
pxenv_get_cached_info_name ( int packet_type ) {
    return "<INVALID>";
}

/* The case in which the caller doesn't supply a buffer is really
 * awkward to support given that we have multiple sources of options,
 * and that we don't actually store the DHCP packets.  (We may not
 * even have performed DHCP; we may have obtained all configuration
 * from non-volatile stored options or from the command line.)
 *
 * Some NBPs rely on the buffers we provide being persistent, so we
 * can't just use the temporary packet buffer.  4.5kB of base memory
 * always wasted just because some clients are too lazy to provide
 * their own buffers...
 */

/**
 * Construct cached DHCP packets
 *
 */
void pxe_fake_cached_info ( void ) {

}

/**
 * UNLOAD BASE CODE STACK
 *
 * @v None				-
 * @ret ...
 *
 */
static PXENV_EXIT_t
pxenv_unload_stack ( struct s_PXENV_UNLOAD_STACK *unload_stack ) {

	return PXENV_EXIT_SUCCESS;
}

/* PXENV_GET_CACHED_INFO
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_get_cached_info ( struct s_PXENV_GET_CACHED_INFO *get_cached_info ) {
	return PXENV_EXIT_SUCCESS;
}

/* PXENV_RESTART_TFTP
 *
 * Status: working
 */
static PXENV_EXIT_t
pxenv_restart_tftp ( struct s_PXENV_TFTP_READ_FILE *restart_tftp ) {
	PXENV_EXIT_t tftp_exit;

}

/* PXENV_START_UNDI
 *
 * Status: working
 */
static PXENV_EXIT_t pxenv_start_undi ( struct s_PXENV_START_UNDI *start_undi ) {

	return PXENV_EXIT_SUCCESS;
}

/* PXENV_STOP_UNDI
 *
 * Status: working
 */
static PXENV_EXIT_t pxenv_stop_undi ( struct s_PXENV_STOP_UNDI *stop_undi ) {

	return PXENV_EXIT_SUCCESS;
}

/* PXENV_START_BASE
 *
 * Status: won't implement (requires major structural changes)
 */
static PXENV_EXIT_t pxenv_start_base ( struct s_PXENV_START_BASE *start_base ) {

	return PXENV_EXIT_FAILURE;
}

/* PXENV_STOP_BASE
 *
 * Status: working
 */
static PXENV_EXIT_t pxenv_stop_base ( struct s_PXENV_STOP_BASE *stop_base ) {

	return PXENV_EXIT_SUCCESS;
}

/** PXE preboot API */
struct pxe_api_call pxe_preboot_api[] __pxe_api_call = {
	PXE_API_CALL ( PXENV_UNLOAD_STACK, pxenv_unload_stack,
		       struct s_PXENV_UNLOAD_STACK ),
	PXE_API_CALL ( PXENV_GET_CACHED_INFO, pxenv_get_cached_info,
		       struct s_PXENV_GET_CACHED_INFO ),
	PXE_API_CALL ( PXENV_RESTART_TFTP, pxenv_restart_tftp,
		       struct s_PXENV_TFTP_READ_FILE ),
	PXE_API_CALL ( PXENV_START_UNDI, pxenv_start_undi,
		       struct s_PXENV_START_UNDI ),
	PXE_API_CALL ( PXENV_STOP_UNDI, pxenv_stop_undi,
		       struct s_PXENV_STOP_UNDI ),
	PXE_API_CALL ( PXENV_START_BASE, pxenv_start_base,
		       struct s_PXENV_START_BASE ),
	PXE_API_CALL ( PXENV_STOP_BASE, pxenv_stop_base,
		       struct s_PXENV_STOP_BASE ),
};
