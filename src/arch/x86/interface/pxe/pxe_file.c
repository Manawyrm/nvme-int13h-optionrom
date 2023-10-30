/** @file
 *
 * PXE FILE API
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/uaccess.h>
#include <ipxe/posix_io.h>
#include <ipxe/features.h>
#include <pxe.h>
#include <realmode.h>

/*
 * Copyright (C) 2007 Michael Brown <mbrown@fensystems.co.uk>.
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

FEATURE ( FEATURE_MISC, "PXEXT", DHCP_EB_FEATURE_PXE_EXT, 2 );

/**
 * FILE OPEN
 *
 * @v file_open				Pointer to a struct s_PXENV_FILE_OPEN
 * @v s_PXENV_FILE_OPEN::FileName	URL of file to open
 * @ret #PXENV_EXIT_SUCCESS		File was opened
 * @ret #PXENV_EXIT_FAILURE		File was not opened
 * @ret s_PXENV_FILE_OPEN::Status	PXE status code
 * @ret s_PXENV_FILE_OPEN::FileHandle	Handle of opened file
 *
 */
static PXENV_EXIT_t pxenv_file_open ( struct s_PXENV_FILE_OPEN *file_open )
{
	return PXENV_EXIT_SUCCESS;
}

/**
 * FILE CLOSE
 *
 * @v file_close			Pointer to a struct s_PXENV_FILE_CLOSE
 * @v s_PXENV_FILE_CLOSE::FileHandle	File handle
 * @ret #PXENV_EXIT_SUCCESS		File was closed
 * @ret #PXENV_EXIT_FAILURE		File was not closed
 * @ret s_PXENV_FILE_CLOSE::Status	PXE status code
 *
 */
static PXENV_EXIT_t pxenv_file_close ( struct s_PXENV_FILE_CLOSE *file_close ) {
	return PXENV_EXIT_SUCCESS;
}

/**
 * FILE SELECT
 *
 * @v file_select			Pointer to a struct s_PXENV_FILE_SELECT
 * @v s_PXENV_FILE_SELECT::FileHandle	File handle
 * @ret #PXENV_EXIT_SUCCESS		File has been checked for readiness
 * @ret #PXENV_EXIT_FAILURE		File has not been checked for readiness
 * @ret s_PXENV_FILE_SELECT::Status	PXE status code
 * @ret s_PXENV_FILE_SELECT::Ready	Indication of readiness
 *
 */
static PXENV_EXIT_t
pxenv_file_select ( struct s_PXENV_FILE_SELECT *file_select ) {

	return PXENV_EXIT_SUCCESS;
}

/**
 * FILE READ
 *
 * @v file_read				Pointer to a struct s_PXENV_FILE_READ
 * @v s_PXENV_FILE_READ::FileHandle	File handle
 * @v s_PXENV_FILE_READ::BufferSize	Size of data buffer
 * @v s_PXENV_FILE_READ::Buffer		Data buffer
 * @ret #PXENV_EXIT_SUCCESS		Data has been read from file
 * @ret #PXENV_EXIT_FAILURE		Data has not been read from file
 * @ret s_PXENV_FILE_READ::Status	PXE status code
 * @ret s_PXENV_FILE_READ::Ready	Indication of readiness
 * @ret s_PXENV_FILE_READ::BufferSize	Length of data read
 *
 */
static PXENV_EXIT_t pxenv_file_read ( struct s_PXENV_FILE_READ *file_read ) {

	return PXENV_EXIT_SUCCESS;
}

/**
 * GET FILE SIZE
 *
 * @v get_file_size			Pointer to a struct s_PXENV_GET_FILE_SIZE
 * @v s_PXENV_GET_FILE_SIZE::FileHandle	File handle
 * @ret #PXENV_EXIT_SUCCESS		File size has been determined
 * @ret #PXENV_EXIT_FAILURE		File size has not been determined
 * @ret s_PXENV_GET_FILE_SIZE::Status	PXE status code
 * @ret s_PXENV_GET_FILE_SIZE::FileSize	Size of file
 */
