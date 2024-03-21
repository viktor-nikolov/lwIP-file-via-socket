[![License](https://img.shields.io/badge/License-BSD_2--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)
# "File via socket" for lwIP
This repository provides a C++ ostream class (client) and a Python script (server) for writing a file on a remote system via an IP socket connection.  
My primary motivation for creating this was to upload extensive debug data (e.g., data from an ADC) from a standalone application running in [FreeRTOS](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842141/FreeRTOS) on AMD [Xilinx Zynq](https://www.xilinx.com/products/silicon-devices/soc/zynq-7000.html) SoC (FreeRTOS uses the [lwIP](https://savannah.nongnu.org/projects/lwip/) stack).  
Nevertheless, the C++ class works also on Windows and Linux.

For example, when you start the server like this

```
python3 file_via_socket.py --path ~/test_data --bind_port 63333
```

and use the following code in the application

```c++
{
    FileViaSocket f( "192.168.44.44", 63333 ); // Connect to the server
    f << "Data:\n";                            // Write the header row
    std::array<int, 3> SampleData = {1, 2, 3};
    for(auto element : SampleData)
        f << element << '\n';                  // Write a data row
} // Destructor on 'f' is called, the connection is closed
```

then the file `~/test_data/via_socket_240318_213421.5840.txt` is created on the server with the content

```
Data:
1
2
3
```

(The file name contains the time stamp for the connection.)

## How to use

### Client-side

Tested (and ready for compilation) on FreeRTOS on AMD Xilinx Zynq SoC (Vitis 2023.1 toolchain),
Windows 11 (MinGW toolchain) and Ubuntu 22.04 (gcc toolchain).

```
g++ -o DemoFileViaSocket DemoFileViaSocket.cpp FileViaSocket.cpp
```

tbd tbd

```
$ ./DemoFileViaSocket 192.168.44.44
"Hello world" sent
"12345678" sent
Buffer sent. All done.
```

tbd

### Server-side

tbd

Run the script with the command 'python3 file_via_socket [params]' or 'python file_via_socket [params]'.

Tested on Ubuntu 22.04 and Windows 11.

tbd

```
usage: file_via_socket [-h] [--path PATH] [--prefix PREFIX] [--ext EXT] [--bind_ip BIND_IP]
                       [--bind_port BIND_PORT]

options:
  -h, --help             show help message and exit
  --path PATH            path for storing the files; defaults to current working directory
  --prefix PREFIX        prefix of the file name; defaults to "via_socket"
  --ext EXT              extension for the file name; defaults to "txt"
  --bind_ip BIND_IP      local IP to bind the socket listener to; defaults to 0.0.0.0
  --bind_port BIND_PORT  local port to listen to connection on; values: 1024..65535; defaults to 65432
```

## Sample project files

enable lwIP in the BSP

[<img src="https://github.com/viktor-nikolov/lwIP-file-via-socket/blob/main/pictures/enable_lwIP.png?raw=true" title="" alt="" width="500">](https://github.com/viktor-nikolov/lwIP-file-via-socket/blob/main/pictures/enable_lwIP.png)

tbd

api_mode "SOCKET API" because it is stand alone app

dhcp_options/lwip_dhcp true

temac_adapter_options/phy_link_speed "1000 Mbps"
