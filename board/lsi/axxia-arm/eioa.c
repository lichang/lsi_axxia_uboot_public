/*
 *  Copyright (C) 2011 LSI Corporation
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

/*
  Notes:

  - Virtual registers (0.769.0.OFFSET) are described in
    rte/api/nca/include/ncp_dev_config_regs.h.
*/

/* #define DEBUG */
/*#define LSI_LOGIO*/
#include <config.h>
#include <common.h>
#include <malloc.h>
#include <asm/io.h>

/*#define TRACE_ALLOCATION*/

#define NCP_TASKIO_LITE
#define NCP_TASKIO_UBOOT_ENV
#define NCP_TASKIO_LITE_UBOOT_ENV_DEFINE_GLOBALS

#define NCP_USE_ALL_PORTS 0xff

#include "ncp_task_basetypes.h"
#include "ncp_task.h"

DECLARE_GLOBAL_DATA_PTR;

/*
  ==============================================================================
  ==============================================================================
  Private Interface
  ==============================================================================
  ==============================================================================
*/

static int initialized = 0;
/* dummy NCP handle. Needed for task APIs. */
ncp_t gNCP = { 0 };
static ncp_task_hdl_t taskHdl = NULL;

/* these needs to match with generated C code from ASE */
static ncp_uint8_t threadQueueSetId = 0;
static ncp_uint8_t ncaQueueId = 0;
static ncp_uint8_t recvQueueId = 0;
static ncp_uint8_t sendVpId = 0;

static int eioaPort = NCP_USE_ALL_PORTS;
static int loopback = 0;
static int rxtest = 0;

extern int dumprx;
extern int dumptx;

#define NCP_EIOA_GEN_CFG_REG_OFFSET(portIndex)                                  \
    0x100000 +                                                                  \
    ((portIndex > 0) ? 0x10000 : 0) +                                           \
    ((portIndex > 0) ? (0x1000 * (portIndex - 1)) : 0)

static int port_by_index[] = 
    {  0,   1,  2,  3,  4,  /* eioa0 */
      16,  17, 18, 19, 20,  /* eioa1 */
      32,  33,              /* eioa2 */
      48,  49,              /* eioa3 */
      64,  65,              /* eioa4 */
      80,  81,              /* eioa5 */
      96,  97,              /* eioa6 */
      112, 113 };           /* eioa7 */

static int index_by_port[] = 
    { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa0 */
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa1 */
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa2 */
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa3 */
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa4 */
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa5 */
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa6 */
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }; /* eioa7 */

static int index_by_port_gmac[] = 
    {  0,  1,  2,  3,  4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa0 */
       5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa1 */
      10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa2 */
      12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa3 */
      14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa4 */
      16, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa5 */
      18, 19, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa6 */
      20, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }; /* eioa7 */

static int index_by_port_xgmac[] = 
    {  0,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa0 */
       5,  6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa1 */
      10, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa2 */
      12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa3 */
      14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa4 */
      16, 17, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa5 */
      18, 19, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,   /* eioa6 */
      20, 21, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }; /* eioa7 */

#define EIOA_NUM_PORT_INDEXES (sizeof(index_by_port)/sizeof(int))

typedef enum 
{
    EIOA_PORT_TYPE_XGMAC,
    EIOA_PORT_TYPE_GMAC
} eioa_port_type;
/* 
 * port type: 1 - gmac, 0 - xgmac
 * By default all are set to gmac.
 */
static int port_type_by_index[] = 
{ 
    EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, 
    EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC,
    EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, 
    EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC,
    EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, 
    EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC,
    EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, EIOA_PORT_TYPE_GMAC, 
    EIOA_PORT_TYPE_GMAC 
};


/* TODO: fix the phy addresses for asic */
static int phy_by_index[] = 
{ 
    0x1c, 0x17, 0x16, 0x15, 0x14, 0x18, 0x13, 0x12, 0x11, 0x10, 0x20, 0x21, 
    0x30, 0x31, 0x40, 0x41, 0x50, 0x51, 0x60, 0x61, 0x70, 0x71 
};

typedef enum
{
    EIOA_PHY_MEDIA_FIBER,
    EIOA_PHY_MEDIA_COPPER
} eioa_phy_media;

/* 
 * phy type: 1 - copper, 0 - fiber 
 * By default all are set to copper.
 */
static int phy_media_by_index[] = 
{ 
    EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, 
    EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER,
    EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, 
    EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER,
    EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, 
    EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER,
    EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, EIOA_PHY_MEDIA_COPPER, 
    EIOA_PHY_MEDIA_COPPER, 
};

#define EIOA_NUM_PORTS (sizeof(port_by_index)/sizeof(int))

#undef TRACE
#define TRACE
#ifdef TRACE
#define NCR_CALL(x) { \
if (0 != (rc = (x))) { \
printf("%s:%s:%d - FAILED\n", __FILE__, __FUNCTION__, __LINE__); \
goto ncp_return; \
} \
}
#else
#define NCR_CALL(x) { \
if (0 != (rc = (x))) { \
goto ncp_return; \
} \
}
#endif

typedef struct {
	unsigned long region;
	unsigned long offset;
} ncr_location_t;

typedef enum {
	NCR_COMMAND_NULL,
	NCR_COMMAND_WRITE,
	NCR_COMMAND_READ,
	NCR_COMMAND_MODIFY,
	NCR_COMMAND_USLEEP,
	NCR_COMMAND_POLL,
	NCR_COMMAND_FUNC
} ncr_command_code_t;

typedef struct {
	ncr_command_code_t command;
	unsigned long region;
	unsigned long offset;
	unsigned long value;
	unsigned long mask;
} ncr_command_t;

typedef int (*func_config_port)(int index);

static int
ncp_dev_configure(ncr_command_t *commands);

void
axxia_dump_packet(const char *header, void *packet, int length);

ncp_st_t
ncp_task_uboot_config(void);

void
ncp_task_uboot_unconfig(void);

#if defined(CONFIG_AXXIA_55XX) || defined(CONFIG_AXXIA_55XX_EMU)
#include "EIOA55xx/mme.c"
#include "EIOA55xx/pbm.c"
#include "EIOA55xx/vp.c"
#include "EIOA55xx/nca.c"
#include "EIOA55xx/eioa.c"
#else
#error "EIOA is not defined for this architecture!"
#endif

ncp_st_t
ncp_task_lite_uboot_unconfig();

/*
  ------------------------------------------------------------------------------
  ncp_dev_reset
*/

