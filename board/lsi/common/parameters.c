/*
 * board/lsi/common/parameters.c
 *
 * Inteface to access the parameter files.
 *
 * Copyright (C) 2009-2013 LSI Corporation
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <config.h>
#include <common.h>

/*
  ==============================================================================
  ==============================================================================
  Private
  ==============================================================================
  ==============================================================================
*/

#if defined(CONFIG_AXXIA_PPC)
/*
  For PPC (34xx, 25xx, 35xx), use version 4 of the parameters.
*/
#define PSWAB(value) (value)
#define PARAMETERS_HEADER_ADDRESS \
	(LCM + (128 * 1024) - sizeof(parameters_header_t))
#define PARAMETERS_SIZE (1024)
#define PARAMETERS_OFFSET ((128 * 1024) - PARAMETERS_SIZE)
#define PARAMETERS_ADDRESS (LCM + PARAMETERS_OFFSET)
#define PARAMETERS_OFFSET_IN_FLASH PARAMETERS_OFFSET
#elif defined(CONFIG_AXXIA_ARM)
/*
  For ARM (55xx), use version 5 of the parameters.
*/
#define PSWAB(value) ntohl(value)
#define PARAMETERS_HEADER_ADDRESS \
	(LSM + (256 * 1024) - sizeof(parameters_header_t))
#define PARAMETERS_SIZE (4096)
#define PARAMETERS_OFFSET ((256 * 1024) - PARAMETERS_SIZE)
#define PARAMETERS_ADDRESS (LSM + PARAMETERS_OFFSET)
#define PARAMETERS_OFFSET_IN_FLASH 0x40000
#else
#error "Unknown Architecture!"
#endif

parameters_header_t *header = (parameters_header_t *)PARAMETERS_HEADER_ADDRESS;
parameters_global_t *global = (parameters_global_t *)1;
parameters_pciesrio_t *pciesrio = (parameters_pciesrio_t *)1;
parameters_voltage_t *voltage = (parameters_voltage_t *)1;
parameters_clocks_t *clocks = (parameters_clocks_t *)1;
parameters_sysmem_t *sysmem = (parameters_sysmem_t *)1;
#ifdef CONFIG_AXXIA_ARM
void *retention = (void *)1;
#endif

/*
  ===============================================================================
  ===============================================================================
  Public
  ===============================================================================
  ===============================================================================
*/

/*
  -------------------------------------------------------------------------------
  read_parameters
*/

int
read_parameters(void)
{
	int rc;
#ifdef CONFIG_AXXIA_ARM
	int i;
	unsigned long *buffer;
#endif

	/*
	  Try LSM first, to allow for board repair when the serial
	  EEPROM contains a valid but incorrect (unusable) parameter
	  table.
	*/

	/* Verify that the paramater table is valid. */
	if (PARAMETERS_MAGIC != PSWAB(header->magic)) {
		/* Initialize the SEEPROM (device 0, read only). */
		ssp_init(0, 1);

		/* Copy the parameters from SPI device 0. */
		rc = ssp_read((void *)PARAMETERS_ADDRESS,
				      PARAMETERS_OFFSET_IN_FLASH,
				      PARAMETERS_SIZE);

		if (0 != rc || PARAMETERS_MAGIC != PSWAB(header->magic))
			/* No parameters available, fail. */
			return -1;
	}

	if (crc32(0, (void *)PARAMETERS_ADDRESS, (PSWAB(header->size) - 12)) !=
	    PSWAB(header->checksum) ) {
		printf("Parameter table is corrupt. 0x%08x!=0x%08x\n",
		       PSWAB(header->checksum),
		       crc32(0, (void *)PARAMETERS_ADDRESS,
			     (PSWAB(header->size) - 12)));
		return -1;
	}

#ifdef CONFIG_AXXIA_ARM
	buffer = (unsigned long *)PARAMETERS_ADDRESS;

	for (i = 0; i < (PARAMETERS_SIZE / 4); ++i) {
		*buffer = ntohl(*buffer);
		++buffer;
	}
#endif

#ifdef DISPLAY_PARAMETERS
	printf("-- -- Header\n"
	       "0x%08lx 0x%08lx 0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n",
	       header->magic,
	       header->size,
	       header->checksum,
	       header->version,
	       header->globalOffset,
	       header->globalSize,
	       header->pciesrioOffset,
	       header->pciesrioSize,
	       header->voltageOffset,
	       header->voltageSize,
	       header->clocksOffset,
	       header->clocksSize,
	       header->sysmemOffset,
	       header->sysmemSize);
#endif

	global = (parameters_global_t *)
		(PARAMETERS_ADDRESS + header->globalOffset);
	pciesrio = (parameters_pciesrio_t *)
		(PARAMETERS_ADDRESS + header->pciesrioOffset);
	voltage = (parameters_voltage_t *)
		(PARAMETERS_ADDRESS + header->voltageOffset);
	clocks = (parameters_clocks_t *)
		(PARAMETERS_ADDRESS + header->clocksOffset);
	sysmem = (parameters_sysmem_t *)
		(PARAMETERS_ADDRESS + header->sysmemOffset);
#ifdef CONFIG_AXXIA_ARM
	retention = (void *)(PARAMETERS_ADDRESS + header->retentionOffset);
#endif

#ifdef DISPLAY_PARAMETERS
	printf("version=%lu flags=0x%lx\n", global->version, global->flags);
#else
	/*printf("Parameter Table Version %lu\n", global->version);*/
#endif

	return 0;
}

#ifdef CONFIG_MEMORY_RETENTION

/*
  -------------------------------------------------------------------------------
  write_parameters
*/

int
write_parameters(parameters_t *parameters)
{
	return 0;
}

#endif	/* CONFIG_MEMORY_RETENTION */