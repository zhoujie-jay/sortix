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
 * x86-family/pic.h
 * Driver for the Programmable Interrupt Controller.
 */

#ifndef SORTIX_X86_FAMILY_PIC_H
#define SORTIX_X86_FAMILY_PIC_H

namespace Sortix {
namespace PIC {

uint16_t ReadIRR();
uint16_t ReadISR();
bool IsSpuriousIRQ(unsigned int irq);
extern "C" void ReprogramPIC();
extern "C" void DeprogramPIC();
void SendMasterEOI();
void SendSlaveEOI();
void SendEOI(unsigned int irq);

} // namespace PIC
} // namespace Sortix

#endif