static int
ncp_dev_reset(void)
{
	unsigned long value;

	/*
	  Reset Modules
	*/

    /* TODO: Get the reset sequence here */

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_do_read
*/

static int
ncp_dev_do_read(ncr_command_t *command, unsigned long *value)
{
	if (NCP_REGION_ID(512, 1) == command->region) {
		*value = *((volatile unsigned *)command->offset);

		return 0;
	}

	if (0x100 <= NCP_NODE_ID(command->region)) {
		static int last_node = -1;

		if (-1 == last_node ||
		    NCP_NODE_ID(command->region) != last_node) {
			last_node = NCP_NODE_ID(command->region);
			debug("READ IGNORED: n=0x%lx t=0x%lx o=0x%lx\n",
				    NCP_NODE_ID(command->region),
				    NCP_TARGET_ID(command->region),
				    command->offset);
		}

		return 0;
	}

	if (0 != ncr_read32(command->region, command->offset, value)) {
		printf("READ ERROR: n=0x%lx t=0x%lx o=0x%lx\n",
			    NCP_NODE_ID(command->region),
			    NCP_TARGET_ID(command->region), command->offset);
		return -1;
	}

	debug("Read 0x%08lx from n=0x%lx t=0x%lx o=0x%lx\n",
		    *value, NCP_NODE_ID(command->region),
		    NCP_TARGET_ID(command->region),
		    command->offset);

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_do_modify
*/

static int
ncp_dev_do_modify(ncr_command_t *command)
{
	if (0x100 <= NCP_NODE_ID(command->region)) {
		static int last_node = -1;

		if (-1 == last_node ||
		    NCP_NODE_ID(command->region) != last_node) {
			last_node = NCP_NODE_ID(command->region);
			debug("MODIFY IGNORED: n=0x%lx t=0x%lx o=0x%lx\n",
				    NCP_NODE_ID(command->region),
				    NCP_TARGET_ID(command->region),
				    command->offset);
		}

		return 0;
	}

	if (0 != ncr_modify32(command->region, command->offset,
			      command->mask, command->value)) {
		printf("MODIFY ERROR: n=0x%lx t=0x%lx o=0x%lx m=0x%lx "
			    "v=0x%lx\n",
			    NCP_NODE_ID(command->region),
			    NCP_TARGET_ID(command->region), command->offset,
			    command->mask, command->value);

		return -1;
	} else {

		debug("MODIFY: r=0x%lx o=0x%lx m=0x%lx v=0x%lx\n",
			    command->region, command->offset,
			    command->mask, command->value);

	}

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_do_write
*/

static int
ncp_dev_do_write(ncr_command_t *command)
{
	debug(" WRITE: r=0x%lx o=0x%lx v=0x%lx\n",
		    command->region, command->offset, command->value);

	if (NCP_REGION_ID(0x200, 1) == command->region) {
		*((volatile unsigned *)command->offset) = command->value;
#ifdef USE_CACHE_SYNC
		flush_cache(command->offset, 4);
#endif
#if 0 /* TODO: find the equivalent for 55xx */
	} else if (NCP_REGION_ID(0x148, 0) == command->region) {
		out_le32((volatile unsigned *)(APB2RC + command->offset),
			 command->value);
		flush_cache((APB2RC + command->offset), 4);
#endif
	} else if (NCP_NODE_ID(command->region) <= 0x101) {
		if (NCP_REGION_ID(0x17, 0x11) == command->region &&
		    0x11c == command->offset) {
			return 0;
		}
#if 0
        /* HACK: to avoid errors in half switch systems, skip eioa2 and eioa3 */
        else if(NCP_NODE_ID(command->region) == 0x29 ||
                NCP_NODE_ID(command->region) == 0x28) {
                   return 0;
        }
#endif
		if (0 != ncr_write32(command->region, command->offset,
				     command->value)) {
			printf("WRITE ERROR: n=0x%lx t=0x%lx o=0x%lx "
				    "v=0x%lx\n",
				    NCP_NODE_ID(command->region),
				    NCP_TARGET_ID(command->region),
				    command->offset, command->value);

			return -1;
		}
	} else {
		static int last_node = -1;

		if (-1 == last_node ||
		    NCP_NODE_ID(command->region) != last_node) {
			last_node = NCP_NODE_ID(command->region);
			debug("WRITE IGNORED: n=0x%lx t=0x%lx o=0x%lx "
				    "v=0x%lx\n",
				    NCP_NODE_ID(command->region),
				    NCP_TARGET_ID(command->region),
				    command->offset,
				    command->value);
		}

		return 0;
	}

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_do_poll
*/

static int
ncp_dev_do_poll(ncr_command_t *command)
{
	int timeout = 1000;
	int delay = 1000;
	unsigned long value;

	do {
		udelay(delay);

		if (0 != ncp_dev_do_read(command, &value)) {
			printf("ncp_dev_do_read() failed!\n");
			return -1;
		}
	} while (((value & command->mask) != command->value) &&
		 0 < --timeout);

	if (0 == timeout) {
		printf("ncp_dev_do_poll() timed out!\n");
		return -1;
	}

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_configure
*/

static int
ncp_dev_configure(ncr_command_t *commands) {
	int rc = 0;
	unsigned long value;
    ncr_command_t *startCmd = commands;

	while (NCR_COMMAND_NULL != commands->command) {
		switch (commands->command) {
		case NCR_COMMAND_WRITE:
			rc = ncp_dev_do_write(commands);
			break;
		case NCR_COMMAND_READ:
			rc = ncp_dev_do_read(commands, &value);
			break;
		case NCR_COMMAND_MODIFY:
			rc = ncp_dev_do_modify(commands);
			break;
		case NCR_COMMAND_USLEEP:
			debug("USLEEP: v=0x%lx\n", commands->value);
			udelay(commands->value);
			break;
		case NCR_COMMAND_POLL:
			rc = ncp_dev_do_poll(commands);
			break;
        case NCR_COMMAND_FUNC:
            if(commands->offset) {
                func_config_port func_ptr = (func_config_port)commands->offset;
                /* call the function in offset field with param in value field */
                rc = func_ptr(commands->value);
            }
            break;
		default:
			printf("Unknown Command: 0x%x, startCmd=0x%x, curCmd=0x%x, entry#=%d\n",
				    commands->command, startCmd, commands,
				    ((unsigned long)commands - (unsigned long)startCmd)/sizeof(ncr_command_t));
			rc = -1;
			break;
		}

		if (ctrlc()) {
			printf("Canceling configuration.\n");
			break;
		}

		++commands;
	}

	return rc;
}

/*
  ------------------------------------------------------------------------------
  task_send
*/

static int
task_send(ncp_task_ncaV2_send_meta_t *taskMetaData)
{
	ncp_st_t ncpStatus;

	do {
#ifdef USE_CACHE_SYNC
		flush_cache((unsigned long)taskMetaData->pduSegAddr0,
			    taskMetaData->pduSegSize0);
#endif

        if(dumptx) {
            axxia_dump_packet("LSI_EIOA TX", (void *)taskMetaData->pduSegAddr0, 
                    taskMetaData->pduSegSize0);
        }
        
		ncpStatus = ncp_task_ncav2_send(taskHdl, NULL, taskMetaData, TRUE,
            NULL, NULL);
        debug("task_send(): after send. status=%d\n", ncpStatus);
        
	} while (NCP_ST_TASK_SEND_QUEUE_FULL == ncpStatus);

	if (NCP_ST_SUCCESS != ncpStatus) {
		return 0;
    }

	return taskMetaData->pduSegSize0;
}

#define DELAY() udelay(5000)

/*
  ------------------------------------------------------------------------------
  line_setup
*/

static int
line_setup(int index)
{
	int rc;
	int retries = 100000;
	unsigned long eioaRegion;
	unsigned long gmacRegion;
	unsigned long gmacPortOffset;
    unsigned long hwPortIndex;
	unsigned long ncr_status;
	char *envstring;
	unsigned short status;
	unsigned long top;
	unsigned long bottom;
	unsigned short ad_value;
	unsigned short ge_ad_value;
	unsigned short control;

    if(index >= 128 || (index < 128 && index_by_port[port_by_index[index]] == -1))
    {
        printf("Invalid gmac port %d (index=%d)\n", port_by_index[index],
            index);
	    return -1;
    }

    debug("line_setup for port=%d\n", port_by_index[index]);

	/* Set the region and offset. */
    if (5 > index) {
        hwPortIndex = index;
		eioaRegion = NCP_REGION_ID(31, 16); /* 0x1f.0x10 */
		gmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(31, 17) : /* 0x1f.0x11 */ 
                                           NCP_REGION_ID(31, 18)); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * hwPortIndex;
	} else if (10 > index) {
	    hwPortIndex = index - 5;
		eioaRegion = NCP_REGION_ID(23, 16); /* 0x17.0x10 */
        gmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(23, 17) : /* 0x1f.0x11 */ 
                                           NCP_REGION_ID(23, 18)); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * hwPortIndex;
	} else if (12 > index) {
	    hwPortIndex = index - 10;
		eioaRegion = NCP_REGION_ID(40, 16); /* 0x28.0x10 */
        gmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(40, 17) : /* 0x1f.0x11 */ 
                                           NCP_REGION_ID(40, 18)); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * hwPortIndex;
	} else if (14 > index) {
	    hwPortIndex = index - 12;
		eioaRegion = NCP_REGION_ID(41, 16); /* 0x29.0x10 */
        gmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(41, 17) : /* 0x1f.0x11 */ 
                                           NCP_REGION_ID(41, 18)); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * hwPortIndex;
	} else if (16 > index) {
	    hwPortIndex = index - 14;
		eioaRegion = NCP_REGION_ID(42, 16); /* 0x2a.0x10 */
        gmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(42, 17) : /* 0x1f.0x11 */ 
                                           NCP_REGION_ID(42, 18)); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * hwPortIndex;
	} else if (18 > index) {
	    hwPortIndex = index - 16;
		eioaRegion = NCP_REGION_ID(43, 16); /* 0x2b.0x10 */
        gmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(43, 17) : /* 0x1f.0x11 */ 
                                           NCP_REGION_ID(43, 18)); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * hwPortIndex;
	} else if (20 > index) {
	    hwPortIndex = index - 18;
		eioaRegion = NCP_REGION_ID(44, 16); /* 0x2c.0x10 */
        gmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(44, 17) : /* 0x1f.0x11 */ 
                                           NCP_REGION_ID(44, 18)); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * hwPortIndex;
	} else if (22 > index) {
	    hwPortIndex = index - 20;
		eioaRegion = NCP_REGION_ID(45, 16); /* 0x2d.0x10 */
        gmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(45, 17) : /* 0x1f.0x11 */ 
                                           NCP_REGION_ID(45, 18)); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * hwPortIndex;
	} else {
	    printf("Invalid gmac port %d\n", port_by_index[index]);
	    return -1;
    }

	/* Disable stuff. */
	NCR_CALL(ncr_modify32(gmacRegion, 0x330 + gmacPortOffset, 0x3f, 0));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x8, 0x0));
	NCR_CALL(ncr_modify32(eioaRegion, 
                NCP_EIOA_GEN_CFG_REG_OFFSET(hwPortIndex) + 0x0, 
                0x00000003, 0x0));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x4, 0x0));

	/* Check for "macspeed".  If set, ignore the PHYs... */
	envstring = getenv("macspeed");

	if (NULL != envstring) {
		printf("Setting gmac%d to %s\n", port_by_index[index], envstring);

		NCR_CALL(ncr_read32(gmacRegion, 0x324 + gmacPortOffset,
				    &ncr_status));
		ncr_status &= ~0x3c;
		ncr_status |= 0x08; /* Force Link Up */

		if (0 == strcmp("10MH", envstring)) {
		} else if (0 == strcmp("10MF", envstring)) {
			ncr_status |= 0x04;
		} else if (0 == strcmp("100MH", envstring)) {
			ncr_status |= 0x10;
		} else if (0 == strcmp("100MF", envstring)) {
			ncr_status |= 0x14;
		} else if (0 == strcmp("1G", envstring)) {
			ncr_status |= 0x24;
		} else {
			printf("macspeed must be set to 10MH, 10MF, 100MH, "
			       "100MF, or 1G\n");
			return -1;
		}

		NCR_CALL(ncr_write32(gmacRegion, 0x324 + gmacPortOffset,
				     ncr_status));
	} else {
#if defined(CONFIG_AXXIA_55XX)
		/* Get ad_value and ge_ad_value from the environment. */
		envstring = getenv("ad_value");

		if (NULL == envstring) {
			ad_value = 0x1e1;
		} else {
			ad_value = simple_strtoul(envstring, NULL, 0);
		}

		envstring = getenv("ge_ad_value");

		if (NULL == envstring) {
			ge_ad_value = 0x300;
		} else {
			ge_ad_value = simple_strtoul(envstring, NULL, 0);
		}

		/* Set the AN advertise values. */
		mdio_write(phy_by_index[index], 4, ad_value);
		mdio_write(phy_by_index[index], 9, ge_ad_value);

		/* Force re-negotiation. */
		control = mdio_read(phy_by_index[index], 0);
		control |= 0x200;
		mdio_write(phy_by_index[index], 0, control);

		DELAY();

		/* Wait for AN complete. */
		for (;;) {
			status = mdio_read(phy_by_index[index], 1);

			if (0 != (status & 0x20))
				break;

			if (0 == retries--) {
				printf("GMAC%d: AN Timed Out.\n",
					    port_by_index[index]);
				return -1;
			}

			DELAY();
		}

		if (0 == (status & 0x4)) {
			printf("GMAC%d: LINK is Down.\n",
				    port_by_index[index]);

			if (NCP_USE_ALL_PORTS != eioaPort)
				return -1; /* Don't Error Out in AUTO Mode. */
		} else {
			status = mdio_read(phy_by_index[index], 0x1c);
        }
#else
		status = 0x28; /* For FPGA, its 100MF */
#endif
		printf("GMAC%02d: ", port_by_index[index]);

		switch ((status & 0x18) >> 3) {
		case 0:
			puts("10M");
			break;
		case 1:
			puts("100M");
			break;
		case 2:
			puts("1G");
			break;
		default:
			puts("UNKNOWN");
			break;
		}

		printf(" %s\n",
		       (0 == (status & 0x20)) ?
		       "Half Duplex" : "Full Duplex");
		DELAY();

		/* Make the MAC match. */

		NCR_CALL(ncr_read32(gmacRegion, 0x324 + gmacPortOffset,
				    &ncr_status));
		ncr_status &= ~0x3c;
		ncr_status |= 0x08;	/* Force Link Up */

		if (0 != (status & 0x20))
			ncr_status |= 0x04; /* Force Full Duplex */

		/* Set the Speed */
		ncr_status |= (((status & 0x18) >> 3) << 4);

		NCR_CALL(ncr_write32(gmacRegion, 0x324 + gmacPortOffset,
				     ncr_status));
	}

	/*
	  Set the Ethernet addresses...
	*/

	top = (ethernet_address[0] << 8) | ethernet_address[1];
	bottom = (ethernet_address[2] << 24) | (ethernet_address[3] << 16) |
	  (ethernet_address[4] << 8) | (ethernet_address[5]);

	/* - EIOA */
	NCR_CALL(ncr_write32(eioaRegion, NCP_EIOA_GEN_CFG_REG_OFFSET(hwPortIndex) + 0x4, bottom));
	NCR_CALL(ncr_write32(eioaRegion, NCP_EIOA_GEN_CFG_REG_OFFSET(hwPortIndex) + 0x8, top));

	/* - Source */
	NCR_CALL(ncr_write32(gmacRegion, 0x304 + gmacPortOffset, bottom));
	NCR_CALL(ncr_write32(gmacRegion, 0x308 + gmacPortOffset, top));

	/* - Unicast */
	NCR_CALL(ncr_write32(gmacRegion, 0x344 + gmacPortOffset, bottom));
	NCR_CALL(ncr_write32(gmacRegion, 0x348 + gmacPortOffset, bottom));
	NCR_CALL(ncr_write32(gmacRegion, 0x34c + gmacPortOffset,
			     ((top & 0xffff) << 16) | (top & 0xffff)));
	NCR_CALL(ncr_write32(gmacRegion, 0x350 + gmacPortOffset, bottom));
	NCR_CALL(ncr_write32(gmacRegion, 0x354 + gmacPortOffset, top));

	/* Enable stuff. */
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x8, 0x8));
    NCR_CALL(ncr_modify32(eioaRegion, 
                NCP_EIOA_GEN_CFG_REG_OFFSET(hwPortIndex) + 0x0, 
                0x00000003, 0x3));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x4, 0x4));

	/* Unicast Filtering based on the Ethernet Address set above. */
	if (0 == rxtest && 0 == loopback)
		NCR_CALL(ncr_modify32(gmacRegion, 0x330 + gmacPortOffset,
				      0x3f, 0x09));

	return 0;

 ncp_return:

	return -1;
}

