/*
 * Copyright (C) 2006 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ipxe/netdevice.h>
#include <ipxe/vlan.h>
#include <ipxe/dhcp.h>
#include <ipxe/settings.h>
#include <ipxe/image.h>
#include <ipxe/sanboot.h>
#include <ipxe/uri.h>
#include <ipxe/open.h>
#include <ipxe/init.h>
#include <ipxe/keys.h>
#include <ipxe/version.h>
#include <ipxe/shell.h>
#include <ipxe/features.h>
#include <ipxe/image.h>
#include <ipxe/timer.h>
#include <usr/ifmgmt.h>
#include <usr/route.h>
#include <usr/imgmgmt.h>
#include <usr/prompt.h>
#include <usr/autoboot.h>
#include <config/general.h>
#include <config/branding.h>

/** @file
 *
 * Automatic booting
 *
 */

/** Link-layer address of preferred autoboot device, if known */
static uint8_t autoboot_ll_addr[MAX_LL_ADDR_LEN];

/** VLAN tag of preferred autoboot device, if known */
static unsigned int autoboot_vlan;

/** Device location of preferred autoboot device, if known */
static struct device_description autoboot_desc;

/** Autoboot device tester */
static int ( * is_autoboot_device ) ( struct net_device *netdev );

/* Disambiguate the various error causes */
#define ENOENT_BOOT __einfo_error ( EINFO_ENOENT_BOOT )
#define EINFO_ENOENT_BOOT \
	__einfo_uniqify ( EINFO_ENOENT, 0x01, "Nothing to boot" )

#define NORMAL	"\033[0m"
#define BOLD	"\033[1m"
#define CYAN	"\033[36m"

/** The "scriptlet" setting */
const struct setting scriptlet_setting __setting ( SETTING_MISC, scriptlet ) = {
	.name = "scriptlet",
	.description = "Boot scriptlet",
	.tag = DHCP_EB_SCRIPTLET,
	.type = &setting_type_string,
};

/**
 * Perform PXE menu boot when PXE stack is not available
 */
__weak int pxe_menu_boot ( struct net_device *netdev __unused ) {
	return -ENOTSUP;
}

/** The "keep-san" setting */
const struct setting keep_san_setting __setting ( SETTING_SANBOOT_EXTRA,
						  keep-san ) = {
	.name = "keep-san",
	.description = "Preserve SAN connection",
	.tag = DHCP_EB_KEEP_SAN,
	.type = &setting_type_int8,
};

/** The "skip-san-boot" setting */
const struct setting skip_san_boot_setting __setting ( SETTING_SANBOOT_EXTRA,
						       skip-san-boot ) = {
	.name = "skip-san-boot",
	.description = "Do not boot from SAN device",
	.tag = DHCP_EB_SKIP_SAN_BOOT,
	.type = &setting_type_int8,
};

/**
 * Boot from filename and root-path URIs
 *
 * @v filename		Filename
 * @v root_paths	Root path(s)
 * @v root_path_count	Number of root paths
 * @v drive		SAN drive (if applicable)
 * @v san_filename	SAN filename (or NULL to use default)
 * @v flags		Boot action flags
 * @ret rc		Return status code
 *
 * The somewhat tortuous flow of control in this function exists in
 * order to ensure that the "sanboot" command remains identical in
 * function to a SAN boot via a DHCP-specified root path, and to
 * provide backwards compatibility for the "keep-san" and
 * "skip-san-boot" options.
 */
