/*
 * 
  * J Logan  jl 
  *
  * (C) Copyright 2008
 * Stuart Wood, Lab X Technologies <stuart.wood@labxtechnologies.com>
 *
 * (C) Copyright 2004
 * Jian Zhang, Texas Instruments, jzhang@ti.com.

 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>

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

/* #define DEBUG */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>


#if defined(CONFIG_CMD_SAVEENV) && defined(CONFIG_LSI_SSP)
#define CMD_SAVEENV
#elif defined(CONFIG_ENV_OFFSET_REDUND)
#error Cannot use CONFIG_ENV_OFFSET_REDUND without CONFIG_CMD_SAVEENV & CONFIG_CMD_NAND
#endif

#if defined(CONFIG_ENV_SIZE_REDUND) && (CONFIG_ENV_SIZE_REDUND != CONFIG_ENV_SIZE)
#error CONFIG_ENV_SIZE_REDUND should be the same as CONFIG_ENV_SIZE
#endif

#ifdef CONFIG_INFERNO
#error CONFIG_INFERNO not supported yet
#endif

#ifndef CONFIG_ENV_RANGE
#define CONFIG_ENV_RANGE	CONFIG_ENV_SIZE
#endif

/* references to names in env_common.c */
#if 0
extern uchar default_environment[];
#endif
extern const unsigned char default_environment[];

char * env_name_spec = "Serial Flash";


#if defined(ENV_IS_EMBEDDED)
#if 0
extern uchar environment[];
#endif
extern unsigned char environment[];
env_t *env_ptr = (env_t *)(&environment[0]);
#else /* ! ENV_IS_EMBEDDED */
env_t *env_ptr = (env_t *)CONFIG_ENV_OFFSET;
#endif /* ENV_IS_EMBEDDED */


/* local functions */
#if !defined(ENV_IS_EMBEDDED)
static void use_default(void);
#endif

DECLARE_GLOBAL_DATA_PTR;

uchar env_get_char_spec (int index)
{
	return ( *((uchar *)(gd->env_addr + index)) );
}


/* 
 * 
 * Mark env OK for now. env_relocate() in env_common.c
 * will call our relocate function which does the real
 * validation.
 
 */
int env_init(void)
{
#if defined(ENV_IS_EMBEDDED) 
	int crc1_ok = 0, crc2_ok = 0;
	env_t *tmp_env1;

#ifdef CONFIG_ENV_OFFSET_REDUND
	env_t *tmp_env2;

	tmp_env2 = (env_t *)((ulong)env_ptr + CONFIG_ENV_SIZE);
	crc2_ok = (crc32(0, tmp_env2->data, ENV_SIZE) == tmp_env2->crc);
#endif

	tmp_env1 = env_ptr;

	crc1_ok = (crc32(0, tmp_env1->data, ENV_SIZE) == tmp_env1->crc);

	if (!crc1_ok && !crc2_ok) {
		gd->env_addr  = 0;
		gd->env_valid = 0;

		return 0;
	} else if (crc1_ok && !crc2_ok) {
		gd->env_valid = 1;
	}
#ifdef CONFIG_ENV_OFFSET_REDUND
	else if (!crc1_ok && crc2_ok) {
		gd->env_valid = 2;
	} else {
		/* both ok - check serial */
		if(tmp_env1->flags == 0xffffffff && tmp_env2->flags == 0)
			gd->env_valid = 2;
		else if(tmp_env2->flags == 0xffffffff && tmp_env1->flags == 0)
			gd->env_valid = 1;
		else if(tmp_env1->flags > tmp_env2->flags)
			gd->env_valid = 1;
		else if(tmp_env2->flags > tmp_env1->flags)
			gd->env_valid = 2;
		else /* flags are equal - almost impossible */
			gd->env_valid = 1;
	}

	if (gd->env_valid == 2)
		env_ptr = tmp_env2;
	else
#endif
	if (gd->env_valid == 1)
		env_ptr = tmp_env1;

	gd->env_addr = (ulong)env_ptr->data;

#else /* ENV_IS_EMBEDDED  */
	gd->env_addr  = (ulong)&default_environment[0];
	gd->env_valid = 1;
#endif /* ENV_IS_EMBEDDED */
	return (0);
}

#ifdef CMD_SAVEENV
/*
 writeenv assumes that target flash/eeprom memory has been erased and that address is page aligned
 */