static int *memset_int(int *s, int c, size_t count)
{
    int indx = 0;
    for(indx = 0; indx < count; indx++) {
        s[indx] = c;
    }

    return s;
}

/*
  -------------------------------------------------------------------------------
  initialize_task_io
*/

static int
initialize_task_io(struct eth_device *dev)
{
	ncp_st_t ncpStatus = NCP_ST_SUCCESS;
	char *eioaport = NULL;
	int i = 0;
    char *phy_media_str = getenv("phymedia");
    eioa_phy_media phy_media = EIOA_PHY_MEDIA_FIBER;

    /* set phy media type */
    if(phy_media_str) {
        if(strcmp(phy_media_str, "copper") == 0) {
            phy_media = EIOA_PHY_MEDIA_COPPER;
        } else if(strcmp(phy_media_str, "fiber") == 0) {
            phy_media = EIOA_PHY_MEDIA_FIBER;
        } else {
            printf("Invalid PHY media: %s\n", phy_media_str);
            return -1;
        }
    }
    /* set all ports to same media type */
    memset_int(phy_media_by_index, phy_media, EIOA_NUM_PORTS);

    /* Set the port. */
	eioaport = getenv("eioaport");

    debug("eioaport=%s\n", eioaport);

    if (NULL != eioaport) {
        if(strcmp(eioaport, "auto-gmac") == 0) {
            /* set same port type for all ports */
            memset_int(port_type_by_index, EIOA_PORT_TYPE_GMAC, EIOA_NUM_PORTS);
            memcpy(index_by_port, index_by_port_gmac, sizeof(index_by_port));
            eioaPort = NCP_USE_ALL_PORTS;
		} else if(strncmp(eioaport, "gmac", 4) == 0) {
            eioaPort = simple_strtoul(&eioaport[4], NULL, 10);
            if(eioaPort < EIOA_NUM_PORT_INDEXES && index_by_port_gmac[eioaPort] != -1) {
                index_by_port[eioaPort] = index_by_port_gmac[eioaPort];
                /* set same port type for all ports */
                memset_int(port_type_by_index, EIOA_PORT_TYPE_GMAC, 
                    EIOA_NUM_PORTS);
            } else {
                printf("Invalid gmac port %d\n", eioaPort);
			    return -1;
            }
        } else if (0 == strcmp(eioaport, "auto-xgmac")) {
            /* set same port type for all ports */
            memset_int(port_type_by_index, EIOA_PORT_TYPE_XGMAC, 
                    EIOA_NUM_PORTS);
            memcpy(index_by_port, index_by_port_xgmac, sizeof(index_by_port));
            eioaPort = NCP_USE_ALL_PORTS;
		} else if(strncmp(eioaport, "xgmac", 5) == 0) {
            eioaPort = simple_strtoul(&eioaport[5], NULL, 10);
            if(eioaPort < EIOA_NUM_PORT_INDEXES && index_by_port_xgmac[eioaPort] != -1) {
                index_by_port[eioaPort] = index_by_port_xgmac[eioaPort];
                /* set same port type for all ports */
                memset_int(port_type_by_index, EIOA_PORT_TYPE_XGMAC, 
                        EIOA_NUM_PORTS);
            } else {
                printf("Invalid xgmac port %d\n", eioaPort);
			    return -1;
            }
        } else {
			printf("If set, eioaport must be one of the following:\n"
                "\tauto-gmac,\n"
                "\tauto-xgmac,\n"
                "\tgmac[0-4,16-20,32,33,48,49,64,65,80,81,96,97,112,113],\n"
                "\txgmac[0,1,16,17,32,33,48,49,64,65,80,81,96,97,112,113]\n");
			return -1;
		}
    }

	if (0 != ncp_dev_reset()) {
		printf("Reset Failed\n");
		return -1;
	}

    debug("Configuring MME...");
	if (0 != ncp_dev_configure(mme)) {
		printf("MME Configuration Failed\n");
		return -1;
	}
    debug("done\n");

    debug("Configuring PBM...");
    if (0 != ncp_dev_configure(pbm)) {
		printf("PBM Configuration Failed\n");
		return -1;
	}
    debug("done\n");

    debug("Configuring VP...");
	if (0 != ncp_dev_configure(vp)) {
		printf("Virtual Pipeline Configuration Failed\n");
		return -1;
	}
    debug("done\n");

    debug("Configuring NCA...");
	if (0 != ncp_dev_configure(nca)) {
		printf("NCA Configuration Failed\n");
		return -1;
	}
    debug("done\n");

    debug("Configuring Uboot task io...");
    /* initialize task io */
	NCP_CALL(ncp_task_uboot_config());
    debug("done\n");

    debug("Configuring EIOA...");
	if (0 != ncp_dev_configure(eioa)) {
		printf("EIOA Configuration Failed\n");
		return -1;
	}
    debug("done\n");

    debug("Creating task hdl.\n");
    /* create task handle */
    NCP_CALL(ncp_task_hdl_create(
                &gNCP,              /* dummy NCP handle */
                threadQueueSetId,   /* thread queue set id */    
                FALSE,              /* shared */
                FALSE,              /* ordered task completion - dont care */
                &taskHdl));         /* task handle */

    debug("Binding to nca queue %d and task recv queue %d.\n", 
            ncaQueueId, recvQueueId);
    /* allocate recv queue */
    NCP_CALL(ncp_task_recv_queue_bind(
                taskHdl,        /* task handle */
                ncaQueueId,     /* NCA queue id */
                16,             /* weight */
                FALSE,          /* shared */
                TRUE,           /* fixed receive queue id */
                &recvQueueId)); /* receive queue id if not fixed */

 ncp_return:
	if (NCP_ST_SUCCESS != ncpStatus) {
        printf("ERROR: status=%d\n", ncpStatus);
		lsi_eioa_eth_halt(dev);

		return -1;
	}

	/*
	  Make sure the network is connected.
	*/

	if (NCP_USE_ALL_PORTS == eioaPort) {
		/* Use all ports. */
		for (i = 0; i < EIOA_NUM_PORTS; ++i) {
            if(port_type_by_index[i] == EIOA_PORT_TYPE_GMAC) {
    			if (0 != line_setup(i)) {
    				return -1;
    			}
            }
#if 0
            else {
                if (0 != line_setup_xgmac(i)) {
    				return -1;
    			}
            }
#endif
		}
	} else {
        if(port_type_by_index[index_by_port[eioaPort]] == EIOA_PORT_TYPE_GMAC) {
    		if (0 != line_setup(index_by_port[eioaPort])) {
    			return -1;
    		}
        }
#if 0
        else {
            if (0 != line_setup_xgmac(index_by_port[eioaPort])) {
				return -1;
			}
        }
#endif
	}

	initialized = 1;

	return 0;
}