int uriboot ( struct uri *filename, struct uri **root_paths,
	      unsigned int root_path_count, int drive,
	      const char *san_filename, unsigned int flags ) {

	struct image *image;
	int rc;

	/* Hook SAN device, if applicable */
	if ( root_path_count ) {
		drive = san_hook ( drive, root_paths, root_path_count,
				   ( ( flags & URIBOOT_NO_SAN_DESCRIBE ) ?
				     SAN_NO_DESCRIBE : 0 ) );
		if ( drive < 0 ) {
			rc = drive;
			printf ( "Could not open SAN device: %s\n",
				 strerror ( rc ) );
			goto err_san_hook;
		}
		printf ( "Registered SAN device %#02x\n", drive );
	}

	/* Describe SAN device, if applicable */
	if ( ! ( flags & URIBOOT_NO_SAN_DESCRIBE ) ) {
		if ( ( rc = san_describe() ) != 0 ) {
			printf ( "Could not describe SAN devices: %s\n",
				 strerror ( rc ) );
			goto err_san_describe;
		}
	}

	/* Allow a root-path-only boot with skip-san enabled to succeed */
	rc = 0;

	/* Attempt filename boot if applicable */
	if ( filename ) {
		//if ( ( rc = imgdownload ( filename, 0, &image ) ) != 0 )
		//	goto err_download;
		//imgstat ( image );
		//image->flags |= IMAGE_AUTO_UNREGISTER;
		if ( ( rc = image_exec ( image ) ) != 0 ) {
			printf ( "Could not boot image: %s\n",
				 strerror ( rc ) );
			/* Fall through to (possibly) attempt a SAN boot
			 * as a fallback.  If no SAN boot is attempted,
			 * our status will become the return status.
			 */
		} else {
			/* Always print an extra newline, because we
			 * don't know where the NBP may have left the
			 * cursor.
			 */
			printf ( "\n" );
		}
	}

	/* Attempt SAN boot if applicable */
	if ( ! ( flags & URIBOOT_NO_SAN_BOOT ) ) {
		if ( fetch_intz_setting ( NULL, &skip_san_boot_setting) == 0 ) {
			printf ( "Booting%s%s from SAN device %#02x\n",
				 ( san_filename ? " " : "" ),
				 ( san_filename ? san_filename : "" ), drive );

			rc = san_boot ( drive, san_filename );
			printf ( "Boot from SAN device %#02x failed: %s\n",
				 drive, strerror ( rc ) );
		} else {
			printf ( "Skipping boot from SAN device %#02x\n",
				 drive );
			/* Avoid overwriting a possible failure status
			 * from a filename boot.
			 */
		}
	}

 err_download:
 err_san_describe:
	/* Unhook SAN device, if applicable */
	if ( ! ( flags & URIBOOT_NO_SAN_UNHOOK ) ) {
		if ( fetch_intz_setting ( NULL, &keep_san_setting ) == 0 ) {
			san_unhook ( drive );
			printf ( "Unregistered SAN device %#02x\n", drive );
		} else {
			printf ( "Preserving SAN device %#02x\n", drive );
		}
	}
 err_san_hook:
	return rc;
}

/**
 * Close all but one network device
 *
 * Called before a fresh boot attempt in order to free up memory.  We
 * don't just close the device immediately after the boot fails,
 * because there may still be TCP connections in the process of
 * closing.
 */
static void close_other_netdevs ( struct net_device *netdev ) {
	struct net_device *other;

	for_each_netdev ( other ) {
		if ( other != netdev )
			ifclose ( other );
	}
}

/**
 * Fetch next-server and filename settings into a URI
 *
 * @v settings		Settings block
 * @ret uri		URI, or NULL on failure
 */
struct uri * fetch_next_server_and_filename ( struct settings *settings ) {
}

/**
 * Fetch root-path setting into a URI
 *
 * @v settings		Settings block
 * @ret uri		URI, or NULL on failure
 */
static struct uri * fetch_root_path ( struct settings *settings ) {
}

/**
 * Fetch san-filename setting
 *
 * @v settings		Settings block
 * @ret san_filename	SAN filename, or NULL on failure
 */
static char * fetch_san_filename ( struct settings *settings ) {
}

/**
 * Check whether or not we have a usable PXE menu
 *
 * @ret have_menu	A usable PXE menu is present
 */
static int have_pxe_menu ( void ) {
}

/**
 * Boot from a network device
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
int netboot ( struct net_device *netdev ) {
}

/**
 * Test if network device matches the autoboot device bus type and location
 *
 * @v netdev		Network device
 * @ret is_autoboot	Network device matches the autoboot device
 */
static int is_autoboot_busloc ( struct net_device *netdev ) {
	struct device *dev;

	for ( dev = netdev->dev ; dev ; dev = dev->parent ) {
		if ( ( dev->desc.bus_type == autoboot_desc.bus_type ) &&
		     ( dev->desc.location == autoboot_desc.location ) )
			return 1;
	}
	return 0;
}