static PXENV_EXIT_t
pxenv_get_file_size ( struct s_PXENV_GET_FILE_SIZE *get_file_size ) {

	return PXENV_EXIT_SUCCESS;
}

/**
 * FILE EXEC
 *
 * @v file_exec				Pointer to a struct s_PXENV_FILE_EXEC
 * @v s_PXENV_FILE_EXEC::Command	Command to execute
 * @ret #PXENV_EXIT_SUCCESS		Command was executed successfully
 * @ret #PXENV_EXIT_FAILURE		Command was not executed successfully
 * @ret s_PXENV_FILE_EXEC::Status	PXE status code
 *
 */
static PXENV_EXIT_t pxenv_file_exec ( struct s_PXENV_FILE_EXEC *file_exec ) {
	return PXENV_EXIT_SUCCESS;
}

/**
 * FILE CMDLINE
 *
 * @v file_cmdline			Pointer to a struct s_PXENV_FILE_CMDLINE
 * @v s_PXENV_FILE_CMDLINE::Buffer	Buffer to contain command line
 * @v s_PXENV_FILE_CMDLINE::BufferSize	Size of buffer
 * @ret #PXENV_EXIT_SUCCESS		Command was executed successfully
 * @ret #PXENV_EXIT_FAILURE		Command was not executed successfully
 * @ret s_PXENV_FILE_EXEC::Status	PXE status code
 * @ret s_PXENV_FILE_EXEC::BufferSize	Length of command line (including NUL)
 *
 */
static PXENV_EXIT_t
pxenv_file_cmdline ( struct s_PXENV_FILE_CMDLINE *file_cmdline ) {
	return PXENV_EXIT_SUCCESS;
}

/**
 * FILE API CHECK
 *
 * @v file_exec				Pointer to a struct s_PXENV_FILE_API_CHECK
 * @v s_PXENV_FILE_API_CHECK::Magic     Inbound magic number (0x91d447b2)
 * @ret #PXENV_EXIT_SUCCESS		Command was executed successfully
 * @ret #PXENV_EXIT_FAILURE		Command was not executed successfully
 * @ret s_PXENV_FILE_API_CHECK::Status	PXE status code
 * @ret s_PXENV_FILE_API_CHECK::Magic	Outbound magic number (0xe9c17b20)
 * @ret s_PXENV_FILE_API_CHECK::Provider "iPXE" (0x45585067)
 * @ret s_PXENV_FILE_API_CHECK::APIMask API function bitmask
 * @ret s_PXENV_FILE_API_CHECK::Flags	Reserved
 *
 */
static PXENV_EXIT_t
pxenv_file_api_check ( struct s_PXENV_FILE_API_CHECK *file_api_check ) {
	return PXENV_EXIT_SUCCESS;
}

/** PXE file API */
struct pxe_api_call pxe_file_api[] __pxe_api_call = {
	PXE_API_CALL ( PXENV_FILE_OPEN, pxenv_file_open,
		       struct s_PXENV_FILE_OPEN ),
	PXE_API_CALL ( PXENV_FILE_CLOSE, pxenv_file_close,
		       struct s_PXENV_FILE_CLOSE ),
	PXE_API_CALL ( PXENV_FILE_SELECT, pxenv_file_select,
		       struct s_PXENV_FILE_SELECT ),
	PXE_API_CALL ( PXENV_FILE_READ, pxenv_file_read,
		       struct s_PXENV_FILE_READ ),
	PXE_API_CALL ( PXENV_GET_FILE_SIZE, pxenv_get_file_size,
		       struct s_PXENV_GET_FILE_SIZE ),
	PXE_API_CALL ( PXENV_FILE_EXEC, pxenv_file_exec,
		       struct s_PXENV_FILE_EXEC ),
	PXE_API_CALL ( PXENV_FILE_CMDLINE, pxenv_file_cmdline,
		       struct s_PXENV_FILE_CMDLINE ),
	PXE_API_CALL ( PXENV_FILE_API_CHECK, pxenv_file_api_check,
		       struct s_PXENV_FILE_API_CHECK ),
};
