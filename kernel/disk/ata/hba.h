/*
 * Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Jonas 'Sortie' Termansen.
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
 * disk/ata/hba.h
 * Driver for ATA.
 */

#ifndef SORTIX_DISK_ATA_HBA_H
#define SORTIX_DISK_ATA_HBA_H

#include <stdint.h>

#include <sortix/kernel/descriptor.h>
#include <sortix/kernel/interrupt.h>
#include <sortix/kernel/kthread.h>

namespace Sortix {
namespace ATA {

class HBA;
class Port;

class Channel
{
	friend class Port;
	friend class HBA;

public:
	Channel();
	~Channel();

public:
	bool Initialize(Ref<Descriptor> dev, const char* devpath);
	void OnInterrupt();

private:
	__attribute__((format(printf, 2, 3)))
	void LogF(const char* format, ...);
	void SelectDrive(unsigned int drive_index);

private:
	struct interrupt_handler interrupt_registration;
	kthread_mutex_t hw_lock;
	uint32_t devaddr;
	HBA* hba;
	unsigned int channel_index;
	unsigned int current_drive;
	uint16_t port_base;
	uint16_t port_control;
	uint16_t busmaster_base;
	Port* drives[2];
	unsigned int interrupt_index;
	bool interrupt_registered;

};

class HBA
{
	friend class Port;

public:
	HBA(uint32_t devaddr);
	~HBA();

public:
	bool Initialize(Ref<Descriptor> dev, const char* devpath);

private:
	bool InitializeChannel(Ref<Descriptor> dev, const char* devpath,
	                       unsigned int channel_index);

private:
	uint32_t devaddr;
	Channel channels[2];

};

} // namespace ATA
} // namespace Sortix

#endif
