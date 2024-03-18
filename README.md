[![License](https://img.shields.io/badge/License-BSD_2--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)
# "File via socket" for lwIP
This repository provides a C++ ostream class (client) and a Python script (server) for writing a file on a remote system via an IP socket connection.  
My main motivation for creating this was to upload large debug data (e.g., data from an ADC) from a standalone application running in [FreeRTOS](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842141/FreeRTOS) on AMD Xilinx Zynq SoC (FreeRTOS uses the [lwIP](https://savannah.nongnu.org/projects/lwip/) stack).  
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
    std::array<int, 5> SampleData = {1, 2, 3, 4, 5};
    for(auto element : SampleData)
        f << element << '\n';                  // Write a data row
} // Destructor is called, connection is closed
```

then file `~/test_data/via_socket_240318_213421.5840.txt` is created on the server with the content

```
Data:
1
2
3
4
5
```