/*
  -------------------------------------------------------------------------------
  finalize_task_io
*/

void
finalize_task_io(void)
{
    int rc = 0;
	unsigned long value;
    ncp_st_t ncpStatus = NCP_ST_SUCCESS;
	/*
	  Stop the queue.
	*/

	/* Disable EIOA gmac NEMACs. */
    ncr_modify32(NCP_REGION_ID(0x1f, 0x11), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0x3c0, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0x480, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0x540, 0x0000000c, 0x0);
    ncr_modify32(NCP_REGION_ID(0x17, 0x11), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0x3c0, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0x480, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0x540, 0x0000000c, 0x0);
    ncr_modify32(NCP_REGION_ID(0x28, 0x11), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x28, 0x12), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x29, 0x11), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x29, 0x12), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2a, 0x11), 0x300, 0x0000000c, 0x0);
    ncr_modify32(NCP_REGION_ID(0x2a, 0x12), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2b, 0x11), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2b, 0x12), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2c, 0x11), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2c, 0x12), 0x300, 0x0000000c, 0x0);
    ncr_modify32(NCP_REGION_ID(0x2d, 0x11), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2d, 0x12), 0x300, 0x0000000c, 0x0);

    /* Disable EIOA xgmac NEMACs. */
    ncr_modify32(NCP_REGION_ID(0x1f, 0x11), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0xc00, 0x0000000c, 0x0);
    ncr_modify32(NCP_REGION_ID(0x17, 0x11), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0xc00, 0x0000000c, 0x0);
    ncr_modify32(NCP_REGION_ID(0x28, 0x11), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x28, 0x12), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x29, 0x11), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x29, 0x12), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2a, 0x11), 0xc00, 0x0000000c, 0x0);
    ncr_modify32(NCP_REGION_ID(0x2a, 0x12), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2b, 0x11), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2b, 0x12), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2c, 0x11), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2c, 0x12), 0xc00, 0x0000000c, 0x0);
    ncr_modify32(NCP_REGION_ID(0x2d, 0x11), 0xc00, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x2d, 0x12), 0xc00, 0x0000000c, 0x0);

	/* swreset gmac and xgmac nemacs */
    ncr_or(NCP_REGION_ID(0x1f, 0x11), 0x20, 0x10f);
	ncr_or(NCP_REGION_ID(0x1f, 0x12), 0x20, 0x10f);
    ncr_or(NCP_REGION_ID(0x17, 0x11), 0x20, 0x10f);
	ncr_or(NCP_REGION_ID(0x17, 0x12), 0x20, 0x10f);
    ncr_or(NCP_REGION_ID(0x28, 0x11), 0x20, 0x10f);
	ncr_or(NCP_REGION_ID(0x28, 0x12), 0x20, 0x10f);
    ncr_or(NCP_REGION_ID(0x29, 0x11), 0x20, 0x10f);
	ncr_or(NCP_REGION_ID(0x29, 0x12), 0x20, 0x10f);
    ncr_or(NCP_REGION_ID(0x2a, 0x11), 0x20, 0x10f);
	ncr_or(NCP_REGION_ID(0x2a, 0x12), 0x20, 0x10f);
    ncr_or(NCP_REGION_ID(0x2b, 0x11), 0x20, 0x10f);
	ncr_or(NCP_REGION_ID(0x2b, 0x12), 0x20, 0x10f);
    ncr_or(NCP_REGION_ID(0x2c, 0x11), 0x20, 0x10f);
	ncr_or(NCP_REGION_ID(0x2c, 0x12), 0x20, 0x10f);
    ncr_or(NCP_REGION_ID(0x2d, 0x11), 0x20, 0x10f);
	ncr_or(NCP_REGION_ID(0x2d, 0x12), 0x20, 0x10f);

	/* Disable ports in EIOA Cores. */
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x1f, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(0), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x1f, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(1), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x1f, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(2), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x1f, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(3), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x1f, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(4), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x17, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(0), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x17, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(1), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x17, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(2), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x17, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(3), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x17, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(4), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x28, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(0), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x28, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(1), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x29, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(0), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x29, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(1), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x2a, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(0), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x2a, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(1), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x2b, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(0), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x2b, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(1), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x2c, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(0), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x2c, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(1), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x2d, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(0), 0x00000003, 0x0));
    NCR_CALL(ncr_modify32(NCP_REGION_ID(0x2d, 0x10), 
                NCP_EIOA_GEN_CFG_REG_OFFSET(1), 0x00000003, 0x0));

	/* Disable iPCQ 0 (only queue 0 is used) */
	value = readl(NCA + 0x11800 + (ncaQueueId * 0x10));
	value &= ~0x00000400;
	writel(value, (NCA + 0x11800 + (ncaQueueId * 0x10)));

	/*
	  Shut down task lite.
	*/

	if (taskHdl) {
        NCP_CALL(ncp_task_recv_queue_unbind(taskHdl, recvQueueId));
        NCP_CALL(ncp_task_hdl_remove(taskHdl));
	}

	ncp_task_uboot_unconfig();

	initialized = 0;

