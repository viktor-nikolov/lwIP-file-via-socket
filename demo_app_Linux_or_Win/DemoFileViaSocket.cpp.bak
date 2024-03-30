/*
This is the demo app of using a C++ ostream class for writing a file on a remote system via an IP socket connection.
Details are explained on GitHub: https://github.com/viktor-nikolov/lwIP-file-via-socket

Tested (and ready for compilation) on Windows 11 (MinGW toolchain) and Ubuntu 22.04 (gcc toolchain).

BSD 2-Clause License:

Copyright (c) 2024 Viktor Nikolov

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <chrono>
#include <thread>
#include <iostream>
#include "FileViaSocket.h"

#ifdef __WIN32__
#   include <winsock2.h>
#endif

const unsigned short SERVER_PORT{ 65432 }; // The server script file_via_socket.py uses port 65432 by default.

int main( int argc, char* argv[] )
{
#ifdef __WIN32__
	// Initiate use of the Winsock DLL
	WORD versionWanted = MAKEWORD(1, 1);
	WSADATA wsaData;
	WSAStartup(versionWanted, &wsaData);
	int WSAresult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if( WSAresult != 0 ) {
		std::cerr << "WSAStartup failed: " << WSAresult << std::endl;
		return 1;
	}
#endif

	std::string ServerAddress;

	// Check if at least one command line argument is provided (excluding the program name)
	if( argc > 1 )
		ServerAddress = argv[1];
	else {
		std::cerr << "Error: No server address provided as command line parameter." << std::endl
		          << "       Provide an IP address in numerical format (e.g. 192.168.44.44)." << std::endl;
		return 1; // Return a non-zero value to indicate error
	}

	try {
		FileViaSocket f( ServerAddress, SERVER_PORT ); // Declare the object and open the connection
		f << "Hello world!\n"; /* We are using '\n' on purpose instead of std::endl, because
		                        * std::endl has a side effect of flushing the buffer, i.e.,
		                        * "Hello world!\n" would be immediatelly sent in a TCP packet. */
		f << "I'm here.\n";
		f << std::flush;       /* We are explicitly flushing the buffer, "Hello world!\nI'm here.\n"
		                        * is sent in a TCP packet. */
		f << "It worked.\n";

	} // Object f ceases to exist, destructor on 'f' is called, buffer is flushed,
	  // "It worked.\n" is sent in a TCP packet, the socket connection is closed, a file is created on the server
	catch( const std::exception& e ) {
        std::cerr << "Error on opening the socket: " << std::endl << e.what() << std::endl;
        return 1;
    }
	std::cout << "\"Hello world\" sent" << std::endl;

	/* We must give the server some time to close the connection on its end.
	   Otherwise, the next call of 'open' would fail because of refused connection. */
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	
	FileViaSocket f; // We just declare the object, no connection is made

	try {
		f.open( ServerAddress, SERVER_PORT ); // Open connection to the server
	} catch( const std::exception& e ) {
        std::cerr << "Error on opening the socket: " << std::endl << e.what() << std::endl;
        return 1;
    }	

	f << '1' << "23456" << 78; // We can write all kinds of data types to an ostream
	f.close(); // Close the connection, another file is created on the server
	std::cout << "\"12345678\" sent" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	try {
		f.open( ServerAddress, SERVER_PORT ); // Open a new connection on the same object
	} catch( const std::exception& e ) {
        std::cerr << "Error on opening the socket: " << std::endl << e.what() << std::endl;
        return 1;
    }	

	// Prepare a buffer to be written to the server
	const unsigned BUFF_SIZE{ 26*1000 };   /* Size of the buffer sent by one call of write() */
	const unsigned BUFFER_COUNT{ 1000 };   /* Number of times we sent the buffer.
	                                          Set this to a high number to perform bulk transfer test. */
	char buffer[BUFF_SIZE];
	for( unsigned i = 0; i < BUFF_SIZE; i++ )
		buffer[i] = 'A' + i % 26; // Fill with repeated sequence from 'A' to 'Z'

	for( unsigned i = 0; i < BUFFER_COUNT; i++ )
		f.write( buffer, BUFF_SIZE ); // Write the whole buffer to the server
	f.close(); // The connection is closed, third file is created on the server
	std::cout << "Buffer sent. All done." << std::endl;

	return 0;
} // main
