/*
 * Copyright (c) 2015 Jonas 'Sortie' Termansen.
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
 * x86-family/ps2.cpp
 * 8042 PS/2 Controller.
 */

#include <assert.h>
#include <stdint.h>

#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/ioport.h>
#include <sortix/kernel/kthread.h>
#include <sortix/kernel/log.h>
#include <sortix/kernel/panic.h>
#include <sortix/kernel/ps2.h>

#include "ps2.h"

namespace Sortix {
namespace PS2 {

static const uint16_t REG_DATA    = 0x0060;
static const uint16_t REG_COMMAND = 0x0064;
static const uint16_t REG_STATUS  = 0x0064;

static const uint8_t REG_COMMAND_READ_RAM            = 0x20;
static const uint8_t REG_COMMAND_WRITE_RAM           = 0x60;
static const uint8_t REG_COMMAND_DISABLE_SECOND_PORT = 0xA7;
static const uint8_t REG_COMMAND_ENABLE_SECOND_PORT  = 0xA8;
static const uint8_t REG_COMMAND_TEST_SECOND_PORT    = 0xA9;
static const uint8_t REG_COMMAND_TEST_CONTROLLER     = 0xAA;
static const uint8_t REG_COMMAND_TEST_FIRST_PORT     = 0xAB;
static const uint8_t REG_COMMAND_DISABLE_FIRST_PORT  = 0xAD;
static const uint8_t REG_COMMAND_ENABLE_FIRST_PORT   = 0xAE;
static const uint8_t REG_COMMAND_SEND_TO_PORT_2      = 0xD4;

static const uint8_t REG_STATUS_OUTPUT   = 1 << 0;
static const uint8_t REG_STATUS_INPUT    = 1 << 1;
static const uint8_t REG_STATUS_SYSTEM   = 1 << 2;
static const uint8_t REG_STATUS_COMMAND  = 1 << 3;
static const uint8_t REG_STATUS_UNKNOWN1 = 1 << 4;
static const uint8_t REG_STATUS_UNKNOWN2 = 1 << 5;
static const uint8_t REG_STATUS_TIMEOUT  = 1 << 6;
static const uint8_t REG_STATUS_PARITY   = 1 << 7;

static const uint8_t REG_CONFIG_FIRST_INTERRUPT   = 1 << 0;
static const uint8_t REG_CONFIG_SECOND_INTERRUPT  = 1 << 1;
static const uint8_t REG_CONFIG_SYSTEM            = 1 << 2;
static const uint8_t REG_CONFIG_ZERO1             = 1 << 3;
static const uint8_t REG_CONFIG_FIRST_CLOCK       = 1 << 4;
static const uint8_t REG_CONFIG_SECOND_CLOCK      = 1 << 5;
static const uint8_t REG_CONFIG_FIRST_TRANSLATION = 1 << 6;
static const uint8_t REG_CONFIG_ZERO2             = 1 << 7;

static const uint8_t DEVICE_RESET_OK = 0xAA;
static const uint8_t DEVICE_ECHO = 0xEE;
static const uint8_t DEVICE_ACK = 0xFA;
static const uint8_t DEVICE_RESEND = 0xFE;
static const uint8_t DEVICE_ERROR = 0xFF;

static const uint8_t DEVICE_CMD_ENABLE_SCAN = 0xF4;
static const uint8_t DEVICE_CMD_DISABLE_SCAN = 0xF5;
static const uint8_t DEVICE_CMD_IDENTIFY = 0xF2;
static const uint8_t DEVICE_CMD_RESET = 0xFF;

static const size_t DEVICE_RETRIES = 5;

// TODO: This is entirely a guess. I don't actually know what timeout is
//       suitable. GRUB seems to use 20 ms. I'll pick 50 ms to be safe.
static const unsigned int TIMEOUT_MS = 50;

static bool WaitInput()
{
	return wait_inport8_clear(REG_STATUS, REG_STATUS_INPUT, false, TIMEOUT_MS);
}

static bool WaitOutput()
{
	return wait_inport8_set(REG_STATUS, REG_STATUS_OUTPUT, false, TIMEOUT_MS);
}

static bool TryReadByte(uint8_t* result)
{
	if ( !WaitOutput() )
		return false;
	*result = inport8(REG_DATA);
	return true;
}

static bool TryWriteByte(uint8_t byte)
{
	if ( !WaitInput() )
		return false;
	outport8(REG_DATA, byte);
	return true;
}

static bool TryWriteCommand(uint8_t byte)
{
	if ( !WaitInput() )
		return false;
	outport8(REG_COMMAND, byte);
	return true;
}

static bool TryWriteToPort(uint8_t byte, uint8_t port)
{
	if ( port == 2 && !TryWriteCommand(REG_COMMAND_SEND_TO_PORT_2) )
		return false;
	return TryWriteByte(byte);
}

static bool TryWriteCommandToPort(uint8_t command, uint8_t port, uint8_t* answer)
{
	*answer = DEVICE_ERROR;
	size_t unrelated = 0;
	for ( size_t i = 0; i < DEVICE_RETRIES; i++ )
	{
		if ( !TryWriteToPort(command, port) )
			return false;
	again:
		uint8_t byte;
		if ( !TryReadByte(&byte) )
			return false;
		if ( byte == DEVICE_RESET_OK && !TryReadByte(&byte) )
			return false;
		*answer = byte;
		if ( byte == DEVICE_ACK || byte == DEVICE_ECHO )
			return true;
		if ( byte != DEVICE_RESEND )
		{
			// We received a weird response, probably pending data, discard it
			// and hope we receive a real acknowledgement.
			if ( 1000 <= unrelated )
				return false;
			unrelated++;
			goto again;
		}
	}
	return false;
}

static bool IsKeyboardResponse(uint8_t* response, size_t size)
{
	if ( size == 0 )
		return true;
	if ( size == 2 && response[0] == 0xAB && response[1] == 0x41 )
		return true;
	if ( size == 2 && response[0] == 0xAB && response[1] == 0xC1 )
		return true;
	if ( size == 2 && response[0] == 0xAB && response[1] == 0x83 )
		return true;
	return false;
}

static bool IsMouseResponse(uint8_t* response, size_t size)
{
	if ( size == 1 && response[0] == 0x00 )
		return true;
	if ( size == 1 && response[0] == 0x03 )
		return true;
	if ( size == 1 && response[0] == 0x04 )
		return true;
	return false;
}

static struct interrupt_handler irq1_registration;
static struct interrupt_handler irq12_registration;
static PS2Device* irq1_device;
static PS2Device* irq12_device;

static void IRQ1Work(void* ctx, void* payload, size_t size)
{
	(void) ctx;
	assert(size == sizeof(unsigned char));
	unsigned char* byteptr = (unsigned char*) payload;
	unsigned char byte = *byteptr;
	if ( irq1_device )
		irq1_device->PS2DeviceOnByte(byte);
}

static void IRQ12Work(void* ctx, void* payload, size_t size)
{
	(void) ctx;
	assert(size == sizeof(unsigned char));
	unsigned char* byteptr = (unsigned char*) payload;
	unsigned char byte = *byteptr;
	if ( irq12_device )
		irq12_device->PS2DeviceOnByte(byte);
}

static void OnIRQ1(struct interrupt_context* intctx, void* user)
{
	(void) intctx;
	(void) user;
	if ( inport8(REG_STATUS) & REG_STATUS_OUTPUT )
	{
		uint8_t byte = inport8(REG_DATA);
		Interrupt::ScheduleWork(IRQ1Work, NULL, &byte, sizeof(byte));
	}
}

static void OnIRQ12(struct interrupt_context* intctx, void* user)
{
	(void) intctx;
	(void) user;
	if ( inport8(REG_STATUS) & REG_STATUS_OUTPUT )
	{
		uint8_t byte = inport8(REG_DATA);
		Interrupt::ScheduleWork(IRQ12Work, NULL, &byte, sizeof(byte));
	}
}

static kthread_mutex_t ps2_device_send_mutex = KTHREAD_MUTEX_INITIALIZER;

static bool PS2DeviceSend(void* ctx, uint8_t byte)
{
	ScopedLock lock(&ps2_device_send_mutex);
	uint8_t port = (uint8_t) (uintptr_t) ctx;
	return TryWriteToPort(byte, port);
}

static bool DetectDevice(uint8_t port, uint8_t* response, size_t* response_size);

void Init(PS2Device* keyboard, PS2Device* mouse)
{
	uint8_t byte;
	uint8_t config;

	if ( !TryWriteCommand(REG_COMMAND_DISABLE_FIRST_PORT) ||
	     !TryWriteCommand(REG_COMMAND_DISABLE_SECOND_PORT) )
		return;
	while ( inport8(REG_STATUS) & REG_STATUS_OUTPUT )
		inport8(REG_DATA);
	if ( !TryWriteCommand(REG_COMMAND_READ_RAM) ||
	     !TryReadByte(&config) )
		return;
	config &= ~REG_CONFIG_FIRST_INTERRUPT;
	config &= ~REG_CONFIG_SECOND_INTERRUPT;
	//config &= ~REG_CONFIG_FIRST_TRANSLATION; // TODO: Not ready for this yet.
	if ( !TryWriteCommand(REG_COMMAND_WRITE_RAM) ||
	     !TryWriteByte(config) )
		return;
	bool dual = config & REG_CONFIG_SECOND_CLOCK;
	if ( !TryWriteCommand(REG_COMMAND_TEST_CONTROLLER) ||
	     !TryReadByte(&byte) )
		return;
	if ( byte == 0xFF )
		return;
	if ( byte != 0x55 )
	{
		Log::PrintF("[PS/2 controller] Self-test failure resulted in "
		            "0x%02X instead of 0x55\n", byte);
		return;
	}
	if ( dual )
	{
		uint8_t config;
		if ( !TryWriteCommand(REG_COMMAND_ENABLE_SECOND_PORT) ||
		     !TryWriteCommand(REG_COMMAND_READ_RAM) ||
		     !TryReadByte(&config) )
			return;
		dual = !(config & REG_CONFIG_SECOND_CLOCK);
		if ( dual && !TryWriteCommand(REG_COMMAND_DISABLE_SECOND_PORT) )
			return;
	}
	bool port_1 = true;
	bool port_2 = dual;
#if 0 // Disabled due to some emulated PS/2 controllers not handling this well.
	if ( port_1 )
	{
		if ( !TryWriteCommand(REG_COMMAND_TEST_FIRST_PORT) ||
		     !TryReadByte(&byte) )
			return;
		port_1 = byte == 0x00;
	}
	if ( port_2 )
	{
		if ( !TryWriteCommand(REG_COMMAND_TEST_SECOND_PORT) ||
		     !TryReadByte(&byte) )
			return;
		port_2 = byte == 0x00;
	}
#endif
	size_t port_1_resp_size = 0;
	uint8_t port_1_resp[2];
	if ( port_1 && !DetectDevice(1, port_1_resp, &port_1_resp_size) )
		port_1 = false;
	size_t port_2_resp_size = 0;
	uint8_t port_2_resp[2];
	if ( port_2 && !DetectDevice(2, port_2_resp, &port_2_resp_size) )
		port_2 = false;
	if ( port_1 && !irq1_device &&
	     IsKeyboardResponse(port_1_resp, port_1_resp_size) )
		irq1_device = keyboard;
	if ( port_2 && !irq12_device &&
	     IsMouseResponse(port_2_resp, port_2_resp_size) )
		irq12_device = mouse;
	if ( port_1 && !irq1_device &&
	     IsMouseResponse(port_1_resp, port_1_resp_size) )
		irq1_device = mouse;
	if ( port_2 && !irq12_device &&
	     IsKeyboardResponse(port_2_resp, port_2_resp_size) )
		irq12_device = keyboard;
	port_1 = port_1 && irq1_device;
	port_2 = port_2 && irq12_device;
	if ( !TryWriteCommand(REG_COMMAND_READ_RAM) ||
	     !TryReadByte(&config) )
		return;
	irq1_registration.handler = OnIRQ1;
	irq1_registration.context = NULL;
	Interrupt::RegisterHandler(Interrupt::IRQ1, &irq1_registration);
	irq12_registration.handler = OnIRQ12;
	irq12_registration.context = NULL;
	Interrupt::RegisterHandler(Interrupt::IRQ12, &irq12_registration);
	config |= port_1 ? REG_CONFIG_FIRST_INTERRUPT : 0;
	config |= port_2 ? REG_CONFIG_SECOND_INTERRUPT : 0;
	if ( !TryWriteCommand(REG_COMMAND_WRITE_RAM) ||
	     !TryWriteByte(config) )
		return;
	if ( port_1 && !TryWriteCommand(REG_COMMAND_ENABLE_FIRST_PORT) )
		return;
	if ( port_2 && !TryWriteCommand(REG_COMMAND_ENABLE_SECOND_PORT) )
		return;
	if ( irq1_device )
		irq1_device->PS2DeviceInitialize((void*) 1, PS2DeviceSend,
		                                 port_1_resp, port_1_resp_size);
	if ( irq12_device )
		irq12_device->PS2DeviceInitialize((void*) 2, PS2DeviceSend,
		                                  port_2_resp, port_2_resp_size);
}

static bool DetectDevice(uint8_t port, uint8_t* response, size_t* response_size)
{
	uint8_t enable = port == 1 ? REG_COMMAND_ENABLE_FIRST_PORT :
	                             REG_COMMAND_ENABLE_SECOND_PORT;
	uint8_t disable = port == 1 ? REG_COMMAND_DISABLE_FIRST_PORT :
	                              REG_COMMAND_DISABLE_SECOND_PORT;
	uint8_t byte;
	if ( !TryWriteCommand(enable) )
		return false;
	if ( !TryWriteCommandToPort(DEVICE_CMD_DISABLE_SCAN, port, &byte) )
	{
		if ( byte == DEVICE_RESEND )
		{
			// HARDWARE BUG:
			// This may be incomplete PS/2 emulation that simulates the
			// controller but the devices always responds with 0xFE to anything
			// they receive. This happens on sortie's pc1, but the keyboard
			// device still supplies IRQ1's and scancodes. Let's assume the
			// devices are stil there even though we can't control them.
			if ( port == 1 )
			{
				*response_size = 2;
				response[0] = 0xAB;
				response[1] = 0x83;
				if ( !TryWriteCommand(disable) )
					return false;
				return true;
			}
			if ( port == 2 )
			{
				*response_size = 1;
				response[0] = 0x00;
				if ( !TryWriteCommand(disable) )
					return false;
				return true;
			}
		}
		TryWriteCommand(disable);
		return false;
	}
	// Empty pending buffer.
	while ( TryReadByte(&byte) )
		continue;
	if ( !TryWriteCommandToPort(DEVICE_CMD_IDENTIFY, port, &byte) )
	{
		TryWriteCommand(disable);
		return false;
	}
	*response_size = 0;
	if ( TryReadByte(&byte) )
	{
		response[(*response_size)++] = byte;
		if ( TryReadByte(&byte) )
			response[(*response_size)++] = byte;
	}
	if ( !TryWriteCommand(disable) )
		return false;
	return true;
}

} // namespace PS2
} // namespace Sortix
