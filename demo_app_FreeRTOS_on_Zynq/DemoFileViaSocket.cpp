/*
This is the demo app of using a C++ ostream class for writing a file on a remote system via an IP socket connection.
Details are explained on GitHub: https://github.com/viktor-nikolov/lwIP-file-via-socket

This version was tested on FreeRTOS running on AMD Xilinx Zynq (Vitis 2023.1 toolchain).

I derived this FreeRTOS networking demo from this sample project provided by AMD Xilinx:
https://github.com/Xilinx/embeddedsw/blob/master/lib/sw_apps/freertos_lwip_tcp_perf_client/src/main.c

BSD 3-Clause License:

Copyright (c) 2024 Viktor Nikolov
Copyright (c) 2018-2022 Xilinx, Inc.
Copyright (c) 2022-2023 Advanced Micro Devices, Inc.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.
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
#include "xil_printf.h"

#include "netif/xadapter.h"
#include "lwip/init.h"
#include "lwip/inet.h"
#include "lwip/dhcp.h"

#include "FileViaSocket.h"

/* IP address and port of the server running the script file_via_socket.py.
 * The address must be provided in numberical form in a string, e.g., "192.168.44.10".*/
const std::string    SERVER_ADDR( "###SERVER_ADDR is not set###" );
//const std::string    SERVER_ADDR( "192.168.44.10" );
const unsigned short SERVER_PORT{ 65432 }; //The server script file_via_socket.py uses port 65432 by default.

//Fallback IP address used when DHCP is not successful
#define DEFAULT_IP_ADDRESS	"192.168.44.150"
#define DEFAULT_IP_MASK		"255.255.255.0"
#define DEFAULT_GW_ADDRESS	"192.168.44.1"

//Size of the stack (as number of 32bit words) for threads we create:
#define STANDARD_THREAD_STACKSIZE 1024

//Size of the stack (as number of 32bit words) for the thread, which sends data using FileViaSocket class
#define SEND_DATA_THREAD_STACKSIZE 8000

sys_thread_t network_init_thread_handle;
static int complete_nw_thread;
struct netif server_netif;

extern volatile int dhcp_timoutcntr;
err_t dhcp_start(struct netif *netif);

static void print_ip(const char *msg, ip_addr_t *ip)
{
	xil_printf(msg);
	xil_printf("%d.%d.%d.%d\n\r", ip4_addr1(ip), ip4_addr2(ip),
	           ip4_addr3(ip), ip4_addr4(ip));
}

static void print_ip_settings(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	print_ip("Board IP:       ", ip);
	print_ip("Netmask :       ", mask);
	print_ip("Gateway :       ", gw);
}

static void assign_default_ip(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw)
{
	int err;

	xil_printf("Configuring default IP %s \r\n", DEFAULT_IP_ADDRESS);

	err = inet_aton(DEFAULT_IP_ADDRESS, ip);
	if(!err)
		xil_printf("Invalid default IP address: %d\r\n", err);

	err = inet_aton(DEFAULT_IP_MASK, mask);
	if(!err)
		xil_printf("Invalid default IP MASK: %d\r\n", err);

	err = inet_aton(DEFAULT_GW_ADDRESS, gw);
	if(!err)
		xil_printf("Invalid default gateway address: %d\r\n", err);
} //assign_default_ip

static void network_thread(void *p)
{
	int mscnt = 0;

	/* the mac address of the board. this should be unique per board */
	u8_t mac_ethernet_address[] = { 0x00, 0x0a, 0x35, 0x00, 0x01, 0x02 };

	xil_printf("\n\r");
	xil_printf("------lwIP Socket Mode TCP Startup------\r\n");

	/* Add network interface to the netif_list, and set it as default */
	if (!xemac_add(&server_netif, NULL, NULL, NULL, mac_ethernet_address,
	               XPAR_XEMACPS_0_BASEADDR)) {
		xil_printf("Error adding N/W interface\r\n");
		return;
	}

	netif_set_default(&server_netif);

	/* specify that the network if is up */
	netif_set_up(&server_netif);

	/* start packet receive thread - required for lwIP operation */
	sys_thread_new("xemacif_input_thread",
	               (void(*)(void*))xemacif_input_thread, &server_netif,
	               STANDARD_THREAD_STACKSIZE,
	               DEFAULT_THREAD_PRIO);

	complete_nw_thread = 1;

	/* Resume the network init thread; auto-negotiation is completed */
	vTaskResume(network_init_thread_handle);

	/* For rest of program's execution we run this loop to generate timers
	 * for DHCP handling by lwIP. */
	dhcp_start(&server_netif);
	while (1) {
		vTaskDelay(DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);
		dhcp_fine_tmr();
		mscnt += DHCP_FINE_TIMER_MSECS;
		if (mscnt >= DHCP_COARSE_TIMER_SECS*1000) {
			dhcp_coarse_tmr();
			mscnt = 0;
		}
	}

	return;
} //network_thread

