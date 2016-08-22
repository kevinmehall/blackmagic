/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2015  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "general.h"
#include "target.h"

#define PAC52_PACIDR 0x00100044
#define PAC52_PACIDR_PAC5223 0xff005223
#define PAC52_PACIDR_MASK    0xff00ffff

#define PAC52_FLASHLOCK 0x40020000
#define PAC52_FLASHLOCK_RESET 0
#define PAC52_FLASHLOCK_ALLOW_WRITE 0xAAAAAAAA

#define PAC52_FLASHSTATUS 0x40020004
#define PAC52_FLASHSTATUS_PERASE 0x02
#define PAC52_FLASHSTATUS_WRITE 0x01

#define PAC52_FLASHPAGE 0x40020008
#define PAC52_FLASHPERASE 0x40020014
#define PAC52_FLASHPERASE_START 0xA5A55A5A

#define PAC5223_PAGESIZE 1024

static int pac52xx_flash_erase(struct target_flash *f, uint32_t addr, size_t len);
static int pac52xx_flash_write(struct target_flash *f,
                            uint32_t dest, const void *src, size_t len);

static void pac52xx_add_flash(target *t,
                           uint32_t addr, size_t length, size_t erasesize)
{
	struct target_flash *f = calloc(1, sizeof(*f));
	f->start = addr;
	f->length = length;
	f->blocksize = erasesize;
	f->erase = pac52xx_flash_erase;
	f->write = pac52xx_flash_write;
	f->align = 2;
	f->erased = 0xff;
	target_add_flash(t, f);
}

bool pac52xx_probe(target *t)
{
	uint32_t id = target_mem_read32(t, PAC52_PACIDR);
	switch (id & PAC52_PACIDR_MASK) {
	case PAC52_PACIDR_PAC5223:
		t->driver = "PAC5223";
		target_add_ram(t, 0x20000000, 0x2000);
		pac52xx_add_flash(t, 0x00000000, 0x80000, 0x400);
		return true;
	}
	return false;
}

static int pac52xx_flash_erase(struct target_flash *f, uint32_t addr, size_t len)
{
	while (len) {
		target_mem_write32(f->t, PAC52_FLASHPAGE, addr / PAC5223_PAGESIZE);
		target_mem_write32(f->t, PAC52_FLASHPERASE, PAC52_FLASHPERASE_START);
		
		uint32_t status;
		do {
				status = target_mem_read32(f->t, PAC52_FLASHSTATUS);
		} while (status != 0);
		
		len -= PAC5223_PAGESIZE;
		addr += PAC5223_PAGESIZE;
	}
	return 0;
}

static int pac52xx_flash_write(struct target_flash *f,
                            uint32_t dest, const void *src, size_t len)
{
	target_mem_write32(f->t, PAC52_FLASHLOCK, PAC52_FLASHLOCK_ALLOW_WRITE);
	while (len) {
		target_mem_write16(f->t, dest, *(uint16_t*)src);
		len -= 2;
		dest += 2;
		src += 2;
	}
	target_mem_write32(f->t, PAC52_FLASHLOCK, PAC52_FLASHLOCK_RESET);
	return 0;
}
