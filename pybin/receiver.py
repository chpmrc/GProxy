#!/usr/bin/env python
import socket
import sys

HOST = ""
PORT = 64000
FILENAME = sys.argv[1];
output = open(FILENAME, 'w');

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind((HOST, PORT))
s.listen(1)
conn, addr = s.accept()
print 'Connected by ', addr
while 1:
	data = conn.recv(65000)
	if not data:
		break
	output.write(data)
		
conn.close()
output.close()