void demo_FileViaSocket_thread(void *p)
{
	bool WasOK{ false };
	do { // We repeat attempts till the first connection is successfully made
		xil_printf( "opening socket to address %s:%d\r\n", SERVER_ADDR.c_str(), SERVER_PORT );

		try {
			FileViaSocket f( SERVER_ADDR, SERVER_PORT ); // Declare the object and open the connection

			f << "Hello world!\n"; /* We are using '\n' on purpose instead of std::endl, because
			                        * std::endl has a side effect of flushing the buffer, i.e.,
			                        * "Hello world!\n" would be immediately sent in a TCP packet. */
			f << "I'm here.\n";
			f << std::flush;       /* We are explicitly flushing the buffer, "Hello world!\nI'm here.\n"
			                        * is sent in a TCP packet. */
			f << "It worked.\n";

			WasOK = true;
		} // Object f ceases to exist, destructor on 'f' is called, buffer is flushed,
		  // "It worked.\n" is sent in a TCP packet, the socket connection is closed, a file is created on the server
		catch( const FileViaSocket::WrongServerIPFormatExc& e ) {
			std::string err( "Error on opening the socket: " + std::string(e.what()) + "\r\n" );
			xil_printf( err.c_str() );
			xil_printf( "TERMINATED\r\n" );
			vTaskDelete(NULL); // We end this thread since the provided server IP address is wrong
		}
		catch( const std::exception& e ) {
			std::string err( "Error on opening the socket: " + std::string(e.what()) + "\r\n" );
			xil_printf( err.c_str() );
			vTaskDelay( pdMS_TO_TICKS( 3000 ) ); // Wait 3 sec, perhaps the problem disappears on next try
		}
	} while( ! WasOK );

	xil_printf( "\"Hello world\" sent\r\n" );

	/* We must give the server some time to close the connection on its end.
	   Otherwise, the next call of 'open' would fail because of refused connection. */
	vTaskDelay( pdMS_TO_TICKS( 50 ) ); // Wait 50 ms

	FileViaSocket f; // We just declare the object, no connection is made

	try {
		f.open( SERVER_ADDR, SERVER_PORT ); // Open connection to the server
	} catch( const std::exception& e ) {
		std::string err( "Error on opening the socket: " + std::string(e.what()) + "\r\n" );
		xil_printf( err.c_str() );
		xil_printf( "TERMINATED\r\n" );
		vTaskDelete(NULL); // We end this thread
	}

	f << '1' << "23456" << 78; // We can write all kinds of data types to an ostream
	f.close(); // Close the connection, another file is created on the server

	xil_printf( "\"12345678\" sent\r\n" );

	vTaskDelay( pdMS_TO_TICKS( 50 ) ); // Wait 50 ms

	try {
		f.open( SERVER_ADDR, SERVER_PORT ); // Open a new connection on the same object
	} catch( const std::exception& e ) {
		std::string err( "Error on opening the socket: " + std::string(e.what()) + "\r\n" );
		xil_printf( err.c_str() );
		xil_printf( "TERMINATED\r\n" );
		vTaskDelete(NULL); // We end this thread
	}

	// Prepare a buffer to be written to the server
	const unsigned BUFF_SIZE{ 26*1000 }; /* Size of a buffer sent by one call of write().
	                                        WARNING: Be sure you have big enough stack in this thread.
	                                                 Set value of SEND_DATA_THREAD_STACKSIZE accordingly. */
	const unsigned BUFFER_COUNT{ 1000 }; /* Number of times we sent the buffer.
	                                        Set this to a high number to perform bulk transfer test. */
	char buffer[BUFF_SIZE];
	for( unsigned i = 0; i < BUFF_SIZE; i++ )
		buffer[i] = 'A' + i % 26; // Fill with repeated sequence from 'A' to 'Z'

	/* Test of bulk data transfer.
	 * On Zybo Z7 I achieved the transfer speed of about 340 Mbps */
	for( unsigned i = 0; i < BUFFER_COUNT; i++ )
		f.write( buffer, BUFF_SIZE ); // Write the whole buffer to the server
	f.close(); // The connection is closed, third file is created on the server

	xil_printf( "Buffer sent. All done.\r\n");
	vTaskDelete(NULL); // All done, we end this thread
} //demo_FileViaSocket_thread

void network_init_thread(void *p)
{
	int mscnt = 0;

	/* initialize lwIP before calling sys_thread_new */
	lwip_init();

	/* Start the thread, which will start the network and DHCP.  */
	/* any thread using lwIP should be created using sys_thread_new */
	sys_thread_new("nw_thread", network_thread, NULL,
	               STANDARD_THREAD_STACKSIZE,
	               DEFAULT_THREAD_PRIO);

	/* Suspend Task until auto-negotiation is completed */
	if (!complete_nw_thread)
		vTaskSuspend(NULL);

	/* Wait for IP address being obtained by DHCP */
	while (1) {
		vTaskDelay(DHCP_FINE_TIMER_MSECS / portTICK_RATE_MS);
		if (server_netif.ip_addr.addr) {
			xil_printf("DHCP request success\r\n");
			break;
		}
		mscnt += DHCP_FINE_TIMER_MSECS;
		if (mscnt >= 10000) {
			xil_printf("ERROR: DHCP request timed out\r\n");
			assign_default_ip(&(server_netif.ip_addr), &(server_netif.netmask),
							&(server_netif.gw));
			break;
		}
	}

	print_ip_settings(&(server_netif.ip_addr), &(server_netif.netmask), &(server_netif.gw));

	xil_printf("\r\n");

	/*** network is ready, we can now start the thread, which sends data using FileViaSocket class ***/

	/* any thread using lwIP should be created using sys_thread_new */
	sys_thread_new("demo_FileViaSocket", demo_FileViaSocket_thread, NULL,
	               SEND_DATA_THREAD_STACKSIZE,
	               DEFAULT_THREAD_PRIO);

	vTaskDelete(NULL); // All done, we can end this thread
	return;
} //network_init_thread

int main()
{
	/* Starting thread, which initializes the network.
	 * It will start the demo_FileViaSocket_thread when the network is ready. */
	network_init_thread_handle = sys_thread_new("network_init_thread", network_init_thread, 0,
	                                            STANDARD_THREAD_STACKSIZE,
	                                            DEFAULT_THREAD_PRIO);
	vTaskStartScheduler();

	return 0;
} //main