ncp_return:
	return;
}

/*
  ==============================================================================
  ==============================================================================
  Public Interface
  ==============================================================================
  ==============================================================================
*/

/*
  -------------------------------------------------------------------------------
  lsi_eioae_eth_halt
*/

void
lsi_eioa_eth_halt(struct eth_device *dev)
{
	if (0 != initialized)
		finalize_task_io();

	return;
}

/*
  ----------------------------------------------------------------------
  lsi_eioa_eth_init
*/

int
lsi_eioa_eth_init(struct eth_device *dev, bd_t *bd)
{
	if (0 == initialized)
		if (0 != initialize_task_io(dev)) {
			printf("Failed to Initialize TaskIO Lite!\n");
			return -1;
		}

	return 0;
}

/*
  -------------------------------------------------------------------------------
  lsi_eioa_eth_send
*/

int
lsi_eioa_eth_send(struct eth_device *dev, volatile void *packet, int length)
{
    ncp_st_t ncpStatus = NCP_ST_SUCCESS;
	int bytes_sent = 0;
	void *taskAddr;
	int i;
    ncp_task_ncaV2_send_meta_t taskMetaData;

    debug("lsi_eioa_eth_send(): packet=0x%p, length=%d\n");

	for (i = 0; i < EIOA_NUM_PORTS; ++i) {
        /* if sending on single port, skip other ports */
        if(eioaPort != NCP_USE_ALL_PORTS && port_by_index[i] != eioaPort) {
            continue;
        }

        debug("lsi_eioa_eth_send(): sending to port=%d\n", eioaPort);

        bytes_sent = 0;
        
        NCP_CALL(ncp_task_ncav2_buffer_alloc(taskHdl, length, &taskAddr));   /* V2 */ 

		if (NULL == taskAddr) {
			printf("Couldn't allocate send buffer.\n");
			return 0;
		}

        /* copy the task to buffer */
		memcpy(taskAddr, (void *)packet, length);
/* HACK: Temporary invalidate until cache coherency is figured in uboot */
#ifdef USE_CACHE_SYNC
        flush_cache((unsigned long)taskAddr, length);
#endif

        /* init task meta data */
		memset(&taskMetaData, 0, sizeof(taskMetaData));
        taskMetaData.virtualFlowID  = sendVpId;
        taskMetaData.priority       = 0;
        taskMetaData.pduSegSize0    = length;
        taskMetaData.ptrCnt         = 1;
        taskMetaData.pduSegAddr0    = (ncp_uint64_t)taskAddr;
        taskMetaData.params[0]   = port_by_index[i]; /* output port */
/* HACK: Temporary invalidate until cache coherency is figured in uboot */
#ifdef USE_CACHE_SYNC
		flush_cache((unsigned long)&taskMetaData, sizeof(taskMetaData));
#endif

		if (length != task_send(&taskMetaData)) {
			printf("Send Failed on Port %d\n", port_by_index[i]);
		} else {
			bytes_sent = length;
		}

        /* if sending on single port, skip rest of the ports */
        if(eioaPort != NCP_USE_ALL_PORTS && port_by_index[i] != eioaPort) {
            break;
        }
	}

ncp_return:
    if(ncpStatus != NCP_ST_SUCCESS) {
        printf("Failed to send packet. status=%d\n", ncpStatus);
    }
	return bytes_sent;
}

