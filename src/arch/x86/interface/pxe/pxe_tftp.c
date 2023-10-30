/** @file
 *
 * PXE TFTP API
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/uaccess.h>
#include <ipxe/in.h>
#include <ipxe/tftp.h>
#include <ipxe/iobuf.h>
#include <ipxe/xfer.h>
#include <ipxe/open.h>
#include <ipxe/process.h>
#include <ipxe/uri.h>
#include <realmode.h>
#include <pxe.h>

/** A PXE TFTP connection */
struct pxe_tftp_connection {
	/** Data transfer interface */
	struct interface xfer;
	/** Data buffer */
	userptr_t buffer;
	/** Size of data buffer */
	size_t size;
	/** Starting offset of data buffer */
	size_t start;
	/** File position */
	size_t offset;
	/** Maximum file position */
	size_t max_offset;
	/** Block size */
	size_t blksize;
	/** Block index */
	unsigned int blkidx;
	/** Overall return status code */
	int rc;
};

/**
 * Close PXE TFTP connection
 *
 * @v pxe_tftp		PXE TFTP connection
 * @v rc		Final status code
 */
static void pxe_tftp_close ( struct pxe_tftp_connection *pxe_tftp, int rc ) {
	intf_shutdown ( &pxe_tftp->xfer, rc );
	pxe_tftp->rc = rc;
}

/**
 * Check flow control window
 *
 * @v pxe_tftp		PXE TFTP connection
 * @ret len		Length of window
 */
static size_t pxe_tftp_xfer_window ( struct pxe_tftp_connection *pxe_tftp ) {
	return pxe_tftp->blksize;
}

/**
 * Receive new data
 *
 * @v pxe_tftp		PXE TFTP connection
 * @v iobuf		I/O buffer
 * @v meta		Transfer metadata
 * @ret rc		Return status code
 */
static int pxe_tftp_xfer_deliver ( struct pxe_tftp_connection *pxe_tftp,
				   struct io_buffer *iobuf,
				   struct xfer_metadata *meta ) {
	return 0;
}

/** PXE TFTP connection interface operations */
static struct interface_operation pxe_tftp_xfer_ops[] = {
	INTF_OP ( xfer_deliver, struct pxe_tftp_connection *,
		  pxe_tftp_xfer_deliver ),
	INTF_OP ( xfer_window, struct pxe_tftp_connection *,
		  pxe_tftp_xfer_window ),
	INTF_OP ( intf_close, struct pxe_tftp_connection *, pxe_tftp_close ),
};

/** PXE TFTP connection interface descriptor */
static struct interface_descriptor pxe_tftp_xfer_desc =
	INTF_DESC ( struct pxe_tftp_connection, xfer, pxe_tftp_xfer_ops );

/** The PXE TFTP connection */
static struct pxe_tftp_connection pxe_tftp = {
	.xfer = INTF_INIT ( pxe_tftp_xfer_desc ),
};

/**
 * Open PXE TFTP connection
 *
 * @v ipaddress		IP address
 * @v port		TFTP server port (in network byte order)
 * @v filename		File name
 * @v blksize		Requested block size
 * @ret rc		Return status code
 */
static int pxe_tftp_open ( IP4_t ipaddress, UDP_PORT_t port,
			   UINT8_t *filename, UINT16_t blksize ) {
	return 0;
}

/**
 * TFTP OPEN
 *
 * @v tftp_open				Pointer to a struct s_PXENV_TFTP_OPEN
 * @v s_PXENV_TFTP_OPEN::ServerIPAddress TFTP server IP address
 * @v s_PXENV_TFTP_OPEN::GatewayIPAddress Relay agent IP address, or 0.0.0.0
 * @v s_PXENV_TFTP_OPEN::FileName	Name of file to open
 * @v s_PXENV_TFTP_OPEN::TFTPPort	TFTP server UDP port
 * @v s_PXENV_TFTP_OPEN::PacketSize	TFTP blksize option to request
 * @ret #PXENV_EXIT_SUCCESS		File was opened
 * @ret #PXENV_EXIT_FAILURE		File was not opened
 * @ret s_PXENV_TFTP_OPEN::Status	PXE status code
 * @ret s_PXENV_TFTP_OPEN::PacketSize	Negotiated blksize
 * @err #PXENV_STATUS_TFTP_INVALID_PACKET_SIZE Requested blksize too small
 *
 * Opens a TFTP connection for downloading a file a block at a time
 * using pxenv_tftp_read().
 *
 * If s_PXENV_TFTP_OPEN::GatewayIPAddress is 0.0.0.0, normal IP
 * routing will take place.  See the relevant
 * @ref pxe_routing "implementation note" for more details.
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 * 
 * @note According to the PXE specification version 2.1, this call
 * "opens a file for reading/writing", though how writing is to be
 * achieved without the existence of an API call %pxenv_tftp_write()
 * is not made clear.
 *
 * @note Despite the existence of the numerous statements within the
 * PXE specification of the form "...if a TFTP/MTFTP or UDP connection
 * is active...", you cannot use pxenv_tftp_open() and
 * pxenv_tftp_read() to read a file via MTFTP; only via plain old
 * TFTP.  If you want to use MTFTP, use pxenv_tftp_read_file()
 * instead.  Astute readers will note that, since
 * pxenv_tftp_read_file() is an atomic operation from the point of
 * view of the PXE API, it is conceptually impossible to issue any
 * other PXE API call "if an MTFTP connection is active".
 */
