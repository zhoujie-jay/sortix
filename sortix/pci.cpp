/******************************************************************************

	COPYRIGHT(C) JONAS 'SORTIE' TERMANSEN 2011.

	This file is part of Sortix.

	Sortix is free software: you can redistribute it and/or modify it under the
	terms of the GNU General Public License as published by the Free Software
	Foundation, either version 3 of the License, or (at your option) any later
	version.

	Sortix is distributed in the hope that it will be useful, but WITHOUT ANY
	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
	details.

	You should have received a copy of the GNU General Public License along
	with Sortix. If not, see <http://www.gnu.org/licenses/>.

	pci.h
	Handles basic PCI bus stuff.

******************************************************************************/

#include "platform.h"
#include "pci.h"
#include "log.h"

namespace Sortix
{
	namespace PCI
	{
		const uint16_t Config_Address = 0xCF8;
		const uint16_t Config_Data = 0xCFC;
	
		uint32_t SwapBytes(uint32_t I)
		{
			return (I >> 24) | ((I >> 8) & 0x0000FF00) | ((I << 8) & 0x00FF0000) | (I << 24);
		}

		const char* ToDeviceDesc(uint32_t ProductInfo, uint32_t DeviceType)
		{
			uint32_t Class			=	(DeviceType) >> 24;
			uint32_t SubClass		=	(DeviceType >> 16) & 0xFF;
			uint32_t ProgIF			=	(DeviceType >> 8) & 0xFF;
			uint32_t RevisionID		=	(DeviceType) & 0xFF;

			if ( Class == 0x00 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Any device except for VGA-Compatible devices"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "VGA-Compatible Device"; }
			}
			if ( Class == 0x01 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "SCSI Bus Controller"; }
				if ( SubClass == 0x01 ) { return "IDE Controller"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "Floppy Disk Controller"; }
				if ( SubClass == 0x03 && ProgIF == 0x00 ) { return "IPI Bus Controller"; }
				if ( SubClass == 0x04 && ProgIF == 0x00 ) { return "RAID Controller"; }
				if ( SubClass == 0x05 && ProgIF == 0x20 ) { return "ATA Controller (Single DMA)"; }
				if ( SubClass == 0x05 && ProgIF == 0x30 ) { return "ATA Controller (Chained DMA)"; }
				if ( SubClass == 0x06 && ProgIF == 0x00 ) { return "Serial ATA (Direct Port Access)"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Mass Storage Controller"; }
			}
			if ( Class == 0x02 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Ethernet Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "Token Ring Controller"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "FDDI Controller"; }
				if ( SubClass == 0x03 && ProgIF == 0x00 ) { return "ATM Controller"; }
				if ( SubClass == 0x04 && ProgIF == 0x00 ) { return "ISDN Controller"; }
				if ( SubClass == 0x05 && ProgIF == 0x00 ) { return "WorldFip Controller"; }
				if ( SubClass == 0x05 && ProgIF == 0x00 ) { return "ATA Controller (Chained DMA)"; }
				if ( SubClass == 0x06 ) { return "PICMG 2.14 Multi Computing"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Network Controller"; }
			}
			if ( Class == 0x03 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "VGA-Compatible Controller"; }
				if ( SubClass == 0x00 && ProgIF == 0x01 ) { return "8512-Compatible Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "XGA Controller"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "3D Controller (Not VGA-Compatible)"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Display Controller"; }
			}
			if ( Class == 0x04 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Video Device"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "Audio Device"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "Computer Telephony Device"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Multimedia Device"; }
			}
			if ( Class == 0x05 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "RAM Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "Flash Controller"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Memory Controller"; }
			}
			if ( Class == 0x06 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Host Bridge"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "ISA Bridge"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "EISA Bridge"; }
				if ( SubClass == 0x03 && ProgIF == 0x00 ) { return "MCA Bridge"; }
				if ( SubClass == 0x04 && ProgIF == 0x00 ) { return "PCI-to-PCI Bridge"; }
				if ( SubClass == 0x04 && ProgIF == 0x01 ) { return "PCI-to-PCI Bridge (Subtractive Decode)"; }
				if ( SubClass == 0x05 && ProgIF == 0x00 ) { return "PCMCIA Bridge"; }
				if ( SubClass == 0x06 && ProgIF == 0x00 ) { return "NuBus Bridge"; }
				if ( SubClass == 0x07 && ProgIF == 0x00 ) { return "CardBus Bridge"; }
				if ( SubClass == 0x08 ) { return "RACEway Bridge"; }
				if ( SubClass == 0x09 && ProgIF == 0x40 ) { return "PCI-to-PCI Bridge (Semi-Transparent, Primary)"; }
				if ( SubClass == 0x09 && ProgIF == 0x80 ) { return "PCI-to-PCI Bridge (Semi-Transparent, Secondary)"; }
				if ( SubClass == 0x0A && ProgIF == 0x00 ) { return "InfiniBrand-to-PCI Host Bridge"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Bridge Device"; }
			}
			if ( Class == 0x07 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Generic XT-Compatible Serial Controller"; }
				if ( SubClass == 0x00 && ProgIF == 0x01 ) { return "16450-Compatible Serial Controller"; }
				if ( SubClass == 0x00 && ProgIF == 0x02 ) { return "16550-Compatible Serial Controller"; }
				if ( SubClass == 0x00 && ProgIF == 0x03 ) { return "16650-Compatible Serial Controller"; }
				if ( SubClass == 0x00 && ProgIF == 0x04 ) { return "16750-Compatible Serial Controller"; }
				if ( SubClass == 0x00 && ProgIF == 0x05 ) { return "16850-Compatible Serial Controller"; }
				if ( SubClass == 0x00 && ProgIF == 0x06 ) { return "16950-Compatible Serial Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "Parallel Port"; }
				if ( SubClass == 0x01 && ProgIF == 0x01 ) { return "Bi-Directional Parallel Port"; }
				if ( SubClass == 0x01 && ProgIF == 0x02 ) { return "ECP 1.X Compliant Parallel Port"; }
				if ( SubClass == 0x01 && ProgIF == 0x03 ) { return "IEEE 1284 Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0xFE ) { return "IEEE 1284 Target Device"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "Multiport Serial Controller"; }
				if ( SubClass == 0x03 && ProgIF == 0x00 ) { return "Generic Modem"; }
				if ( SubClass == 0x03 && ProgIF == 0x01 ) { return "Hayes Compatible Modem (16450-Compatible Interface)"; }
				if ( SubClass == 0x03 && ProgIF == 0x02 ) { return "Hayes Compatible Modem (16550-Compatible Interface)"; }
				if ( SubClass == 0x03 && ProgIF == 0x03 ) { return "Hayes Compatible Modem (16650-Compatible Interface)"; }
				if ( SubClass == 0x03 && ProgIF == 0x04 ) { return "Hayes Compatible Modem (16750-Compatible Interface)"; }
				if ( SubClass == 0x04 && ProgIF == 0x00 ) { return "IEEE 488.1/2 (GPIB) Controller"; }
				if ( SubClass == 0x05 && ProgIF == 0x00 ) { return "Smart Card"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Communications Device"; }
			}
			if ( Class == 0x08 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Generic 8259 PIC"; }
				if ( SubClass == 0x00 && ProgIF == 0x01 ) { return "ISA PIC"; }
				if ( SubClass == 0x00 && ProgIF == 0x02 ) { return "EISA PIC"; }
				if ( SubClass == 0x00 && ProgIF == 0x10 ) { return "I/O APIC Interrupt Controller"; }
				if ( SubClass == 0x00 && ProgIF == 0x20 ) { return "I/O(x) APIC Interrupt Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "Generic 8237 DMA Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0x01 ) { return "ISA DMA Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0x02 ) { return "EISA DMA Controller"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "Generic 8254 System Timer"; }
				if ( SubClass == 0x02 && ProgIF == 0x01 ) { return "ISA System Timer"; }
				if ( SubClass == 0x02 && ProgIF == 0x02 ) { return "EISA System Timer"; }
				if ( SubClass == 0x03 && ProgIF == 0x00 ) { return "Generic RTC Controller"; }
				if ( SubClass == 0x03 && ProgIF == 0x01 ) { return "ISA RTC Controller"; }
				if ( SubClass == 0x04 && ProgIF == 0x01 ) { return "Generic PCI Hot-Plug Controller"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other System Peripheral"; }
			}
			if ( Class == 0x09 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Keyboard Controller"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "Digitizer"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "Mouse Controller"; }
				if ( SubClass == 0x03 && ProgIF == 0x00 ) { return "Scanner Controller"; }
				if ( SubClass == 0x04 && ProgIF == 0x00 ) { return "Gameport Controller (Generic)"; }
				if ( SubClass == 0x04 && ProgIF == 0x10 ) { return "Gameport Controller (Legacy)"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Input Controller"; }
			}
			if ( Class == 0x0A )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Generic Docking Station"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Docking Station"; }
			}
			if ( Class == 0x0B )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "386 Processor"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "486 Processor"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "Pentium Processor"; }
				if ( SubClass == 0x10 && ProgIF == 0x00 ) { return "Alpha Processor"; }
				if ( SubClass == 0x20 && ProgIF == 0x00 ) { return "PowerPC Processor"; }
				if ( SubClass == 0x30 && ProgIF == 0x00 ) { return "MIPS Processor"; }
				if ( SubClass == 0x40 && ProgIF == 0x00 ) { return "Co-Processor"; }
			}
			if ( Class == 0x0C )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "IEEE 1394 Controller (FireWire)"; }
				if ( SubClass == 0x00 && ProgIF == 0x10 ) { return "IEEE 1394 Controller (1394 OpenHCI Spec)"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "ACCESS.bus"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "SSA"; }
				if ( SubClass == 0x03 && ProgIF == 0x00 ) { return "USB (Universal Host Controller Spec)"; }
				if ( SubClass == 0x03 && ProgIF == 0x10 ) { return "USB (Open Host Controller Spec)"; }
				if ( SubClass == 0x03 && ProgIF == 0x20 ) { return "USB2 Host Controller (Intel Enhanced Host Controller Interface)"; }
				if ( SubClass == 0x03 && ProgIF == 0x80 ) { return "USB"; }
				if ( SubClass == 0x03 && ProgIF == 0xFE ) { return "USB (Not Host Controller)"; }
				if ( SubClass == 0x04 && ProgIF == 0x00 ) { return "Fibre Channel"; }
				if ( SubClass == 0x05 && ProgIF == 0x00 ) { return "SMBus"; }
				if ( SubClass == 0x06 && ProgIF == 0x00 ) { return "InfiniBand"; }
				if ( SubClass == 0x07 && ProgIF == 0x00 ) { return "IPMI SMIC Interface"; }
				if ( SubClass == 0x07 && ProgIF == 0x01 ) { return "IPMI Kybd Controller Style Interface"; }
				if ( SubClass == 0x07 && ProgIF == 0x02 ) { return "IPMI Block Transfer Interface"; }
				if ( SubClass == 0x08 && ProgIF == 0x02 ) { return "SERCOS Interface Standard (IEC 61491)"; }
				if ( SubClass == 0x09 && ProgIF == 0x00 ) { return "CANbus"; }
			}
			if ( Class == 0x0E )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Message FIFO"; }
				if ( SubClass == 0x00 ) { return "I20 Architecture"; }
			}
			if ( Class == 0x0F )
			{
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "TV Controller"; }
				if ( SubClass == 0x02 && ProgIF == 0x00 ) { return "Audio Controller"; }
				if ( SubClass == 0x03 && ProgIF == 0x00 ) { return "Voice Controller"; }
				if ( SubClass == 0x04 && ProgIF == 0x00 ) { return "Data Controller"; }
			}
			if ( Class == 0x10 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "Network and Computing Encryption/Decryption"; }
				if ( SubClass == 0x10 && ProgIF == 0x00 ) { return "Entertainment Encryption/Decryption"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Encryption/Decryption"; }
			}
			if ( Class == 0x11 )
			{
				if ( SubClass == 0x00 && ProgIF == 0x00 ) { return "DPIO Modules"; }
				if ( SubClass == 0x01 && ProgIF == 0x00 ) { return "Performance Counters"; }
				if ( SubClass == 0x10 && ProgIF == 0x00 ) { return "Communications Syncrhonization Plus Time and Frequency Test/Measurment"; }
				if ( SubClass == 0x20 && ProgIF == 0x00 ) { return "Management Card"; }
				if ( SubClass == 0x80 && ProgIF == 0x00 ) { return "Other Data Acquisition/Signal Processing Controller"; }
			}

			return NULL;
		}

		void Init()
		{
#if 0
			Log::Print("PCI Devices: ");

			for ( nat Bus = 0; Bus < 256; Bus++ )
			{
				for ( nat Slot = 0; Slot < 32; Slot++ )
				{
					for ( nat Function = 0; Function < 8; Function++ )
					{
						uint32_t ProductInfo = CheckDevice(Bus, Slot, Function);

						if ( ProductInfo == 0xFFFFFFFF ) { continue; }

						uint32_t DeviceType = ReadLong(Bus, Slot, Function, 0x08);

						const char* DeviceDesc = ToDeviceDesc(ProductInfo, DeviceType);

						if ( DeviceDesc != NULL )
						{
							Log::PrintF("%s, ", DeviceDesc);
						}
						else
						{
							Log::PrintF("Unknown PCI Device @ %x:%x.%x (ProductInfo=0x%x, DeviceType=0x%x), ", Bus, Slot, Function, ProductInfo, DeviceType);
						}
					}
				}
			}

			Log::Print("\b\b\n");
#endif
		}

		uint32_t ReadLong(uint8_t Bus, uint8_t Slot, uint8_t Function, uint8_t Offset)
		{
			unsigned long LBus = (unsigned long) Bus;
			unsigned long LSlot = (unsigned long) Slot;
			unsigned long LFunc = (unsigned long) Function;

			// create configuration address.
			unsigned long Address = (unsigned long) ( (LBus << 16) | (LSlot << 11) | (LFunc << 8) | (Offset & 0xFC) | ((uint32_t) 0x80000000));

			// Write out the address.
			CPU::OutPortL(Config_Address, Address);

			// Read in the data.
			return CPU::InPortL(Config_Data);

		}

		uint32_t CheckDevice(uint8_t Bus, uint8_t Slot, uint8_t Function)
		{
			return ReadLong(Bus, Slot, Function, 0);
		}
	}
}

