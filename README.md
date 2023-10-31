# NVMe INT13h Option ROM
Boot legacy PCs from NVMe storage:

![T43 showing the NVME BIOS init message](https://screenshot.tbspace.de/bsqzafghutv.jpg)
![T43 Boot Menu showing a NVMe option](https://screenshot.tbspace.de/rdmgtabvhpl.jpg)

This project allows old x86 computers using a classic BIOS to boot from modern NVMe storage attached via PCI(e).
It's a heavily modified version of [iPXE](https://ipxe.org/start) (which usually allows for booting from the network), but 
instead of the network, this code uses a port of the [SeaBIOS](https://github.com/coreboot/seabios/tree/master) [NVMe implementation](https://github.com/coreboot/seabios/blob/master/src/hw/nvme.c) to talk to a local NVMe drive.

## Project status
Works™   
Very little fault tolerance and testing on different devices.

#### Supported hosts
- IBM ThinkPad T43
- QEMU VM, i440fx

#### Supported SSDs
- Samsung SSD 980 
- Corsair Force MP510

## Features
- Booting from NVMe storage in Legacy/BIOS mode
- Works on 32bit-only CPUs (Pentium 3, Atom, etc.)
- Option ROM, USB flash drive, SD card, CD-ROM, chainloading (e.g. from grub) booting supported

## Limitations
- Write support is currently disabled. The code is present, but it's additional danger for no benefit (for my usecase)
- Only 1 NVMe drive (with 1 namespace) is properly handled at the moment
- 64bit BARs are probably not handled correctly (untested!)

## Similar projects / Alternatives
There are 2 other Option ROMs publicly available:
- Samsung 950 Pro (exposed by the PCIe device, device-specific)
- community modded VMWare NVMe ROM (didn't work on my hardware, did work in QEMU)

This project is GPL-licensed, can be freely redistributed (both as binaries and source) and can be instrumented/debugged easily (as it's written in C).
