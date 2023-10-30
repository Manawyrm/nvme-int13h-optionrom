#ifndef CONFIG_GENERAL_H
#define CONFIG_GENERAL_H

/** @file
 *
 * General configuration
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <config/defaults.h>

/*
 * Banner timeout configuration
 *
 * This controls the timeout for the "Press Ctrl-B for the iPXE
 * command line" banner displayed when iPXE starts up.  The value is
 * specified in tenths of a second for which the banner should appear.
 * A value of 0 disables the banner.
 *
 * ROM_BANNER_TIMEOUT controls the "Press Ctrl-B to configure iPXE"
 * banner displayed only by ROM builds of iPXE during POST.  This
 * defaults to being twice the length of BANNER_TIMEOUT, to allow for
 * BIOSes that switch video modes immediately before calling the
 * initialisation vector, thus rendering the banner almost invisible
 * to the user.
 */
#define BANNER_TIMEOUT		20
#define ROM_BANNER_TIMEOUT	( 2 * BANNER_TIMEOUT )

/*
 * Network protocols
 *
 */

#undef	NET_PROTO_IPV4		/* IPv4 protocol */
#undef NET_PROTO_IPV6	/* IPv6 protocol */
#undef	NET_PROTO_FCOE		/* Fibre Channel over Ethernet protocol */
#undef	NET_PROTO_STP		/* Spanning Tree protocol */
#undef	NET_PROTO_LACP		/* Link Aggregation control protocol */
#undef	NET_PROTO_EAPOL		/* EAP over LAN protocol */
#undef NET_PROTO_LLDP	/* Link Layer Discovery protocol */

/*
 * PXE support
 *
 */
#undef	PXE_STACK		/* PXE stack in iPXE - you want this! */
#undef	PXE_MENU		/* PXE menu booting */

/*
 * Download protocols
 *
 */

#undef	DOWNLOAD_PROTO_TFTP	/* Trivial File Transfer Protocol */
#undef	DOWNLOAD_PROTO_HTTP	/* Hypertext Transfer Protocol */
#undef	DOWNLOAD_PROTO_HTTPS	/* Secure Hypertext Transfer Protocol */
#undef	DOWNLOAD_PROTO_FTP	/* File Transfer Protocol */
#undef	DOWNLOAD_PROTO_SLAM	/* Scalable Local Area Multicast */
#undef	DOWNLOAD_PROTO_NFS	/* Network File System Protocol */
#undef DOWNLOAD_PROTO_FILE	/* Local filesystem access */

/*
 * SAN boot protocols
 *
 */

#undef	SANBOOT_PROTO_ISCSI	/* iSCSI protocol */
#undef	SANBOOT_PROTO_AOE	/* AoE protocol */
#undef	SANBOOT_PROTO_IB_SRP	/* Infiniband SCSI RDMA protocol */
#undef	SANBOOT_PROTO_FCP	/* Fibre Channel protocol */
#undef	SANBOOT_PROTO_HTTP	/* HTTP SAN protocol */
#define SANBOOT_PROTO_NVME

/*
 * HTTP extensions
 *
 */
#undef HTTP_AUTH_BASIC		/* Basic authentication */
#undef HTTP_AUTH_DIGEST	/* Digest authentication */
#undef HTTP_AUTH_NTLM	/* NTLM authentication */
#undef HTTP_ENC_PEERDIST	/* PeerDist content encoding */
#undef HTTP_HACK_GCE		/* Google Compute Engine hacks */

/*
 * 802.11 cryptosystems and handshaking protocols
 *
 */
#undef	CRYPTO_80211_WEP	/* WEP encryption (deprecated and insecure!) */
#undef	CRYPTO_80211_WPA	/* WPA Personal, authenticating with passphrase */
#undef	CRYPTO_80211_WPA2	/* Add support for stronger WPA cryptography */

/*
 * Name resolution modules
 *
 */

#undef	DNS_RESOLVER		/* DNS resolver */

/*
 * Image types
 *
 * Etherboot supports various image formats.  Select whichever ones
 * you want to use.
 *
 */
#undef	IMAGE_NBI		/* NBI image support */
#undef	IMAGE_ELF		/* ELF image support */
#undef	IMAGE_MULTIBOOT		/* MultiBoot image support */
#define	IMAGE_PXE		/* PXE image support */
#undef	IMAGE_SCRIPT		/* iPXE script image support */
#undef	IMAGE_BZIMAGE		/* Linux bzImage image support */
#undef	IMAGE_COMBOOT		/* SYSLINUX COMBOOT image support */
#undef	IMAGE_EFI		/* EFI image support */
#undef	IMAGE_SDI		/* SDI image support */
#undef	IMAGE_PNM		/* PNM image support */
#undef	IMAGE_PNG		/* PNG image support */
#undef	IMAGE_DER		/* DER image support */
#undef	IMAGE_PEM		/* PEM image support */
#undef	IMAGE_ZLIB		/* ZLIB image support */
#undef	IMAGE_GZIP		/* GZIP image support */