int writeenv(size_t offset, u_char *buf)
{
	ssp_init(0, 0);
	return ssp_write(buf, offset, CONFIG_ENV_SIZE, 0);
	
}
#ifdef CONFIG_ENV_OFFSET_REDUND
int saveenv(void)
{
	int ret = 0;
	env_t   env_new;
        ssize_t len;
        char    *res;

	res = (char *)&env_new.data;
        len = hexport_r(&env_htab, '\0', 0, &res, ENV_SIZE, 0, NULL);
        if (len < 0) {
                printf("Cannot export environment\n");
		return 1;
        }

	env_new.flags++;
#if 0
	env_crc_update ();
#endif
	env_new.crc = crc32(0, env_new.data, ENV_SIZE);
#if 0
	env_import ((unsigned char *)env_ptr, 1);
#endif
	
	if (CONFIG_ENV_RANGE < CONFIG_ENV_SIZE)
		return 1;
	if(gd->env_valid == 1) {
		printf("Writing to redundant seeprom (0x%x)... ",
		       CONFIG_ENV_OFFSET_REDUND);
		ret = writeenv(CONFIG_ENV_OFFSET_REDUND, (u_char *) &env_new);
	} else {
		printf("Writing to seeprom (0x%x)... ", CONFIG_ENV_OFFSET);
		ret = writeenv(CONFIG_ENV_OFFSET, (u_char *) &env_new);
	}
	if (ret) {
		puts("FAILED!\n");
		return 1;
	}

	puts ("done\n");
	gd->env_valid = (gd->env_valid == 2 ? 1 : 2);

	return ret;
}
#else /* ! CONFIG_ENV_OFFSET_REDUND */
int saveenv(void)
{
	int ret = 0;
	env_t   env_new;
        ssize_t len;
        char    *res;


    res = (char *)&env_new.data;
        len = hexport_r(&env_htab, '\0', 0, &res, ENV_SIZE, 0, NULL);
        if (len < 0) {
                printf("Cannot export environment\n");
		return 1;
        }
#if 0
	env_crc_update ();
#endif
	env_new.crc = crc32(0, env_new.data, ENV_SIZE);
#if 0
	env_import ((unsigned char *)env_ptr, 1);
#endif

	if (CONFIG_ENV_RANGE < CONFIG_ENV_SIZE)
		return 1;
	puts ("Erasing seeprom...\n");
	if (seeprom_sector_erase(CONFIG_ENV_OFFSET))
		return 1;
	puts ("Writing to seeprom... ");
	if (writeenv(CONFIG_ENV_OFFSET, (u_char *) env_new)) {
		puts("FAILED!\n");
		return 1;
	}

	puts ("done\n");
	return ret;
}
#endif /* CONFIG_ENV_OFFSET_REDUND */
#endif /* CMD_SAVEENV */

int readenv (size_t offset, u_char * buf)
{
	ssp_init(0, 1);
	return ssp_read(buf,offset,CONFIG_ENV_SIZE);
}

#ifdef CONFIG_ENV_OFFSET_REDUND
void env_relocate_spec (void)
{
#if !defined(ENV_IS_EMBEDDED)
	int crc1_ok = 0, crc2_ok = 0;
	env_t *tmp_env1, *tmp_env2;

	tmp_env1 = (env_t *) malloc(CONFIG_ENV_SIZE);
	tmp_env2 = (env_t *) malloc(CONFIG_ENV_SIZE_REDUND);

	if ((tmp_env1 == NULL) || (tmp_env2 == NULL)) {
		puts("Can't allocate buffers for environment\n");
		free (tmp_env1);
		free (tmp_env2);
		return use_default();
	}

	if (readenv(CONFIG_ENV_OFFSET, (u_char *) tmp_env1))
		puts("No Valid Environment Area Found\n");
	if (readenv(CONFIG_ENV_OFFSET_REDUND, (u_char *) tmp_env2))
		puts("No Valid Reundant Environment Area Found\n");

#if 1
	crc1_ok = (crc32(0, tmp_env1->data, ENV_SIZE) == tmp_env1->crc);
	crc2_ok = (crc32(0, tmp_env2->data, ENV_SIZE) == tmp_env2->crc);
#endif
#if 0
	crc1_ok = 1;
	crc2_ok = 1;
#endif

	if(!crc1_ok && !crc2_ok) {
		free(tmp_env1);
		free(tmp_env2);
		return use_default();
	} else if(crc1_ok && !crc2_ok)
		gd->env_valid = 1;
	else if(!crc1_ok && crc2_ok)
		gd->env_valid = 2;
	else {
		/* both ok - check serial */
		if(tmp_env1->flags == 0xffffffff && tmp_env2->flags == 0)
			gd->env_valid = 2;
		else if(tmp_env2->flags == 0xffffffff && tmp_env1->flags == 0)
			gd->env_valid = 1;
		else if(tmp_env1->flags > tmp_env2->flags)
			gd->env_valid = 1;
		else if(tmp_env2->flags > tmp_env1->flags)
			gd->env_valid = 2;
		else /* flags are equal - almost impossible */
			gd->env_valid = 1;

	}

	//free(env_ptr);
	if(gd->env_valid == 1) {
		env_ptr = tmp_env1;
		free(tmp_env2);
	} else {
		env_ptr = tmp_env2;
		free(tmp_env1);
	}

	env_import((char *)env_ptr, 0);

#endif /* ! ENV_IS_EMBEDDED */
}
#else /* ! CONFIG_ENV_OFFSET_REDUND */
/*
 */
void env_relocate_spec (void)
{
#if !defined(ENV_IS_EMBEDDED)
	int ret;
printf("In env_relocate no redund! \n\r";

	ret = readenv(CONFIG_ENV_OFFSET, (u_char *) env_ptr);
	if (ret)
		return use_default();

	if (crc32(0, env_ptr->data, ENV_SIZE) != env_ptr->crc)
		return use_default();

	env_import((char *)env_ptr, 0);

#endif /* ! ENV_IS_EMBEDDED */
}
#endif /* CONFIG_ENV_OFFSET_REDUND */

#if !defined(ENV_IS_EMBEDDED)
static void use_default()
{
	puts ("*** Warning - bad CRC or NAND, using default environment\n\n");
	set_default_env(NULL);
}
#endif