/*
  -------------------------------------------------------------------------------
  lsi_eioa_eth_rx
*/

int
lsi_eioa_eth_rx(struct eth_device *dev)
{
	ncp_st_t ncpStatus;
	ncp_task_ncaV2_recv_buf_t *task;
	int bytes_received = 0;
    ncp_vp_hdl_t vpHdl;
    ncp_uint8_t engineSeqId;
    ncp_uint8_t recvQueueId;

    /* receive the task */
    NCP_CALL(ncp_task_ncav2_recv(taskHdl, &recvQueueId, &vpHdl, &engineSeqId, 
                &task, NULL, FALSE));

    if(dumprx) {
        axxia_dump_packet("LSI_EIOA RX", (void *)task->pduSegAddr0, 
                task->pduSegSize0);
    }

/* HACK: Temporary invalidate until cache coherency is figured in uboot */
#ifdef USE_CACHE_SYNC
    invalidate_dcache_all();
#endif

    debug("lsi_eioa_eth_rx(): received task addr=0x%p, port=%d, size=%d\n", 
                task, task->params[0], task->pduSegSize0);

    /* 
     * If receiving on any port or on the single configured port, handle the 
     * packet and give it to the up layer. Otherwise, free it and return.
     */
	if ((NCP_USE_ALL_PORTS == eioaPort) ||
        (NCP_USE_ALL_PORTS != eioaPort && task->params[0] == eioaPort)) {

    	if (NCP_USE_ALL_PORTS == eioaPort) {
    		eioaPort = task->params[0];
    		printf("Selecting EIOA Port GMAC%02d\n", eioaPort);
    	}

    	bytes_received = task->pduSegSize0;

        /* copy the received packet to the up layer buffer */
    	memcpy((void *)NetRxPackets[0], (void *)task->pduSegAddr0, bytes_received);

        /* give the packet to the up layer */
    	if (0 == loopback && 0 == rxtest)
    		NetReceive(NetRxPackets[0], bytes_received);
    }

    /* free the buffer */
    NCP_CALL(ncp_task_ncav2_free_rx_task(taskHdl, task));
 ncp_return:
    if (NCP_ST_TASK_RECV_QUEUE_EMPTY == ncpStatus) {
		return 0;
	} else if (NCP_ST_SUCCESS != ncpStatus) {
		printf("%s:%d - NCP_CALL() Failed: 0x%08x\n",
		       __FILE__, __LINE__, ncpStatus);
		return 0;
	}

	return bytes_received;
}

/*
  -------------------------------------------------------------------------------
  lsi_eioa_receive_test
*/

void
lsi_eioa_receive_test(struct eth_device *dev)
{
	int packets_received = 0;
	bd_t *bd = gd->bd;

    rxtest = 1;
	eth_halt();

	if (0 != eth_init(bd)) {
        rxtest = 0;
		eth_halt();
		return;
	}

	for (;;) {
        int packet_len = eth_rx();
		if (0 != packet_len) {
			++packets_received;
        }

		if (ctrlc())
			break;
	}

    rxtest = 0;
	eth_halt();
	printf("EIOA Receive Test Interrupted.  Received %d packets.\n",
	       packets_received);

	return;
}

/*
  -------------------------------------------------------------------------------
  lsi_eioa_loopback_test
*/