/*
 * Command-line commands to include
 *
 */
#undef	AUTOBOOT_CMD		/* Automatic booting */
#undef	NVO_CMD			/* Non-volatile option storage commands */
#undef	CONFIG_CMD		/* Option configuration console */
#undef	IFMGMT_CMD		/* Interface management commands */
#undef	IWMGMT_CMD		/* Wireless interface management commands */
#undef IBMGMT_CMD		/* Infiniband management commands */
#undef FCMGMT_CMD		/* Fibre Channel management commands */
#undef	ROUTE_CMD		/* Routing table management commands */
#undef IMAGE_CMD		/* Image management commands */
#undef DHCP_CMD		/* DHCP management commands */
#undef SANBOOT_CMD		/* SAN boot commands */
#undef MENU_CMD		/* Menu commands */
#undef LOGIN_CMD		/* Login command */
#undef SYNC_CMD		/* Sync command */
#undef SHELL_CMD		/* Shell command */
#undef NSLOOKUP_CMD		/* DNS resolving command */
#undef TIME_CMD		/* Time commands */
#undef DIGEST_CMD		/* Image crypto digest commands */
#undef LOTEST_CMD		/* Loopback testing commands */
#undef VLAN_CMD		/* VLAN commands */
#undef PXE_CMD		/* PXE commands */
#undef REBOOT_CMD		/* Reboot command */
#undef POWEROFF_CMD		/* Power off command */
#undef IMAGE_TRUST_CMD	/* Image trust management commands */
#undef PCI_CMD		/* PCI commands */
#undef PARAM_CMD		/* Request parameter commands */
#undef NEIGHBOUR_CMD		/* Neighbour management commands */
#undef PING_CMD		/* Ping command */
#undef CONSOLE_CMD		/* Console command */
#undef IPSTAT_CMD		/* IP statistics commands */
#undef PROFSTAT_CMD		/* Profiling commands */
#undef NTP_CMD		/* NTP commands */
#undef CERT_CMD		/* Certificate management commands */
#undef IMAGE_MEM_CMD		/* Read memory command */
#undef IMAGE_ARCHIVE_CMD	/* Archive image management commands */
#undef SHIM_CMD		/* EFI shim command (or dummy command) */
#undef CPUID_CMD

#undef ENTROPY_RTC
#undef ENTROPY_RDRAND
#undef REBOOT_PCBIOS

#undef	PCI_SETTINGS	/* PCI device settings */
#undef	CPUID_SETTINGS	/* CPUID settings */
#undef	MEMMAP_SETTINGS	/* Memory map settings */
#undef	VMWARE_SETTINGS	/* VMware GuestInfo settings */
#undef	VRAM_SETTINGS	/* Video RAM dump settings */
#undef	ACPI_SETTINGS	/* ACPI settings */

/*
 * ROM-specific options
 *
 */
#undef	NONPNP_HOOK_INT19	/* Hook INT19 on non-PnP BIOSes */
#define	AUTOBOOT_ROM_FILTER	/* Autoboot only devices matching our ROM */

/*
 * Virtual network devices
 *
 */
#undef VNIC_IPOIB		/* Infiniband IPoIB virtual NICs */
#undef VNIC_XSIGO		/* Infiniband Xsigo virtual NICs */

/*
 * Error message tables to include
 *
 */
#undef	ERRMSG_80211		/* All 802.11 error descriptions (~3.3kb) */

/*
 * Obscure configuration options
 *
 * You probably don't need to touch these.
 *
 */

#undef	BUILD_SERIAL		/* Include an automatic build serial
				 * number.  Add "bs" to the list of
				 * make targets.  For example:
				 * "make bin/rtl8139.dsk bs" */
#undef	BUILD_ID		/* Include a custom build ID string,
				 * e.g "test-foo" */
#undef	NULL_TRAP		/* Attempt to catch NULL function calls */
#undef	GDBSERIAL		/* Remote GDB debugging over serial */
#undef	GDBUDP			/* Remote GDB debugging over UDP
				 * (both may be set) */
//#define EFI_DOWNGRADE_UX	/* Downgrade UEFI user experience */
#undef	TIVOLI_VMM_WORKAROUND	/* Work around the Tivoli VMM's garbling of SSE
				 * registers when iPXE traps to it due to
				 * privileged instructions */

#include <config/named.h>
#include NAMED_CONFIG(general.h)
#include <config/local/general.h>
#include LOCAL_NAMED_CONFIG(general.h)

#endif /* CONFIG_GENERAL_H */
