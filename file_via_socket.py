# This is the server side script for receiving data via IP socket. Creates a file for each socket connection,
# with timestamp in the file name.
# For details see the GitHub repository https://github.com/viktor-nikolov/lwIP-file-via-socket
#
# Run the script with the command 'python3 file_via_socket [params]' or 'python file_via_socket [params]'.
# Tested on Ubuntu 22.04 and Windows 11.
#
# usage: file_via_socket [-h] [--path PATH] [--prefix PREFIX] [--ext EXT] [--bind_ip BIND_IP] [--bind_port BIND_PORT]
#
# options:
#   -h, --help              Show help message and exit
#   --path PATH             Path for storing the files; defaults to current working directory
#   --prefix PREFIX         Prefix of the file name; defaults to "via_socket"
#   --ext EXT               Extension for the file name; defaults to "txt"
#   --bind_ip BIND_IP       Local IP to bind the socket listener to; defaults to 0.0.0.0
#   --bind_port BIND_PORT   Local port to listen to connection on; values: 1024..65535; defaults to 65432
#
# BSD 2-Clause License:
#
# Copyright (c) 2024 Viktor Nikolov
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import ipaddress
import socket
import signal
import argparse
from datetime import datetime
from select import select


def signal_handler(signum, frame):
    # Handler for Ctlr+C signal
    print("\nExecution interrupted by the user. Exiting...")
    exit(1)


def check_valid_ip(arg_val):
    # Checking value of IP address passed as command line argument
    # We attempt to create an IPv4 or IPv6 address object. If successful, the string is a valid IP address.
    # Otherwise, ValueError exception is raised.
    ipaddress.ip_address(arg_val)
    return arg_val


def check_port_value(arg_val):
    # Checking value of port passed as command line argument
    port = int(arg_val)
    if port <= 1023 or port > 65535:
        raise ValueError()
    return port


def bytes2human_readable(bytes_val):
    # Convert number of bytes to a human-readable value with units
    if bytes_val > 1024 * 1024 * 1024:
        return "{:.2f} GB".format(bytes_val / (1024 * 1024 * 1024))
    elif bytes_val > 1024 * 1024:
        return "{:.2f} MB".format(bytes_val / (1024 * 1024))
    elif bytes_val > 1024:
        return "{:.2f} KB".format(bytes_val / 1024)
    else:
        return "{:} B".format(bytes_val)


signal.signal(signal.SIGINT, signal_handler)  # Handling Ctrl+C

# Global variables
filePath = ""
filePrefix = "via_socket"
fileExt = "txt"
bindIP = "0.0.0.0"  # Standard loopback interface address (localhost)
bindPort = 65432    # Port to listen on (non-privileged ports are > 1023)
fileName = ""       # Initializing the variable to avoid a warning

# Create the command line argument parser
parser = argparse.ArgumentParser(prog="file_via_socket",
                                 description='Server side script for receiving data via IP socket. '
                                             'Creates a file for each socket connection, with timestamp in the file name.')
# Add arguments
parser.add_argument('--path', type=str, help='path for storing the files; defaults to current working directory')
parser.add_argument('--prefix', type=str, help=f'prefix of the file name; defaults to "{filePrefix}"')
parser.add_argument('--ext', type=str, help=f'extension for the file name; defaults to "{fileExt}"')
parser.add_argument('--bind_ip', type=check_valid_ip, help=f'local IP to bind the socket listener to; defaults to {bindIP}')
parser.add_argument('--bind_port', type=check_port_value,
                    help=f'local port to listen to connection on; values: 1024..65535; defaults to {bindPort}')
# Parse the command line arguments
args = parser.parse_args()
# Store command line arguments values
if args.path is not None:
    filePath = args.path.rstrip('/\\')  # remove trailing character '/' or '\'
if args.prefix is not None:
    filePrefix = args.prefix
if args.ext is not None:
    fileExt = args.ext.lstrip('.')  # remove leading character '.'
if args.bind_ip is not None:
    bindIP = args.bind_ip
if args.bind_port is not None:
    bindPort = args.bind_port

print(f"Waiting for connection on {bindIP}:{bindPort}\n"
       "(Press Ctrl+C to terminate)")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    #  Set the IP socket
    s.bind((bindIP, bindPort))
    s.listen(1)
    while True:  # The outer cycle is for each socket connection, i.e., for each file
        while True:
            # We are using "select" because if we called s.accept() directly, it wouldn't be terminated by Ctrl+C
            ready, _, _ = select([s], [], [], 1)  # Check if we have an incoming connection; timeout set to 1 sec
            if ready:
                conn, addr = s.accept()  # Connection object is returned
                break  # We have a connection, let's handle incoming data

        # Construct the file name
        fileName = ""
        if filePath != "":
            fileName = filePath + '/'
        now = datetime.now()
#                                             date and time             first 4 digits of microseconds
        fileName += filePrefix + now.strftime("_%y%m%d_%H%M%S") + '.' + now.strftime('%f')[:4]
        if fileExt != "":
            fileName += "." + fileExt
        try:
            with conn, open(fileName, 'wb') as f:
                print(f"Got connection from {addr[0]}:{addr[1]}")
                dataTotal = 0
                while True:  # Reading data sent by client via the socket
                    try:
                        data = conn.recv(4096)
                        if data:
                            f.write(data)
                            dataTotal += len(data)
                        if not data:  # Empty data means that the client has disconnected
                            break
                    except ConnectionResetError:
                        break
                print(f"    Received total: {bytes2human_readable(dataTotal)}")
        except FileNotFoundError:
            print(f"ERROR: Unable to open file '{fileName}'")
            exit(1)
        except PermissionError:
            print(f"ERROR: Permission error when opening file '{fileName}'")
            exit(1)
