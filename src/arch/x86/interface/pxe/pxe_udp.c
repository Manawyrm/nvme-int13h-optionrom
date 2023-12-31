/** @file
 *
 * PXE UDP API
 *
 */

#include <string.h>
#include <byteswap.h>
#include <ipxe/iobuf.h>
#include <ipxe/xfer.h>
#include <ipxe/udp.h>
#include <ipxe/uaccess.h>
#include <ipxe/process.h>
#include <ipxe/netdevice.h>
#include <ipxe/malloc.h>
#include <realmode.h>
#include <pxe.h>

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

/** A PXE UDP pseudo-header */
struct pxe_udp_pseudo_header {
	/** Source IP address */
	IP4_t src_ip;
	/** Source port */
	UDP_PORT_t s_port;
	/** Destination IP address */
	IP4_t dest_ip;
	/** Destination port */
	UDP_PORT_t d_port;
} __attribute__ (( packed ));

/** A PXE UDP connection */
struct pxe_udp_connection {
	/** Data transfer interface to UDP stack */
	struct interface xfer;
	/** Local address */
	struct sockaddr_in local;
	/** List of received packets */
	struct list_head list;
};

/**
 * Receive PXE UDP data
 *
 * @v pxe_udp			PXE UDP connection
 * @v iobuf			I/O buffer
 * @v meta			Data transfer metadata
 * @ret rc			Return status code
 *
 * Receives a packet as part of the current pxenv_udp_read()
 * operation.
 */
static int pxe_udp_deliver ( struct pxe_udp_connection *pxe_udp,
			     struct io_buffer *iobuf,
			     struct xfer_metadata *meta ) {
	return 0;
}

/** PXE UDP data transfer interface operations */
static struct interface_operation pxe_udp_xfer_operations[] = {
	INTF_OP ( xfer_deliver, struct pxe_udp_connection *, pxe_udp_deliver ),
};

/** PXE UDP data transfer interface descriptor */
static struct interface_descriptor pxe_udp_xfer_desc =
	INTF_DESC ( struct pxe_udp_connection, xfer, pxe_udp_xfer_operations );

/** The PXE UDP connection */
static struct pxe_udp_connection pxe_udp = {
	.xfer = INTF_INIT ( pxe_udp_xfer_desc ),
	.local = {
		.sin_family = AF_INET,
	},
	.list = LIST_HEAD_INIT ( pxe_udp.list ),
};

/**
 * UDP OPEN
 *
 * @v pxenv_udp_open			Pointer to a struct s_PXENV_UDP_OPEN
 * @v s_PXENV_UDP_OPEN::src_ip		IP address of this station, or 0.0.0.0
 * @ret #PXENV_EXIT_SUCCESS		Always
 * @ret s_PXENV_UDP_OPEN::Status	PXE status code
 * @err #PXENV_STATUS_UDP_OPEN		UDP connection already open
 * @err #PXENV_STATUS_OUT_OF_RESOURCES	Could not open connection
 *
 * Prepares the PXE stack for communication using pxenv_udp_write()
 * and pxenv_udp_read().
 *
 * The IP address supplied in s_PXENV_UDP_OPEN::src_ip will be
 * recorded and used as the local station's IP address for all further
 * communication, including communication by means other than
 * pxenv_udp_write() and pxenv_udp_read().  (If
 * s_PXENV_UDP_OPEN::src_ip is 0.0.0.0, the local station's IP address
 * will remain unchanged.)
 *
 * You can only have one open UDP connection at a time.  This is not a
 * meaningful restriction, since pxenv_udp_write() and
 * pxenv_udp_read() allow you to specify arbitrary local and remote
 * ports and an arbitrary remote address for each packet.  According
 * to the PXE specifiation, you cannot have a UDP connection open at
 * the same time as a TFTP connection; this restriction does not apply
 * to Etherboot.
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 *
 * @note The PXE specification does not make it clear whether the IP
 * address supplied in s_PXENV_UDP_OPEN::src_ip should be used only
 * for this UDP connection, or retained for all future communication.
 * The latter seems more consistent with typical PXE stack behaviour.
 *
 * @note Etherboot currently ignores the s_PXENV_UDP_OPEN::src_ip
 * parameter.
 *
 */
static PXENV_EXIT_t pxenv_udp_open ( struct s_PXENV_UDP_OPEN *pxenv_udp_open )
{
	return PXENV_EXIT_SUCCESS;
}

/**
 * UDP CLOSE
 *
 * @v pxenv_udp_close			Pointer to a struct s_PXENV_UDP_CLOSE
 * @ret #PXENV_EXIT_SUCCESS		Always
 * @ret s_PXENV_UDP_CLOSE::Status	PXE status code
 * @err None				-
 *
 * Closes a UDP connection opened with pxenv_udp_open().
 *
 * You can only have one open UDP connection at a time.  You cannot
 * have a UDP connection open at the same time as a TFTP connection.
 * You cannot use pxenv_udp_close() to close a TFTP connection; use
 * pxenv_tftp_close() instead.
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 *
 */
static PXENV_EXIT_t
pxenv_udp_close ( struct s_PXENV_UDP_CLOSE *pxenv_udp_close ) {
	return PXENV_EXIT_SUCCESS;
}