/**
 * Identify autoboot device by bus type and location
 *
 * @v bus_type		Bus type
 * @v location		Location
 */
void set_autoboot_busloc ( unsigned int bus_type, unsigned int location ) {

	/* Record autoboot device description */
	autoboot_desc.bus_type = bus_type;
	autoboot_desc.location = location;

	/* Mark autoboot device as present */
	is_autoboot_device = is_autoboot_busloc;
}

/**
 * Identify autoboot device by link-layer address
 *
 * @v ll_addr		Link-layer address
 * @v len		Length of link-layer address
 * @v vlan		VLAN tag
 */
void set_autoboot_ll_addr ( const void *ll_addr, size_t len,
			    unsigned int vlan ) {

}

/**
 * Boot the system
 */
static int autoboot ( void ) {
}

/**
 * Prompt for shell entry
 *
 * @ret	enter_shell	User wants to enter shell
 */
static int shell_banner ( void ) {

	/* Skip prompt if timeout is zero */
	if ( BANNER_TIMEOUT <= 0 )
		return 0;

	/* Prompt user */
	printf ( "\n" );
	return ( prompt ( "Press Ctrl-B for the " PRODUCT_SHORT_NAME
			  " command line...",
			  ( ( BANNER_TIMEOUT * TICKS_PER_SEC ) / 10 ),
			  CTRL_B ) == 0 );
}

/**
 * Main iPXE flow of execution
 *
 * @v netdev		Network device, or NULL
 * @ret rc		Return status code
 */
int ipxe ( struct net_device *netdev ) {
	struct feature *feature;
	struct image *image;
	char *scriptlet;
	int rc;

	/*
	 * Print welcome banner
	 *
	 *
	 * If you wish to brand this build of iPXE, please do so by
	 * defining the string PRODUCT_NAME in config/branding.h.
	 *
	 * While nothing in the GPL prevents you from removing all
	 * references to iPXE or https://ipxe.org, we prefer you not
	 * to do so.
	 *
	 */
	//printf ( NORMAL "\n\n" PRODUCT_NAME "\n" BOLD PRODUCT_SHORT_NAME " %s"
	//	 NORMAL " -- " PRODUCT_TAG_LINE " -- "
	//	 CYAN PRODUCT_URI NORMAL "\nFeatures:", product_version );
	//for_each_table_entry ( feature, FEATURES )
	//	printf ( " %s", feature->name );

    printf ( NORMAL "\n\n" BOLD PRODUCT_SHORT_NAME
             NORMAL " -- " PRODUCT_TAG_LINE " -- " NORMAL );

	printf ( "\n" );

    struct uri *test = parse_uri("");
    struct uri *nvme_uri = parse_uri("nvme:0");
    struct uri *nvme_uris[] = {nvme_uri};

    //uriboot(NULL, nvme_uris, 1, 0x80, NULL, 0);

    struct uri *filename = NULL;
    struct uri **root_paths = nvme_uris;
    unsigned int root_path_count = 1;
    int drive = 0x80;
    const char *san_filename = NULL;
    unsigned int flags = 0;

    drive = san_hook ( drive, root_paths, root_path_count,
                       ( ( flags & URIBOOT_NO_SAN_DESCRIBE ) ?
                         SAN_NO_DESCRIBE : 0 ) );
    if ( drive < 0 ) {
        rc = drive;
        printf ( "Could not open SAN device: %s\n",
                 strerror ( rc ) );
        return rc;
    }
    printf ( "Registered SAN device %#02x\n", drive );

    if ( ( rc = san_describe() ) != 0 ) {
        printf ( "Could not describe SAN devices: %s\n",
                 strerror ( rc ) );
        return rc;
    }

    printf ( "Booting%s%s from SAN device %#02x\n",
             ( san_filename ? " " : "" ),
             ( san_filename ? san_filename : "" ), drive );
    rc = san_boot ( drive, san_filename );
    printf ( "Boot from SAN device %#02x failed: %s\n",
             drive, strerror ( rc ) );
    return rc;
}