static PXENV_EXIT_t pxenv_tftp_open ( struct s_PXENV_TFTP_OPEN *tftp_open ) {
	return PXENV_EXIT_FAILURE;
}

/**
 * TFTP CLOSE
 *
 * @v tftp_close			Pointer to a struct s_PXENV_TFTP_CLOSE
 * @ret #PXENV_EXIT_SUCCESS		File was closed successfully
 * @ret #PXENV_EXIT_FAILURE		File was not closed
 * @ret s_PXENV_TFTP_CLOSE::Status	PXE status code
 * @err None				-
 *
 * Close a connection previously opened with pxenv_tftp_open().  You
 * must have previously opened a connection with pxenv_tftp_open().
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 */
static PXENV_EXIT_t pxenv_tftp_close ( struct s_PXENV_TFTP_CLOSE *tftp_close ) {
	return PXENV_EXIT_SUCCESS;
}

/**
 * TFTP READ
 *
 * @v tftp_read				Pointer to a struct s_PXENV_TFTP_READ
 * @v s_PXENV_TFTP_READ::Buffer		Address of data buffer
 * @ret #PXENV_EXIT_SUCCESS		Data was read successfully
 * @ret #PXENV_EXIT_FAILURE		Data was not read
 * @ret s_PXENV_TFTP_READ::Status	PXE status code
 * @ret s_PXENV_TFTP_READ::PacketNumber	TFTP packet number
 * @ret s_PXENV_TFTP_READ::BufferSize	Length of data written into buffer
 *
 * Reads a single packet from a connection previously opened with
 * pxenv_tftp_open() into the data buffer pointed to by
 * s_PXENV_TFTP_READ::Buffer.  You must have previously opened a
 * connection with pxenv_tftp_open().  The data written into
 * s_PXENV_TFTP_READ::Buffer is just the file data; the various
 * network headers have already been removed.
 *
 * The buffer must be large enough to contain a packet of the size
 * negotiated via the s_PXENV_TFTP_OPEN::PacketSize field in the
 * pxenv_tftp_open() call.  It is worth noting that the PXE
 * specification does @b not require the caller to fill in
 * s_PXENV_TFTP_READ::BufferSize before calling pxenv_tftp_read(), so
 * the PXE stack is free to ignore whatever value the caller might
 * place there and just assume that the buffer is large enough.  That
 * said, it may be worth the caller always filling in
 * s_PXENV_TFTP_READ::BufferSize to guard against PXE stacks that
 * mistake it for an input parameter.
 *
 * The length of the TFTP data packet will be returned via
 * s_PXENV_TFTP_READ::BufferSize.  If this length is less than the
 * blksize negotiated via s_PXENV_TFTP_OPEN::PacketSize in the call to
 * pxenv_tftp_open(), this indicates that the block is the last block
 * in the file.  Note that zero is a valid length for
 * s_PXENV_TFTP_READ::BufferSize, and will occur when the length of
 * the file is a multiple of the blksize.
 *
 * The PXE specification doesn't actually state that calls to
 * pxenv_tftp_read() will return the data packets in strict sequential
 * order, though most PXE stacks will probably do so.  The sequence
 * number of the packet will be returned in
 * s_PXENV_TFTP_READ::PacketNumber.  The first packet in the file has
 * a sequence number of one, not zero.
 *
 * To guard against flawed PXE stacks, the caller should probably set
 * s_PXENV_TFTP_READ::PacketNumber to one less than the expected
 * returned value (i.e. set it to zero for the first call to
 * pxenv_tftp_read() and then re-use the returned s_PXENV_TFTP_READ
 * parameter block for subsequent calls without modifying
 * s_PXENV_TFTP_READ::PacketNumber between calls).  The caller should
 * also guard against potential problems caused by flawed
 * implementations returning the occasional duplicate packet, by
 * checking that the value returned in s_PXENV_TFTP_READ::PacketNumber
 * is as expected (i.e. one greater than that returned from the
 * previous call to pxenv_tftp_read()).
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 */
static PXENV_EXIT_t pxenv_tftp_read ( struct s_PXENV_TFTP_READ *tftp_read ) {
	return PXENV_EXIT_FAILURE;
}