void
lsi_eioa_loopback_test(struct eth_device *dev)
{
	bd_t *bd = gd->bd;
	int bytes_received;
	int packets_looped = 0;

	loopback = 1;
	eth_halt();

	if (0 != eth_init(bd)) {
        loopback = 0;
		eth_halt();
		return;
	}

	for (;;) {
		if (0 != (bytes_received = eth_rx())) {
			if (bytes_received !=
			    eth_send((void *)NetRxPackets[0], bytes_received)) {
				printf("lsi_eioa_eth_send() failed.\n");
			} else {
				++packets_looped;
			}
		}

		if (ctrlc())
			break;
	}

	loopback = 0;
	eth_halt();
	printf("EIOA Loopback Test Interrupted.  Looped back %d packets.\n",
	       packets_looped);

	return;
}


#if 0
/*
  ------------------------------------------------------------------------------
  line_setup_xgmac
*/

static int
line_setup_xgmac(int index)
{
	int rc;
	int retries = 100000;
	unsigned long eioaRegion;
	unsigned long xgmacRegion;
	unsigned long xgmacPortOffset;
	unsigned long eioaPortOffset;
    unsigned long hwPortIndex;
	unsigned long ncr_status;
	char *envstring;
	unsigned short status;
	unsigned long top;
	unsigned long bottom;
	unsigned short ad_value;
	unsigned short ge_ad_value;
	unsigned short control;

    if(index >= 128 || (index < 128 && index_by_port_xgmac[port_by_index[index]] != -1))
    {
        printf("Invalid xgmac port %d (index=%d)\n", port_by_index[index],
            index);
	    return -1;
    }
    
	/* Set the region and offset. */
    if (2 > index) {
        hwPortIndex = index;
		eioaRegion = NCP_REGION_ID(31, 16); /* 0x1f.0x10 */
		xgmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(31, 17) : /* 0x1f.0x11 */ 
                                            NCP_REGION_ID(31, 18)); /* 0x1f.0x12 */
		xgmacPortOffset = 0xc0 * hwPortIndex;
	} else if (7 > index) {
	    hwPortIndex = index - 5;
		eioaRegion = NCP_REGION_ID(23, 16); /* 0x17.0x10 */
        xgmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(23, 17) : /* 0x1f.0x11 */ 
                                            NCP_REGION_ID(23, 18)); /* 0x1f.0x12 */
		xgmacPortOffset = 0xc0 * hwPortIndex;
	} else if (12 > index) {
	    hwPortIndex = index - 10;
		eioaRegion = NCP_REGION_ID(40, 16); /* 0x28.0x10 */
        xgmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(40, 17) : /* 0x1f.0x11 */ 
                                            NCP_REGION_ID(40, 18)); /* 0x1f.0x12 */
		xgmacPortOffset = 0xc0 * hwPortIndex;
	} else if (14 > index) {
	    hwPortIndex = index - 12;
		eioaRegion = NCP_REGION_ID(41, 16); /* 0x29.0x10 */
        xgmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(41, 17) : /* 0x1f.0x11 */ 
                                            NCP_REGION_ID(41, 18)); /* 0x1f.0x12 */
		xgmacPortOffset = 0xc0 * hwPortIndex;
	} else if (16 > index) {
	    hwPortIndex = index - 14;
		eioaRegion = NCP_REGION_ID(42, 16); /* 0x2a.0x10 */
        xgmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(42, 17) : /* 0x1f.0x11 */ 
                                            NCP_REGION_ID(42, 18)); /* 0x1f.0x12 */
		xgmacPortOffset = 0xc0 * hwPortIndex;
	} else if (18 > index) {
	    hwPortIndex = index - 16;
		eioaRegion = NCP_REGION_ID(43, 16); /* 0x2b.0x10 */
        xgmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(43, 17) : /* 0x1f.0x11 */ 
                                            NCP_REGION_ID(43, 18)); /* 0x1f.0x12 */
		xgmacPortOffset = 0xc0 * hwPortIndex;
	} else if (20 > index) {
	    hwPortIndex = index - 18;
		eioaRegion = NCP_REGION_ID(44, 16); /* 0x2c.0x10 */
        xgmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(44, 17) : /* 0x1f.0x11 */ 
                                            NCP_REGION_ID(44, 18)); /* 0x1f.0x12 */
		xgmacPortOffset = 0xc0 * hwPortIndex;
	} else if (22 > index) {
	    hwPortIndex = index - 20;
		eioaRegion = NCP_REGION_ID(45, 16); /* 0x2d.0x10 */
        xgmacRegion = ((hwPortIndex == 0) ? NCP_REGION_ID(45, 17) : /* 0x1f.0x11 */ 
                                            NCP_REGION_ID(45, 18)); /* 0x1f.0x12 */
		xgmacPortOffset = 0xc0 * hwPortIndex;
	} else {
	    printf("Invalid xgmac port %d\n", port_by_index[index]);
	    return -1;
    }

	/* Disable stuff. */
	NCR_CALL(ncr_modify32(xgmacRegion, 0xc30 + xgmacPortOffset, 0x73f, 0));
	NCR_CALL(ncr_modify32(xgmacRegion, 0xc00 + xgmacPortOffset, 0x8, 0x0));
	NCR_CALL(ncr_modify32(eioaRegion, 
                NCP_EIOA_GEN_CFG_REG_OFFSET(hwPortIndex) + 0x0, 
                0x00000003, 0x0));
	NCR_CALL(ncr_modify32(xgmacRegion, 0xc00 + xgmacPortOffset, 0x4, 0x0));

	/* Check for "macspeed".  If set, ignore the PHYs... */
	envstring = getenv("macspeed");

	if (NULL != envstring) {
		printf("Overriding MAC Speed Settings; Ignoring PHY(s)!"
		       " %s\n", envstring);

		NCR_CALL(ncr_read32(xgmacRegion, 0x324 + xgmacPortOffset,
				    &ncr_status));
		ncr_status &= ~0x3c;
		ncr_status |= 0x08; /* Force Link Up */

		if (0 == strcmp("10MH", envstring)) {
		} else if (0 == strcmp("10MF", envstring)) {
			ncr_status |= 0x04;
		} else if (0 == strcmp("100MH", envstring)) {
			ncr_status |= 0x10;
		} else if (0 == strcmp("100MF", envstring)) {
			ncr_status |= 0x14;
		} else if (0 == strcmp("1G", envstring)) {
			ncr_status |= 0x24;
		} else {
			printf("macspeed must be set to 10MH, 10MF, 100MH, "
			       "100MF, or 1G\n");
			return -1;
		}

		NCR_CALL(ncr_write32(xgmacRegion, 0x324 + xgmacPortOffset,
				     ncr_status));
	} else {
		/* Get ad_value and ge_ad_value from the environment. */
		envstring = getenv("ad_value");

		if (NULL == envstring) {
			ad_value = 0x1e1;
		} else {
			ad_value = simple_strtoul(envstring, NULL, 0);
		}

		envstring = getenv("ge_ad_value");

		if (NULL == envstring) {
			ge_ad_value = 0x300;
		} else {
			ge_ad_value = simple_strtoul(envstring, NULL, 0);
		}

		/* Set the AN advertise values. */
		mdio_write(phy_by_index[index], 4, ad_value);
		mdio_write(phy_by_index[index], 9, ge_ad_value);

		/* Force re-negotiation. */
		control = mdio_read(phy_by_index[index], 0);
		control |= 0x200;
		mdio_write(phy_by_index[index], 0, control);

		DELAY();

		/* Wait for AN complete. */
		for (;;) {
			status = mdio_read(phy_by_index[index], 1);

			if (0 != (status & 0x20))
				break;

			if (0 == retries--) {
				printf("GMAC%d: AN Timed Out.\n",
					    port_by_index[index]);
				return -1;
			}

			DELAY();
		}

		if (0 == (status & 0x4)) {
			printf("GMAC%d: LINK is Down.\n",
				    port_by_index[index]);

			if (NCP_USE_ALL_PORTS != eioaPort)
				return -1; /* Don't Error Out in AUTO Mode. */
		} else {
			status = mdio_read(phy_by_index[index], 0x1c);
			printf("GMAC%02d: ", port_by_index[index]);

			switch ((status & 0x18) >> 3) {
			case 0:
				puts("10M");
				break;
			case 1:
				puts("100M");
				break;
			case 2:
				puts("1G");
				break;
			default:
				puts("UNKNOWN");
				break;
			}

			printf(" %s\n",
			       (0 == (status & 0x20)) ?
			       "Half Duplex" : "Full Duplex");
			DELAY();

			/* Make the MAC match. */

			NCR_CALL(ncr_read32(xgmacRegion, 0x324 + xgmacPortOffset,
					    &ncr_status));
			ncr_status &= ~0x3c;
			ncr_status |= 0x08;	/* Force Link Up */

			if (0 != (status & 0x20))
				ncr_status |= 0x04; /* Force Full Duplex */

			/* Set the Speed */
			ncr_status |= (((status & 0x18) >> 3) << 4);

			NCR_CALL(ncr_write32(xgmacRegion, 0x324 + xgmacPortOffset,
					     ncr_status));
		}
	}

	/*
	  Set the Ethernet addresses...
	*/

	top = (ethernet_address[0] << 8) | ethernet_address[1];
	bottom = (ethernet_address[2] << 24) | (ethernet_address[3] << 16) |
	  (ethernet_address[4] << 8) | (ethernet_address[5]);

	/* - EIOA */
	NCR_CALL(ncr_write32(eioaRegion, 0xa0 + eioaPortOffset, bottom));
	NCR_CALL(ncr_write32(eioaRegion, 0xa4 + eioaPortOffset, top));

	/* - Source */
	NCR_CALL(ncr_write32(xgmacRegion, 0x304 + xgmacPortOffset, bottom));
	NCR_CALL(ncr_write32(xgmacRegion, 0x308 + xgmacPortOffset, top));

	/* - Unicast */
	NCR_CALL(ncr_write32(xgmacRegion, 0x344 + xgmacPortOffset, bottom));
	NCR_CALL(ncr_write32(xgmacRegion, 0x348 + xgmacPortOffset, bottom));
	NCR_CALL(ncr_write32(xgmacRegion, 0x34c + xgmacPortOffset,
			     ((top & 0xffff) << 16) | (top & 0xffff)));
	NCR_CALL(ncr_write32(xgmacRegion, 0x350 + xgmacPortOffset, bottom));
	NCR_CALL(ncr_write32(xgmacRegion, 0x354 + xgmacPortOffset, top));

	/* Enable stuff. */
	NCR_CALL(ncr_modify32(xgmacRegion, 0x300 + xgmacPortOffset, 0x8, 0x8));
	NCR_CALL(ncr_modify32(eioaRegion, 
                NCP_EIOA_GEN_CFG_REG_OFFSET(hwPortIndex) + 0x0, 
                0x00000003, 0x3));
	NCR_CALL(ncr_modify32(xgmacRegion, 0x300 + xgmacPortOffset, 0x4, 0x4));

	/* Unicast Filtering based on the Ethernet Address set above. */
	if (0 == rxtest && 0 == loopback)
		NCR_CALL(ncr_modify32(xgmacRegion, 0x330 + xgmacPortOffset,
				      0x3f, 0x09));

	return 0;

 ncp_return:

	return -1;
}

