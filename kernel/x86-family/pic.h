/*******************************************************************************

    Copyright(C) Jonas 'Sortie' Termansen 2011, 2012, 2013, 2014.

    This file is part of Sortix.

    Sortix is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    You should have received a copy of the GNU General Public License along with
    Sortix. If not, see <http://www.gnu.org/licenses/>.

    x86-family/pic.h
    Driver for the Programmable Interrupt Controller.

*******************************************************************************/

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