/**
 * TFTP/MTFTP read file
 *
 * @v tftp_read_file		     Pointer to a struct s_PXENV_TFTP_READ_FILE
 * @v s_PXENV_TFTP_READ_FILE::FileName		File name
 * @v s_PXENV_TFTP_READ_FILE::BufferSize 	Size of the receive buffer
 * @v s_PXENV_TFTP_READ_FILE::Buffer		Address of the receive buffer
 * @v s_PXENV_TFTP_READ_FILE::ServerIPAddress	TFTP server IP address
 * @v s_PXENV_TFTP_READ_FILE::GatewayIPAddress	Relay agent IP address
 * @v s_PXENV_TFTP_READ_FILE::McastIPAddress	File's multicast IP address
 * @v s_PXENV_TFTP_READ_FILE::TFTPClntPort	Client multicast UDP port
 * @v s_PXENV_TFTP_READ_FILE::TFTPSrvPort	Server multicast UDP port
 * @v s_PXENV_TFTP_READ_FILE::TFTPOpenTimeOut	Time to wait for first packet
 * @v s_PXENV_TFTP_READ_FILE::TFTPReopenDelay	MTFTP inactivity timeout
 * @ret #PXENV_EXIT_SUCCESS			File downloaded successfully
 * @ret #PXENV_EXIT_FAILURE			File not downloaded
 * @ret s_PXENV_TFTP_READ_FILE::Status		PXE status code
 * @ret s_PXENV_TFTP_READ_FILE::BufferSize	Length of downloaded file
 *
 * Downloads an entire file via either TFTP or MTFTP into the buffer
 * pointed to by s_PXENV_TFTP_READ_FILE::Buffer.
 *
 * The PXE specification does not make it clear how the caller
 * requests that MTFTP be used rather than TFTP (or vice versa).  One
 * reasonable guess is that setting
 * s_PXENV_TFTP_READ_FILE::McastIPAddress to 0.0.0.0 would cause TFTP
 * to be used instead of MTFTP, though it is conceivable that some PXE
 * stacks would interpret that as "use the DHCP-provided multicast IP
 * address" instead.  Some PXE stacks will not implement MTFTP at all,
 * and will always use TFTP.
 *
 * It is not specified whether or not
 * s_PXENV_TFTP_READ_FILE::TFTPSrvPort will be used as the TFTP server
 * port for TFTP (rather than MTFTP) downloads.  Callers should assume
 * that the only way to access a TFTP server on a non-standard port is
 * to use pxenv_tftp_open() and pxenv_tftp_read().
 *
 * If s_PXENV_TFTP_READ_FILE::GatewayIPAddress is 0.0.0.0, normal IP
 * routing will take place.  See the relevant
 * @ref pxe_routing "implementation note" for more details.
 *
 * It is interesting to note that s_PXENV_TFTP_READ_FILE::Buffer is an
 * #ADDR32_t type, i.e. nominally a flat physical address.  Some PXE
 * NBPs (e.g. NTLDR) are known to call pxenv_tftp_read_file() in real
 * mode with s_PXENV_TFTP_READ_FILE::Buffer set to an address above
 * 1MB.  This means that PXE stacks must be prepared to write to areas
 * outside base memory.  Exactly how this is to be achieved is not
 * specified, though using INT 15,87 is as close to a standard method
 * as any, and should probably be used.  Switching to protected-mode
 * in order to access high memory will fail if pxenv_tftp_read_file()
 * is called in V86 mode; it is reasonably to expect that a V86
 * monitor would intercept the relatively well-defined INT 15,87 if it
 * wants the PXE stack to be able to write to high memory.
 *
 * Things get even more interesting if pxenv_tftp_read_file() is
 * called in protected mode, because there is then absolutely no way
 * for the PXE stack to write to an absolute physical address.  You
 * can't even get around the problem by creating a special "access
 * everything" segment in the s_PXE data structure, because the
 * #SEGDESC_t descriptors are limited to 64kB in size.
 *
 * Previous versions of the PXE specification (e.g. WfM 1.1a) provide
 * a separate API call, %pxenv_tftp_read_file_pmode(), specifically to
 * work around this problem.  The s_PXENV_TFTP_READ_FILE_PMODE
 * parameter block splits s_PXENV_TFTP_READ_FILE::Buffer into
 * s_PXENV_TFTP_READ_FILE_PMODE::BufferSelector and
 * s_PXENV_TFTP_READ_FILE_PMODE::BufferOffset, i.e. it provides a
 * protected-mode segment:offset address for the data buffer.  This
 * API call is no longer present in version 2.1 of the PXE
 * specification.
 *
 * Etherboot makes the assumption that s_PXENV_TFTP_READ_FILE::Buffer
 * is an offset relative to the caller's data segment, when
 * pxenv_tftp_read_file() is called in protected mode.
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 */
PXENV_EXIT_t pxenv_tftp_read_file ( struct s_PXENV_TFTP_READ_FILE
				    *tftp_read_file ) {
	return PXENV_EXIT_FAILURE;
}