/**
 * UDP WRITE
 *
 * @v pxenv_udp_write			Pointer to a struct s_PXENV_UDP_WRITE
 * @v s_PXENV_UDP_WRITE::ip		Destination IP address
 * @v s_PXENV_UDP_WRITE::gw		Relay agent IP address, or 0.0.0.0
 * @v s_PXENV_UDP_WRITE::src_port	Source UDP port, or 0
 * @v s_PXENV_UDP_WRITE::dst_port	Destination UDP port
 * @v s_PXENV_UDP_WRITE::buffer_size	Length of the UDP payload
 * @v s_PXENV_UDP_WRITE::buffer		Address of the UDP payload
 * @ret #PXENV_EXIT_SUCCESS		Packet was transmitted successfully
 * @ret #PXENV_EXIT_FAILURE		Packet could not be transmitted
 * @ret s_PXENV_UDP_WRITE::Status	PXE status code
 * @err #PXENV_STATUS_UDP_CLOSED	UDP connection is not open
 * @err #PXENV_STATUS_UNDI_TRANSMIT_ERROR Could not transmit packet
 *
 * Transmits a single UDP packet.  A valid IP and UDP header will be
 * prepended to the payload in s_PXENV_UDP_WRITE::buffer; the buffer
 * should not contain precomputed IP and UDP headers, nor should it
 * contain space allocated for these headers.  The first byte of the
 * buffer will be transmitted as the first byte following the UDP
 * header.
 *
 * If s_PXENV_UDP_WRITE::gw is 0.0.0.0, normal IP routing will take
 * place.  See the relevant @ref pxe_routing "implementation note" for
 * more details.
 *
 * If s_PXENV_UDP_WRITE::src_port is 0, port 2069 will be used.
 *
 * You must have opened a UDP connection with pxenv_udp_open() before
 * calling pxenv_udp_write().
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 *
 * @note Etherboot currently ignores the s_PXENV_UDP_WRITE::gw
 * parameter.
 *
 */
static PXENV_EXIT_t
pxenv_udp_write ( struct s_PXENV_UDP_WRITE *pxenv_udp_write ) {
	return PXENV_EXIT_SUCCESS;
}

/**
 * UDP READ
 *
 * @v pxenv_udp_read			Pointer to a struct s_PXENV_UDP_READ
 * @v s_PXENV_UDP_READ::dest_ip		Destination IP address, or 0.0.0.0
 * @v s_PXENV_UDP_READ::d_port		Destination UDP port, or 0
 * @v s_PXENV_UDP_READ::buffer_size	Size of the UDP payload buffer
 * @v s_PXENV_UDP_READ::buffer		Address of the UDP payload buffer
 * @ret #PXENV_EXIT_SUCCESS		A packet has been received
 * @ret #PXENV_EXIT_FAILURE		No packet has been received
 * @ret s_PXENV_UDP_READ::Status	PXE status code
 * @ret s_PXENV_UDP_READ::src_ip	Source IP address
 * @ret s_PXENV_UDP_READ::dest_ip	Destination IP address
 * @ret s_PXENV_UDP_READ::s_port	Source UDP port
 * @ret s_PXENV_UDP_READ::d_port	Destination UDP port
 * @ret s_PXENV_UDP_READ::buffer_size	Length of UDP payload
 * @err #PXENV_STATUS_UDP_CLOSED	UDP connection is not open
 * @err #PXENV_STATUS_FAILURE		No packet was ready to read
 *
 * Receive a single UDP packet.  This is a non-blocking call; if no
 * packet is ready to read, the call will return instantly with
 * s_PXENV_UDP_READ::Status==PXENV_STATUS_FAILURE.
 *
 * If s_PXENV_UDP_READ::dest_ip is 0.0.0.0, UDP packets addressed to
 * any IP address will be accepted and may be returned to the caller.
 *
 * If s_PXENV_UDP_READ::d_port is 0, UDP packets addressed to any UDP
 * port will be accepted and may be returned to the caller.
 *
 * You must have opened a UDP connection with pxenv_udp_open() before
 * calling pxenv_udp_read().
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 *
 * @note The PXE specification (version 2.1) does not state that we
 * should fill in s_PXENV_UDP_READ::dest_ip and
 * s_PXENV_UDP_READ::d_port, but Microsoft Windows' NTLDR program
 * expects us to do so, and will fail if we don't.
 *
 */
static PXENV_EXIT_t pxenv_udp_read ( struct s_PXENV_UDP_READ *pxenv_udp_read ) {
	return PXENV_EXIT_FAILURE;
}

/** PXE UDP API */
struct pxe_api_call pxe_udp_api[] __pxe_api_call = {
	PXE_API_CALL ( PXENV_UDP_OPEN, pxenv_udp_open,
		       struct s_PXENV_UDP_OPEN ),
	PXE_API_CALL ( PXENV_UDP_CLOSE, pxenv_udp_close,
		       struct s_PXENV_UDP_CLOSE ),
	PXE_API_CALL ( PXENV_UDP_WRITE, pxenv_udp_write,
		       struct s_PXENV_UDP_WRITE ),
	PXE_API_CALL ( PXENV_UDP_READ, pxenv_udp_read,
		       struct s_PXENV_UDP_READ ),
};

/**
 * Discard some cached PXE UDP data
 *
 * @ret discarded	Number of cached items discarded
 */
static unsigned int pxe_udp_discard ( void ) {
}

/** PXE UDP cache discarder */
struct cache_discarder pxe_udp_discarder __cache_discarder ( CACHE_NORMAL ) = {
	.discard = pxe_udp_discard,
};
