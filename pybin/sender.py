#!/usr/bin/env python

import socket
import sys

HOST = "192.168.1.129"
PORT = 59000
FILENAME = sys.argv[1];
input = open(FILENAME, 'r');

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))
s.sendall(input.read())
s.close()
input.close()
print("Transmission complete (sender)\n");