/*
  ------------------------------------------------------------------------------
  line_renegotiate
*/

static int
line_renegotiate(int index)
{
	int rc;
	unsigned long eioaRegion;
	unsigned long gmacRegion;
	unsigned long gmacPortOffset;
	char *envstring;
	unsigned short ad_value;
	unsigned short ge_ad_value;
	unsigned short control;

	/* Set the region and offset. */
	if (4 > index) {
		eioaRegion = NCP_REGION_ID(31, 16); /* 0x1f.0x10 */
		gmacRegion = NCP_REGION_ID(31, 18); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * index;
	} else {
		eioaRegion = NCP_REGION_ID(23, 16); /* 0x17.0x10 */
		gmacRegion = NCP_REGION_ID(23, 18); /* 0x17.0x12 */
		gmacPortOffset = 0xc0 * (index - 4);
	}

	/* Disable stuff. */
	NCR_CALL(ncr_modify32(gmacRegion, 0x330 + gmacPortOffset, 0x3f, 0));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x8, 0x0));
	NCR_CALL(ncr_modify32(eioaRegion, 0x70, 0x11000000, 0x0));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x4, 0x0));

	/* Check for "macspeed".  If set, ignore the PHYs... */
	envstring = getenv("macspeed");

	if (NULL == envstring) {
		/* Get ad_value and ge_ad_value from the environment. */
		envstring = getenv("ad_value");

		if (NULL == envstring) {
			ad_value = 0x1e1;
		} else {
			ad_value = simple_strtoul(envstring, NULL, 0);
		}

		envstring = getenv("ge_ad_value");

		if (NULL == envstring) {
			ge_ad_value = 0x300;
		} else {
			ge_ad_value = simple_strtoul(envstring, NULL, 0);
		}

		/* Set the AN advertise values. */
		mdio_write(phy_by_index[index], 4, ad_value);
		mdio_write(phy_by_index[index], 9, ge_ad_value);

		/* Force re-negotiation. */
		control = mdio_read(phy_by_index[index], 0);
		control |= 0x200;
		mdio_write(phy_by_index[index], 0, control);
	}

	return 0;

 ncp_return:

	return -1;
}

/*
  ------------------------------------------------------------------------------
  alloc_nvm
*/

static void *
alloc_nvm(int size)
{
#ifdef TRACE_ALLOCATION
	void *address;

	address = malloc(size);
	printf(" NVM_ALLOC: 0x%08x bytes at 0x%p\n", size, address);

	return address;
#else
	return malloc(size);
#endif
}

/*
  ------------------------------------------------------------------------------
  free_nvm
*/

static void
free_nvm(void *address)
{
	free(address);
#ifdef TRACE_ALLOCATION
	printf("  NVM_FREE: at 0x%p\n", address);
#endif
}

/*
  ------------------------------------------------------------------------------
  va2pa
*/

static ncp_uint32_t
va2pa(void *virtual_address)
{
	return (ncp_uint32_t)virtual_address;
}


#endif
