/*
 * common/cmd_lsi_net.c
 *
 * (C) Copyright 2009
 * LSI, john.jacques@lsi.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef CONFIG_SPL_BUILD

#include <config.h>
#include <common.h>
#include <exports.h>
#include <command.h>

DECLARE_GLOBAL_DATA_PTR;

extern int dumprx;
extern int dumptx;

/*
  ======================================================================
  Local Stuff
*/

/*
  ----------------------------------------------------------------------
  do_net
*/
  
int
do_net(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if( 0 == strncmp( argv[1], "dr", strlen( "dr" ) ) ) {
		dumprx ^= 1;

		return 0;
	}

	if( 0 == strncmp( argv[1], "dt", strlen( "dt" ) ) ) {
		dumptx ^= 1;

		return 0;
	}

	if( 0 == strncmp( argv[1], "l", strlen( "l" ) ) ) {
		printf( "Starting Loopback Test -- Ctrl-C to Exit.\n" );
		lsi_net_loopback_test(eth_get_dev());

		return 0;
	}

	if( 0 == strncmp( argv[1], "r", strlen( "r" ) ) ) {
		printf( "Receiving Packets -- Ctrl-C to Exit.\n" );
		lsi_net_receive_test(eth_get_dev());

		return 0;
	}

	if( 0 == strncmp( argv[1], "s", strlen( "s" ) ) ) {
		bd_t * bd = gd->bd;
		char * buffer = malloc( 60 );

		if( ( char * ) 0 == buffer ) {
			printf( "malloc() failed!\n" );
			printf( "%s", cmdtp->usage);

			return -1;
		} else {
			printf( "Sending Packet...\n" );
			eth_init( bd );
			eth_send( ( void * ) buffer, 60 );
			eth_halt( );
			free( buffer );

			return 0;
		}
	}

	printf( "%s", cmdtp->usage );

	return -1;
}

/*
  ======================================================================
  Command Definitions
  ======================================================================
*/

U_BOOT_CMD(net, 2, 0, do_net,
	   "net loopback|receive|send|dr|dt\n",
	   "l,oopback loop all received packets back\n"
	   "r,ecevie packets, and dump them to the screen\n"
	   "s,end send one packet\n"
	   "dr toggle the \"dumprx\" flag\n"
	   "dt toggle the \"dumptx\" flag\n");

#endif /* CONFIG_SPL_BUILD */