/**
 * TFTP GET FILE SIZE
 *
 * @v tftp_get_fsize		     Pointer to a struct s_PXENV_TFTP_GET_FSIZE
 * @v s_PXENV_TFTP_GET_FSIZE::ServerIPAddress	TFTP server IP address
 * @v s_PXENV_TFTP_GET_FSIZE::GatewayIPAddress	Relay agent IP address
 * @v s_PXENV_TFTP_GET_FSIZE::FileName	File name
 * @ret #PXENV_EXIT_SUCCESS		File size was determined successfully
 * @ret #PXENV_EXIT_FAILURE		File size was not determined
 * @ret s_PXENV_TFTP_GET_FSIZE::Status	PXE status code
 * @ret s_PXENV_TFTP_GET_FSIZE::FileSize	File size
 *
 * Determine the size of a file on a TFTP server.  This uses the
 * "tsize" TFTP option, and so will not work with a TFTP server that
 * does not support TFTP options, or that does not support the "tsize"
 * option.
 *
 * The PXE specification states that this API call will @b not open a
 * TFTP connection for subsequent use with pxenv_tftp_read().  (This
 * is somewhat daft, since the only way to obtain the file size via
 * the "tsize" option involves issuing a TFTP open request, but that's
 * life.)
 *
 * You cannot call pxenv_tftp_get_fsize() while a TFTP or UDP
 * connection is open.
 *
 * If s_PXENV_TFTP_GET_FSIZE::GatewayIPAddress is 0.0.0.0, normal IP
 * routing will take place.  See the relevant
 * @ref pxe_routing "implementation note" for more details.
 *
 * On x86, you must set the s_PXE::StatusCallout field to a nonzero
 * value before calling this function in protected mode.  You cannot
 * call this function with a 32-bit stack segment.  (See the relevant
 * @ref pxe_x86_pmode16 "implementation note" for more details.)
 * 
 * @note There is no way to specify the TFTP server port with this API
 * call.  Though you can open a file using a non-standard TFTP server
 * port (via s_PXENV_TFTP_OPEN::TFTPPort or, potentially,
 * s_PXENV_TFTP_READ_FILE::TFTPSrvPort), you can only get the size of
 * a file from a TFTP server listening on the standard TFTP port.
 * "Consistency" is not a word in Intel's vocabulary.
 */
static PXENV_EXIT_t pxenv_tftp_get_fsize ( struct s_PXENV_TFTP_GET_FSIZE
					   *tftp_get_fsize ) {
	return PXENV_EXIT_FAILURE;
}

/** PXE TFTP API */
struct pxe_api_call pxe_tftp_api[] __pxe_api_call = {
	PXE_API_CALL ( PXENV_TFTP_OPEN, pxenv_tftp_open,
		       struct s_PXENV_TFTP_OPEN ),
	PXE_API_CALL ( PXENV_TFTP_CLOSE, pxenv_tftp_close,
		       struct s_PXENV_TFTP_CLOSE ),
	PXE_API_CALL ( PXENV_TFTP_READ, pxenv_tftp_read,
		       struct s_PXENV_TFTP_READ ),
	PXE_API_CALL ( PXENV_TFTP_READ_FILE, pxenv_tftp_read_file,
		       struct s_PXENV_TFTP_READ_FILE ),
	PXE_API_CALL ( PXENV_TFTP_GET_FSIZE, pxenv_tftp_get_fsize,
		       struct s_PXENV_TFTP_GET_FSIZE ),
};
