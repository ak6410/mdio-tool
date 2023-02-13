/*
mdio-tool allow for direct access to mdio registers in a network phy.

Routines are based on mii-tool: http://freecode.com/projects/mii-tool

mdio-tool comes with ABSOLUTELY NO WARRANTY; Use with care!

Copyright (C) 2013 Pieter Voorthuijsen

mdio-tool is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

mdio-tool is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with mdio-tool.  If not, see <http://www.gnu.org/licenses/>.
*/

/* https://github.com/PieVo/mdio-tool */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <assert.h>

#ifndef __GLIBC__
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#endif
#include "mii.h"

#define MAX_ETH		8		/* Maximum # of interfaces */

static int skfd = -1;		/* AF_INET socket for ioctl() calls. */
static struct ifreq ifr;

/*--------------------------------------------------------------------*/

static int mdio_read(int skfd, int location)
{
    struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
    mii->reg_num = location;
    if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
	return -1;
    }
    return mii->val_out;
}

static void mdio_write(int skfd, int location, int value)
{
    struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;
    mii->reg_num = location;
    mii->val_in = value;
    if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0) {
	fprintf(stderr, "SIOCSMIIREG on %s failed: %s\n", ifr.ifr_name,
		strerror(errno));
    }
}

static void usage(char *prog)
{
	printf("Usage:\n"
			"  %s r [page] device register\n"
			"  %s w [page] device register value\n"
			"     PAGE, REGISTER and VALUE are hexadecimals\n",
			prog, prog) ;
}

int main(int argc, char **argv)
{
	int page, addr, dev, val, op, use_page;
	struct mii_data *mii = (struct mii_data *)&ifr.ifr_data;

	page = 0 ;
	use_page = 0 ;
	if(argc < 2 || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")  ) {
		usage(argv[0]) ;
		return 0;
	}

	/* Open a basic socket. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		perror("socket");
		return -1;
	}

	if (NULL == argv[1] || NULL == argv[2] || NULL == argv[3]) {
		fprintf(stderr, "Error: missing argument(s)\n\n") ;
		usage(argv[0]) ;
		return 1 ;
	}

	op = argv[1][0] ;
	switch(op) {
		case 'R':
		case 'r':
			if (NULL == argv[4]) {
				/* Page 0 */
				page = 0 ;
				addr = (int)strtol(argv[3], NULL, 16);
			} else {
				page = (int)strtol(argv[3], NULL, 16);
				addr = (int)strtol(argv[4], NULL, 16);
				use_page = 1 ;
			}
			break ;
		case 'W':
		case 'w':
			if (NULL == argv[5]) {
				/* Page 0 */
				page = 0 ;
				addr = (int)strtol(argv[3], NULL, 16);
				val  = (int)strtol(argv[4], NULL, 16);
			} else {
				page = (int)strtol(argv[3], NULL, 16);
				addr = (int)strtol(argv[4], NULL, 16);
				val  = (int)strtol(argv[5], NULL, 16);
				use_page = 1 ;
			}
			break ;
		default:
			fprintf(stderr, "Error: bad operation '%c'\n\n", op) ;
			usage(argv[0]) ;
			return 1 ;
	}

	/* Get the vitals from the interface. */
	strncpy(ifr.ifr_name, argv[2], IFNAMSIZ);
	if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0) {
		if (errno != ENODEV)
		fprintf(stderr, "SIOCGMIIPHY on '%s' failed: %s\n",
			argv[2], strerror(errno));
		return -1;
	}
	if (use_page) {
		mdio_write(skfd, 31, page);
	}
	switch(op) {
		case 'R':
		case 'r':
			printf("0x%.4x\n", mdio_read(skfd, addr));
			break ;
		case 'W':
		case 'w':
			mdio_write(skfd, addr, val);
			break ;
	}
	if (use_page) {
		mdio_write(skfd, 31, 0);
	}
	close(skfd);
}
