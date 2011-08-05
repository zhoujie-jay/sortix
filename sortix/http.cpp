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

	http.cpp
	A very small http server running in kernel mode, heh!

******************************************************************************/

#include "platform.h"
#include <libmaxsi/string.h>
#include <libmaxsi/memory.h>
#include "uart.h"

using namespace Maxsi;

namespace Sortix
{
	namespace HTTP
	{
		uint32_t Left;

		bool TryByte(char* Dest)
		{
			if ( Left == 0 )
			{
				UART::Read((uint8_t*) &Left, sizeof(Left));
			}

			if ( Left == 0 ) { return false; }

			UART::Read((uint8_t*) Dest, 1);
			Left--;

			return true;
		}

		void Close()
		{
			uint32_t ToSend = 0;
			UART::Write(&ToSend, sizeof(ToSend));
		}

		void Send(const char* Response, uint32_t Len)
		{
			UART::Write(&Len, sizeof(Len));
			UART::Write(Response, Len);
		}

		void Respond(const char* Body, uint32_t Len)
		{
				const char* Headers = "HTTP/1.1 200 Success\r\nConnection: close\r\nContent-Type: application/xhtml+xml\r\n\r\n";
				Send(Headers, String::Length(Headers));
				Send(Body, Len);
				Close();
		}

		void HandleResource(const char* Operation, const char* Resource)
		{
			if ( String::Compare(Resource, "/") == 0 )
			{
				const char* Response =
					"<!DOCTYPE html>\n"
					"<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" xml:lang=\"en\">\n"
					"	<head>\n"
					"		<title>Hello, World!</title>\n"
					"	</head>\n"
					"	<body>\n"
					"		<h1>\"Hello, World!\" said Sortix with a big smile!</h1>\n"
					"		<p>\"Finally\", Sortix continued, \"someone completed a working HTTP server that runs under Sortix, which runs as a virtual machine, that communicates over a serial terminal driver to proxy server that communicates with the real internet!\".</p>\n"
					"		<p>Sortix was happy.</p>\n"
					"		<p>This website was served by a server running under my very own operating system, which is connected to the internet through a serial modem driver. Heh!</p>\n"
					"	</body>\n"
					"</html>\n"
				;

				Respond(Response, String::Length(Response)); return;
			}
			else
			{
				const char* Response = "HTTP/1.1 400 Not Found\r\nConnection: close\r\n\r\nHTTP 404 File Not Found\n";
				Send(Response, String::Length(Response)); Close(); return;
			}
		}

		void HandleConnection()
		{
			char Buffer[1024];

			char* Operation = NULL;
			char* Resource = NULL;

			size_t Used = 0;

			nat State = 0;

			while ( true )
			{
				if ( !TryByte(Buffer + Used) ) { return; }
				Used++;
				Buffer[Used] = '\0';				

				if ( State == 0 )
				{
					if ( Used >= 4 )
					{
						if ( Buffer[0] == 'G' && Buffer[1] == 'E' && Buffer[2] == 'T' && Buffer[3] == ' ' )
						{
							State++;
							Buffer[3] = '\0';
							Operation = Buffer;
						}
						else
						{
							const char* Response = "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n\r\nThis request is not supported by this server!\n";
							Send(Response, String::Length(Response)); Close(); return;
						}
					}
				}
				if ( State == 1 )
				{
					if ( Buffer[Used-1] == ' ' )
					{
						Resource = Buffer + 4;
						Buffer[Used-1] = '\0';
						State++;
						break;
					}
				}

				if ( Used == 1024 )
				{
					const char* Response = "HTTP/1.1 400 Bad Request\r\n\r\n";
					Send(Response, String::Length(Response)); Close(); return;
				}
			}

			nat LineChars = 0;

			while ( LineChars < 4 )
			{
				char TMP;
				if ( !TryByte(&TMP) ) { return; }

				if ( TMP == '\r' || TMP == '\n' )
				{
					LineChars++;
				}
				else
				{
					LineChars = 0;
				}
			}

			HandleResource(Operation, Resource);
		}

		void Init()
		{
			Left = 0;

			while ( true )
			{
				HandleConnection();
			}
		}
	}
}
