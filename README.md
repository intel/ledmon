# This package contains the Enclosure LED Utilities, version 0.95

Copyright (C) 2009-2021 Intel Corporation.

All files in this package can be freely distributed and used according
to the terms of the GNU General Public License, version 2.
See http://www.gnu.org/ for details.

-------------------------

## 1. Dependencies

-------------------------

Following packages are required to compile:

|RHEL|SLES|Debian/Ubuntu|
|:---:|:---:|:---:|
| `RHEL8: pkgconf`, `RHEL7: pkgconfig`  | `pkg-config` | `pkg-config` |
| `automake` | `automake`   | `automake`   |
| `autoconf` | `autoconf`   | `autoconf`   |
| `gcc` | `gcc` | `gcc` |
| `make` | `make` | `make` |
| `sg3_utils-devel`| `libsgutils-devel`  | `libsgutils2-dev` |
| `systemd-devel`  | `libudev-devel`     | `libudev-dev`     |
| `pciutils-devel` | `pciutils-devel`    | `libpci-dev`      |

## 2. Configure package

-------------------------

Run `autogen.sh` to generate compiling configurations:  
   `./autogen.sh`  
   `./configure`

Run ./configure with:  
    `--enable-systemd` to configure with systemd service.  

## 3. Compiling the package

-------------------------

Run `make` command to compile the package.

## 4. (Un)installing the package

-------------------------

Run following commands to install package:  
   `make install`

Run following commands to uninstall package:  
   `make uninstall`

## 5. Release notes

-------------------------

a. Enclosure LED Utilities is meant as a part of RHEL, SLES and Debian/Ubuntu linux
   distributions.

b. For backplane enclosures attached to ISCI controller support is limited to
   Intel(R) Intelligent Backplane.
