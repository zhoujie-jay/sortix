/*
 * Copyright (c) 2011, 2012, 2013, 2014 Jonas 'Sortie' Termansen.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * x86-family/pic.cpp
 * Driver for the Programmable Interrupt Controller.
 */

#include <stdint.h>

#include <sortix/kernel/cpu.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kernel.h>

#include "pic.h"

namespace Sortix {
namespace PIC {

const uint16_t PIC_MASTER = 0x20;
const uint16_t PIC_SLAVE = 0xA0;
const uint16_t PIC_COMMAND = 0x00;
const uint16_t PIC_DATA = 0x01;
const uint8_t PIC_CMD_ENDINTR = 0x20;
const uint8_t PIC_ICW1_ICW4 = 0x01; // ICW4 (not) needed
const uint8_t PIC_ICW1_SINGLE = 0x02; // Single (cascade) mode
const uint8_t PIC_ICW1_INTERVAL4 = 0x04; // Call address interval 4 (8)
const uint8_t PIC_ICW1_LEVEL = 0x08; // Level triggered (edge) mode
const uint8_t PIC_CMD_INIT = 0x10;
const uint8_t PIC_MODE_8086 = 0x01; // 8086/88 (MCS-80/85) mode
const uint8_t PIC_MODE_AUTO = 0x02; // Auto (normal) EOI
const uint8_t PIC_MODE_BUF_SLAVE = 0x08; // Buffered mode/slave
const uint8_t PIC_MODE_BUF_MASTER = 0x0C; // Buffered mode/master
const uint8_t PIC_MODE_SFNM = 0x10; // Special fully nested (not)
const uint8_t PIC_READ_IRR = 0x0A;
const uint8_t PIC_READ_ISR = 0x0B;

static uint16_t ReadRegister(uint8_t ocw3)
{
	outport8(PIC_MASTER + PIC_COMMAND, ocw3);
	outport8(PIC_SLAVE + PIC_COMMAND, ocw3);
	return inport8(PIC_MASTER + PIC_COMMAND) << 0 |
	       inport8(PIC_SLAVE  + PIC_COMMAND) << 8;
}

uint16_t ReadIRR()
{
	return ReadRegister(PIC_READ_IRR);
}

uint16_t ReadISR()
{
	return ReadRegister(PIC_READ_ISR);
}

extern "C" void ReprogramPIC()
{
	uint8_t master_mask = 0;
	uint8_t slave_mask = 0;
	outport8(PIC_MASTER + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	outport8(PIC_SLAVE + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	outport8(PIC_MASTER + PIC_DATA, Interrupt::IRQ0);
	outport8(PIC_SLAVE + PIC_DATA, Interrupt::IRQ8);
	outport8(PIC_MASTER + PIC_DATA, 0x04); // Slave PIC at IRQ2
	outport8(PIC_SLAVE + PIC_DATA, 0x02); // Cascade Identity
	outport8(PIC_MASTER + PIC_DATA, PIC_MODE_8086);
	outport8(PIC_SLAVE + PIC_DATA, PIC_MODE_8086);
	outport8(PIC_MASTER + PIC_DATA, master_mask);
	outport8(PIC_SLAVE + PIC_DATA, slave_mask);
}

extern "C" void DeprogramPIC()
{
	uint8_t master_mask = 0;
	uint8_t slave_mask = 0;
	outport8(PIC_MASTER + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	outport8(PIC_SLAVE + PIC_COMMAND, PIC_CMD_INIT | PIC_ICW1_ICW4);
	outport8(PIC_MASTER + PIC_DATA, 0x08);
	outport8(PIC_SLAVE + PIC_DATA, 0x70);
	outport8(PIC_MASTER + PIC_DATA, 0x04); // Slave PIC at IRQ2
	outport8(PIC_SLAVE + PIC_DATA, 0x02); // Cascade Identity
	outport8(PIC_MASTER + PIC_DATA, PIC_MODE_8086);
	outport8(PIC_SLAVE + PIC_DATA, PIC_MODE_8086);
	outport8(PIC_MASTER + PIC_DATA, master_mask);
	outport8(PIC_SLAVE + PIC_DATA, slave_mask);
}

void SendMasterEOI()
{
	outport8(PIC_MASTER, PIC_CMD_ENDINTR);
}

void SendSlaveEOI()
{
	outport8(PIC_SLAVE, PIC_CMD_ENDINTR);
}

void SendEOI(unsigned int irq)
{
	if ( 8 <= irq )
		SendSlaveEOI();
	SendMasterEOI();
}

} // namespace PIC
} // namespace Sortix
