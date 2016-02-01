/*
 * mach-rpi2.c
 *
 * Copyright(c) 2007-2016 Jianjun Jiang <8192542@qq.com>
 * Official site: http://xboot.org
 * Mobile phone: +86-18665388956
 * QQ: 8192542
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <xboot.h>
#include <bcm2836/reg-pm.h>

static const struct mmap_t mach_map[] = {
	{ 0 },
};

static bool_t mach_detect(struct machine_t * mach)
{
	return TRUE;
}

static bool_t mach_memmap(struct machine_t * mach)
{
	return TRUE;
}

static bool_t mach_shutdown(struct machine_t * mach)
{
	return FALSE;
}

static bool_t mach_reboot(struct machine_t * mach)
{
	virtual_addr_t regbase = phys_to_virt(BCM2836_PM_BASE);
	u32_t val;

	val = read32(regbase + PM_RSTC);
	val &= ~0x30;
	val |= 0x20;
	write32(regbase + PM_WDOG, (0x5a << 24) | 1);
	write32(regbase + PM_RSTC, (0x5a << 24) | val);
	return TRUE;
}

static bool_t mach_sleep(struct machine_t * mach)
{
	return FALSE;
}

static bool_t mach_cleanup(struct machine_t * mach)
{
	return TRUE;
}

static const char * mach_uniqueid(struct machine_t * mach)
{
	return NULL;
}

static int mach_keygen(struct machine_t * mach, const char * msg, void * key)
{
	return 0;
}

static struct machine_t rpi2 = {
	.name 		= "raspberry-pi-2-b",
	.desc 		= "Raspberry Pi 2 Module B",
	.map		= mach_map,
	.detect 	= mach_detect,
	.memmap		= mach_memmap,
	.shutdown	= mach_shutdown,
	.reboot		= mach_reboot,
	.sleep		= mach_sleep,
	.cleanup	= mach_cleanup,
	.uniqueid	= mach_uniqueid,
	.keygen		= mach_keygen,
};

static __init void mach_rpi2_init(void)
{
	register_machine(&rpi2);
}
machine_initcall(mach_rpi2_init);
